/*
    QuickerSFV - A fast checksum checker
    Copyright (C) 2025  Andreas Weis (der_ghulbus@ghulbus-inc.de)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <quicker_sfv/quicker_sfv.hpp>

#include <ui/enforce.hpp>
#include <ui/file_dialog.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#   define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#   define NOMINMAX
#endif
#include <Windows.h>
#include <windowsx.h>
#include <atlbase.h>
#include <CommCtrl.h>
#include <ShObjIdl_core.h>
#include <strsafe.h>
#include <tchar.h>
#include <uxtheme.h>

#include <resource.h>

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstring>
#include <generator>
#include <limits>
#include <memory>
#include <numeric>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include <condition_variable>
#include <mutex>
#include <thread>

using namespace quicker_sfv;
using quicker_sfv::gui::enforce;

constexpr UINT const WM_SCHEDULER_WAKEUP = WM_USER + 1;

class EventHandler {
public:
    EventHandler& operator=(EventHandler&&) = delete;
    virtual ~EventHandler() = 0;

    enum class CompletionStatus {
        Ok,
        Missing,
        Bad,
    };
    struct Result {
        uint32_t total;
        uint32_t ok;
        uint32_t bad;
        uint32_t missing;
    };

    virtual void onCheckStarted(uint32_t n_files) = 0;
    virtual void onProgress(std::u8string_view file, uint32_t percentage, uint32_t bandwidth_mib_s) = 0;
    virtual void onFileCompleted(std::u8string_view file, Digest const& checksum, CompletionStatus status) = 0;
    virtual void onCheckCompleted(Result r) = 0;
    virtual void onCancelRequested() = 0;
    virtual void onCanceled() = 0;
    virtual void onError(quicker_sfv::Error error, std::u8string_view msg) = 0;
};

EventHandler::~EventHandler() = default;


struct CreateOperation {
    ChecksumFile output;
    std::vector<std::u16string> paths_to_scan;
    std::u16string target_file_path;
};

struct VerifyOperation {
    EventHandler* event_handler;
    HasherOptions options;
    std::u16string source_file;
    ChecksumProvider* provider;
};

struct CreateFromFolderOperation {
    EventHandler* event_handler;
    HasherOptions options;
    std::u16string target_file;
    std::u16string folder_path;
    ChecksumProvider* provider;
};

struct CancelOperation {
    EventHandler* event_handler;
};

struct Scheduler {
private:
    struct OperationState {
        bool cancel_requested;
        EventHandler* event_handler;
        ChecksumProvider* checksum_provider;
        enum Op {
            Create,
            Verify
        } kind;
        ChecksumFile checksum_file;
        std::u16string checksum_path;
        HasherPtr hasher;
    };
    std::vector<OperationState> m_opsQueue;
    std::mutex m_mtxOps;
    std::condition_variable m_cvOps;
    bool m_shutdownRequested;
    HANDLE m_cancelEvent;

    struct Event {
        struct ECheckStarted {
            uint32_t n_files;
        };
        struct EProgress {
            std::u8string file;
            uint32_t percentage;
            uint32_t bandwidth_mib_s;
        };
        struct EFileCompleted {
            std::u8string file;
            Digest checksum;
            EventHandler::CompletionStatus status;
        };
        struct ECheckCompleted {
            EventHandler::Result r;
        };
        struct ECancelRequested {
        };
        struct ECanceled {
        };
        struct EError {
            Error error;
            std::u8string msg;
        };
        EventHandler* recipient;
        std::variant<ECheckStarted, EProgress, EFileCompleted, ECheckCompleted, ECancelRequested, ECanceled, EError> event;
    };
    std::vector<Event> m_eventsQueue;
    std::mutex m_mtxEvents;

    std::thread m_worker;
    DWORD m_startingThreadId;
public:
    Scheduler();
    ~Scheduler();
    Scheduler& operator=(Scheduler&&) = delete;
    void start();
    void shutdown();
    void run();
    void post(VerifyOperation op);
    void post(CancelOperation op);
    void post(CreateFromFolderOperation op);
private:
    void worker();
    void doVerify(OperationState& op);

    void signalCheckStarted(EventHandler* recipient, uint32_t n_files);
    void signalProgress(EventHandler* recipient, std::u8string_view file, uint32_t percentage, uint32_t bandwidth_mib_s);
    void signalFileCompleted(EventHandler* recipient, std::u8string_view file, Digest checksum, EventHandler::CompletionStatus status);
    void signalCheckCompleted(EventHandler* recipient, EventHandler::Result r);
    void signalCancelRequested(EventHandler* recipient);
    void signalCanceled(EventHandler* recipient);
    void signalError(EventHandler* recipient, quicker_sfv::Error error, std::u8string_view msg);

    static void dispatchEvent(EventHandler* recipient, Event::ECheckStarted const& e);
    static void dispatchEvent(EventHandler* recipient, Event::EProgress const& e);
    static void dispatchEvent(EventHandler* recipient, Event::EFileCompleted const& e);
    static void dispatchEvent(EventHandler* recipient, Event::ECheckCompleted const& e);
    static void dispatchEvent(EventHandler* recipient, Event::ECancelRequested const& e);
    static void dispatchEvent(EventHandler* recipient, Event::ECanceled const& e);
    static void dispatchEvent(EventHandler* recipient, Event::EError const& e);
};

Scheduler::Scheduler()
    :m_shutdownRequested(false), m_cancelEvent(nullptr)
{}

Scheduler::~Scheduler() {
    if (m_worker.joinable()) {
        shutdown();
    }
    CloseHandle(m_cancelEvent);
}

void Scheduler::start() {
    m_startingThreadId = GetCurrentThreadId();
    m_cancelEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!m_cancelEvent) {
        throwException(Error::Failed);
    }
    m_worker = std::thread([this]() { worker(); });
}

void Scheduler::shutdown() {
    {
        std::scoped_lock lk(m_mtxOps);
        m_shutdownRequested = true;
    }
    m_cvOps.notify_all();
    SetEvent(m_cancelEvent);
    m_worker.join();
}

void Scheduler::run() {
    std::vector<Event> pending_events;
    {
        std::scoped_lock lk(m_mtxEvents);
        swap(pending_events, m_eventsQueue);
    }
    for (auto const& e : pending_events) {
        std::visit([r = e.recipient](auto&& ev) { dispatchEvent(r, ev); }, e.event);
    }
}

void Scheduler::post(VerifyOperation op) {
    std::scoped_lock lk(m_mtxOps);
    m_opsQueue.push_back(OperationState{
        .cancel_requested = false,
        .event_handler = op.event_handler,
        .checksum_provider = op.provider,
        .kind = OperationState::Op::Verify,
        .checksum_file = ChecksumFile{},
        .checksum_path = std::move(op.source_file),
        .hasher = op.provider->createHasher(op.options)
    });
    m_cvOps.notify_one();
}

void Scheduler::post(CancelOperation) {
    SetEvent(m_cancelEvent);
}

void Scheduler::post(CreateFromFolderOperation op) {
    // @todo
}

void Scheduler::worker() {
    std::vector<OperationState> pending_ops;
    for (;;) {
        {
            std::unique_lock lk(m_mtxOps);
            m_cvOps.wait(lk, [this]() { return m_shutdownRequested || !m_opsQueue.empty(); });
            if (m_shutdownRequested) { break; }
            pending_ops.clear();
            swap(pending_ops, m_opsQueue);
        }
        for (auto& op : pending_ops) {
            try {
                switch (op.kind) {
                case OperationState::Op::Verify:
                    doVerify(op);
                    break;
                }
            } catch (Exception& e) {
                signalError(op.event_handler, e.code(), e.what8());
            }
        }
    }
}

namespace {
inline char16_t const* assumeUtf16(LPCWSTR z_str) {
    return reinterpret_cast<char16_t const*>(z_str);
}

inline LPCWSTR toWcharStr(std::u16string const& u16str) {
    return reinterpret_cast<LPCWSTR>(u16str.c_str());
}

std::u16string resolvePath(std::u16string const& path) {
    wchar_t empty;
    DWORD const required_size = GetFullPathName(toWcharStr(path), 0, &empty, nullptr);
    wchar_t* buffer = new wchar_t[required_size];
    GetFullPathName(toWcharStr(path), required_size, buffer, nullptr);
    std::u16string ret{ assumeUtf16(buffer) };
    delete buffer;
    return ret;
}

LPCTSTR formatString(LPTSTR out_buffer, size_t buffer_size, LPCTSTR format, ...) {
    va_list args;
    va_start(args, format);
    HRESULT hres = StringCchVPrintf(out_buffer, buffer_size, format, args);
    if ((hres != S_OK) && (hres != STRSAFE_E_INSUFFICIENT_BUFFER)) {
        throwException(Error::Failed);
    }
    va_end(args);
    return out_buffer;
}

std::u16string formatString(size_t buffer_size, LPCTSTR format, ...) {
    va_list args;
    va_start(args, format);
    std::u16string ret;
    ret.resize(buffer_size);
    size_t remain = 0;
    HRESULT hres = StringCchVPrintfEx(reinterpret_cast<TCHAR*>(ret.data()), buffer_size - 1, nullptr, &remain, 0, format, args);
    if ((hres != S_OK) && (hres != STRSAFE_E_INSUFFICIENT_BUFFER)) {
        throwException(Error::Failed);
    }
    va_end(args);
    ret.resize(ret.size() - remain - 1);
    return ret;
}
} // anonymous namespace

class FileInputWin32 : public FileInput {
private:
    HANDLE m_fin;
    bool m_eof;
public:
    FileInputWin32(std::u16string const& filename)
        :m_eof(false)
    {
        m_fin = CreateFile(toWcharStr(filename), GENERIC_READ, FILE_SHARE_READ, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (m_fin == INVALID_HANDLE_VALUE) {
            std::abort();
        }
    }

    ~FileInputWin32() override {
        CloseHandle(m_fin);
    }

    size_t read(std::span<std::byte> read_buffer) override {
        if (read_buffer.size() >= std::numeric_limits<DWORD>::max()) { std::abort(); }
        if (m_eof) { return FileInput::RESULT_END_OF_FILE; }
        DWORD bytes_read = 0;
        if (!ReadFile(m_fin, read_buffer.data(), static_cast<DWORD>(read_buffer.size()), &bytes_read, nullptr)) {
            quicker_sfv::throwException(Error::FileIO);
        }
        if (bytes_read == 0) {
            m_eof = true;
            return FileInput::RESULT_END_OF_FILE;
        }
        return bytes_read;
    }
};


class FileOutputWin32 : public FileOutput {
private:
    HANDLE m_fout;
public:
    FileOutputWin32(std::u16string const& filename)
    {
        m_fout = CreateFile(toWcharStr(filename), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (m_fout == INVALID_HANDLE_VALUE) {
            throwException(Error::FileIO);
        }
    }

    ~FileOutputWin32() override {
        CloseHandle(m_fout);
    }

    size_t write(std::span<std::byte const> bytes_to_write) override {
        if (bytes_to_write.size() >= std::numeric_limits<DWORD>::max()) { std::abort(); }
        DWORD bytes_written = 0;
        if (!WriteFile(m_fout, bytes_to_write.data(), static_cast<DWORD>(bytes_to_write.size()), &bytes_written, nullptr)) {
            throwException(Error::FileIO);
        }
        return bytes_written;
    }
};

class HandleGuard {
private:
    HANDLE h;
public:
    explicit HandleGuard(HANDLE handle) :h(handle) {}
    ~HandleGuard() { CloseHandle(h); }
    HandleGuard& operator=(HandleGuard&&) = delete;
};

template<typename T, size_t N>
class SlidingWindow {
private:
    std::array<T, N> m_elements;
    size_t m_numberOfElements = 0;
    size_t m_nextElement = 0;
public:
    void push(T&& e) {
        m_elements[m_nextElement] = std::move(e);
        m_nextElement = (m_nextElement + 1) % N;
        m_numberOfElements = std::min(m_numberOfElements + 1, N);
    }

    void push(T const& e) {
        m_elements[m_nextElement] = e;
        m_nextElement = (m_nextElement + 1) % N;
        m_numberOfElements = std::min(m_numberOfElements + 1, N);
    }

    T rollingAverage() const {
        if (m_numberOfElements == 0) { return T{}; }
        return std::accumulate(begin(m_elements), begin(m_elements) + m_numberOfElements, T{}) / m_numberOfElements;
    }
};

void Scheduler::doVerify(OperationState& op) {
    auto const offsetLow = [](size_t i) -> DWORD { return static_cast<DWORD>(i & 0xffffffffull); };
    auto const offsetHigh = [](size_t i) -> DWORD { return static_cast<DWORD>((i >> 32ull) & 0xffffffffull); };

    std::u16string const base_path = [](std::u16string_view checksum_file_path) -> std::u16string {
        auto it_slash = std::find(checksum_file_path.rbegin(), checksum_file_path.rend(), u'\\').base();
        return std::u16string{ checksum_file_path.substr(0, std::distance(checksum_file_path.begin(), it_slash)) };
    }(op.checksum_path);

    constexpr DWORD const BUFFER_SIZE = 4 << 20;
    FileInputWin32 reader(op.checksum_path);
    op.checksum_file = op.checksum_provider->readFromFile(reader);
    EventHandler::Result result{};
    result.total = static_cast<uint32_t>(op.checksum_file.getEntries().size());
    signalCheckStarted(op.event_handler, result.total);

    HANDLE event_front = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!event_front) {
        // catastrophic error
        throwException(Error::Failed);
    }
    HandleGuard guard_event_front(event_front);
    HANDLE event_back = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!event_back) {
        // catastrophic error
        throwException(Error::Failed);
    }
    HandleGuard guard_event_back(event_back);

    std::vector<char> front_buffer;
    front_buffer.resize(BUFFER_SIZE);
    std::vector<char> back_buffer;
    back_buffer.resize(BUFFER_SIZE);

    std::chrono::steady_clock::time_point t_front;
    std::chrono::steady_clock::time_point t_back;

    uint32_t last_progress = 0;

    for (auto const& f : op.checksum_file.getEntries()) {
        SlidingWindow<std::chrono::nanoseconds, 10> bandwidth_track;
        op.hasher->reset();

        std::u16string const file_path = resolvePath(base_path + convertToUtf16(f.path));
        HANDLE fin = CreateFile(toWcharStr(file_path), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_OVERLAPPED,
                                nullptr);
        if (fin == INVALID_HANDLE_VALUE) {
            // file missing
            ++result.missing;
            signalFileCompleted(op.event_handler, f.path, Digest{}, EventHandler::CompletionStatus::Missing);
            continue;
        }
        HandleGuard guard_fin(fin);

        size_t read_offset = 0;
        size_t bytes_hashed = 0;
        bool is_eof = false;
        bool is_canceled = false;
        bool is_error = false;
        size_t const file_size = [&is_error, fin]() -> size_t {
            LARGE_INTEGER l_file_size;
            if (!GetFileSizeEx(fin, &l_file_size)) {
                is_error = true;
                return 0;
            }
            return l_file_size.QuadPart;
        }();

        bool front_pending = false;
        bool back_pending = false;
        OVERLAPPED overlapped_storage[2] = {};
        LPOVERLAPPED overlapped_front = &overlapped_storage[0];
        LPOVERLAPPED overlapped_back = &overlapped_storage[1];
        *overlapped_front = OVERLAPPED{
            .Offset = offsetLow(read_offset),
            .OffsetHigh = offsetHigh(read_offset),
            .hEvent = event_front
        };
        t_front = std::chrono::steady_clock::now();
        if (!ReadFile(fin, front_buffer.data(), BUFFER_SIZE, nullptr, overlapped_front)) {
            DWORD const err = GetLastError();
            if (err == ERROR_HANDLE_EOF) {
                // eof before async
                is_eof = true;
            } else if (err == ERROR_IO_PENDING) {
                front_pending = true;
            } else {
                // file read error
                is_error = true;
                break;
            }
        }
        read_offset += front_buffer.size();
        while (!is_eof && !is_canceled && !is_error) {
            *overlapped_back = OVERLAPPED{
                    .Offset = offsetLow(read_offset),
                    .OffsetHigh = offsetHigh(read_offset),
                    .hEvent = event_back
            };
            t_back = std::chrono::steady_clock::now();
            if (!ReadFile(fin, back_buffer.data(), BUFFER_SIZE, nullptr, overlapped_back)) {
                DWORD const err = GetLastError();
                if (err == ERROR_HANDLE_EOF) {
                    // eof before async
                    is_eof = true;
                } else if (err == ERROR_IO_PENDING) {
                    back_pending = true;
                } else {
                    // file read error
                    is_error = true;
                    break;
                }
            }
            read_offset += front_buffer.size();

            DWORD bytes_read = 0;
            // cancel event has to go first, or it will get starved by completing i/os
            HANDLE event_handles[] = { m_cancelEvent, event_front };
            DWORD const wait_ret = WaitForMultipleObjects(2, event_handles, FALSE, INFINITE);
            if (wait_ret == WAIT_OBJECT_0 + 1) {
                // read successful
                front_pending = false;
                if (!GetOverlappedResult(fin, overlapped_front, &bytes_read, FALSE)) {
                    if (GetLastError() == ERROR_HANDLE_EOF) {
                        // eof
                        is_eof = true;
                    } else {
                        // file read error
                        is_error = true;
                        break;
                    }
                } else {
                    if (bytes_read == BUFFER_SIZE) { bandwidth_track.push(std::chrono::steady_clock::now() - t_front); }
                }
            } else if (wait_ret == WAIT_OBJECT_0) {
                // cancel
                CancelIo(fin);
                is_canceled = true;
                break;
            } else {
                // catastrophic error: unexpected wait result
                throwException(Error::Failed);
            }

            op.hasher->addData(std::span<char const>(front_buffer.data(), bytes_read));
            bytes_hashed += bytes_read;
            uint32_t current_progress = static_cast<uint32_t>(bytes_hashed * 100 / file_size);
            if (current_progress != last_progress) {
                int64_t const t_avg = bandwidth_track.rollingAverage().count();
                uint32_t const bandwidth_mib_s = static_cast<uint32_t>((t_avg) ? ((static_cast<int64_t>(BUFFER_SIZE) * 1'000'000'000ll) / (t_avg * 1'048'576ll)) : 0);
                signalProgress(op.event_handler, f.path, current_progress, bandwidth_mib_s);
                last_progress = current_progress;
            }

            std::swap(front_buffer, back_buffer);
            std::swap(event_front, event_back);
            std::swap(overlapped_front, overlapped_back);
            std::swap(front_pending, back_pending);
            std::swap(t_front, t_back);
        }
        if (front_pending) { WaitForSingleObject(event_front, INFINITE); DWORD b; GetOverlappedResult(fin, overlapped_front, &b, TRUE); }
        if (back_pending) { WaitForSingleObject(event_back, INFINITE); DWORD b; GetOverlappedResult(fin, overlapped_back, &b, TRUE); }
        if (is_canceled) {
            signalCanceled(op.event_handler);
            return;
        }
        auto const digest = op.hasher->finalize();
        if (digest == f.digest) {
            signalFileCompleted(op.event_handler, f.path, std::move(digest), EventHandler::CompletionStatus::Ok);
            ++result.ok;
        } else {
            signalFileCompleted(op.event_handler, f.path, std::move(digest), EventHandler::CompletionStatus::Bad);
            ++result.bad;
        }
    }
    signalCheckCompleted(op.event_handler, result);
}

void Scheduler::signalCheckStarted(EventHandler* recipient, uint32_t n_files) {
    std::scoped_lock lk(m_mtxEvents);
    m_eventsQueue.emplace_back(Event {
        .recipient = recipient,
        .event = Event::ECheckStarted {
            .n_files = n_files
        }
    });
}

void Scheduler::signalProgress(EventHandler* recipient, std::u8string_view file, uint32_t percentage, uint32_t bandwidth_mib_s) {
    std::scoped_lock lk(m_mtxEvents);
    m_eventsQueue.emplace_back(Event {
        .recipient = recipient,
        .event = Event::EProgress {
            .file = std::u8string(file),
            .percentage = percentage,
            .bandwidth_mib_s = bandwidth_mib_s
        }
    });
    PostThreadMessage(m_startingThreadId, WM_SCHEDULER_WAKEUP, 0, 0);
}

void Scheduler::signalFileCompleted(EventHandler* recipient, std::u8string_view file, Digest checksum, EventHandler::CompletionStatus status) {
    std::scoped_lock lk(m_mtxEvents);
    m_eventsQueue.emplace_back(Event {
        .recipient = recipient,
        .event = Event::EFileCompleted {
            .file = std::u8string(file),
            .checksum = std::move(checksum),
            .status = status
        }
    });
    PostThreadMessage(m_startingThreadId, WM_SCHEDULER_WAKEUP, 0, 0);
}

void Scheduler::signalCheckCompleted(EventHandler* recipient, EventHandler::Result r) {
    std::scoped_lock lk(m_mtxEvents);
    m_eventsQueue.emplace_back(Event {
        .recipient = recipient,
        .event = Event::ECheckCompleted {
            .r = r
        }
    });
    PostThreadMessage(m_startingThreadId, WM_SCHEDULER_WAKEUP, 0, 0);
}

void Scheduler::signalCancelRequested(EventHandler* recipient) {
    std::scoped_lock lk(m_mtxEvents);
    m_eventsQueue.emplace_back(Event {
        .recipient = recipient,
        .event = Event::ECancelRequested {
        }
    });
    PostThreadMessage(m_startingThreadId, WM_SCHEDULER_WAKEUP, 0, 0);
}

void Scheduler::signalCanceled(EventHandler* recipient)  {
    std::scoped_lock lk(m_mtxEvents);
    m_eventsQueue.emplace_back(Event {
        .recipient = recipient,
        .event = Event::ECanceled {
        }
    });
    PostThreadMessage(m_startingThreadId, WM_SCHEDULER_WAKEUP, 0, 0);
}

void Scheduler::signalError(EventHandler* recipient, quicker_sfv::Error error, std::u8string_view msg) {
    std::scoped_lock lk(m_mtxEvents);
    m_eventsQueue.emplace_back(Event {
        .recipient = recipient,
        .event = Event::EError {
            .error = error,
            .msg = std::u8string(msg)
        }
    });
    PostThreadMessage(m_startingThreadId, WM_SCHEDULER_WAKEUP, 0, 0);
}

void Scheduler::dispatchEvent(EventHandler* recipient, Event::ECheckStarted const& e) {
    recipient->onCheckStarted(e.n_files);
}

void Scheduler::dispatchEvent(EventHandler* recipient, Event::EProgress const& e) {
    recipient->onProgress(e.file, e.percentage, e.bandwidth_mib_s);
}

void Scheduler::dispatchEvent(EventHandler* recipient, Event::EFileCompleted const& e) {
    recipient->onFileCompleted(e.file, e.checksum, e.status);
}

void Scheduler::dispatchEvent(EventHandler* recipient, Event::ECheckCompleted const& e) {
    recipient->onCheckCompleted(e.r);
}

void Scheduler::dispatchEvent(EventHandler* recipient, Event::ECancelRequested const&) {
    recipient->onCancelRequested();
}

void Scheduler::dispatchEvent(EventHandler* recipient, Event::ECanceled const&) {
    recipient->onCanceled();
}

void Scheduler::dispatchEvent(EventHandler* recipient, Event::EError const& e) {
    recipient->onError(e.error, e.msg);
}


class FileProviders {
private:
    std::vector<quicker_sfv::ChecksumProviderPtr> m_providers;
    struct FileType {
        std::u8string extensions;
        std::u8string description;
    };
    std::vector<FileType> m_fileTypes;
public:
    FileProviders()
    {
        m_providers.emplace_back(quicker_sfv::createSfvProvider());
        registerFileType(*m_providers.back());
        m_providers.emplace_back(quicker_sfv::createMD5Provider());
        registerFileType(*m_providers.back());
        // @todo:
        //  - .ckz
        //  - .par
        //  - .csv
        //  - .txt
    }

    ChecksumProvider* getMatchingProviderFor(std::u8string_view filename) {
        for (auto const& p: m_providers) {
            std::u8string_view exts = p->fileExtensions();
            // split extensions
            for (std::u8string_view::size_type it = 0; it != std::u8string_view::npos;) {
                auto const it_end = exts.find(u8';');
                auto ext = exts.substr(it, it_end);
                if (ext[0] == u8'*') {
                    ext = ext.substr(1);
                    if (filename.ends_with(ext)) {
                        return p.get();
                    }
                } else {
                    enforce(!"Invalid extension provided");
                }
                it = it_end;
            }
        }
        return nullptr;
    }

    std::span<FileType const> fileTypes() const {
        return m_fileTypes;
    }

    ChecksumProvider* getProviderFromIndex(UINT provider_index) {
        if ((provider_index >= 0) && (provider_index < m_providers.size())) {
            return m_providers[provider_index].get();
        }
        return nullptr;
    }
private:
    void registerFileType(ChecksumProvider const& provider) {
        m_fileTypes.emplace_back(std::u8string(provider.fileExtensions()), std::u8string(provider.fileDescription()));
    }
};

class MainWindow: public EventHandler {
private:
    HINSTANCE m_hInstance;
    TCHAR const* m_windowTitle;
    HWND m_hWnd;
    HMENU m_hMenu;
    HWND m_hTextFieldLeft;
    HWND m_hTextFieldRight;
    HWND m_hListView;
    HIMAGELIST m_imageList;

    struct Stats {
        uint32_t total;
        uint32_t completed;
        uint32_t progress;
        uint32_t ok;
        uint32_t bad;
        uint32_t missing;
        uint32_t bandwidth;
    } m_stats;
    struct ListViewEntry {
        std::u16string name;
        std::u16string checksum;
        enum Status {
            Ok,
            FailedMismatch,
            FailedMissing,
            Information
        } status;
    };
    std::vector<ListViewEntry> m_listEntries;

    HasherOptions m_options;
    FileProviders* m_fileProviders;
    Scheduler* m_scheduler;
public:
    explicit MainWindow(FileProviders& file_providers, Scheduler& scheduler);

    ~MainWindow() override;

    MainWindow(MainWindow const&) = delete;
    MainWindow(MainWindow&&) = delete;
    MainWindow& operator=(MainWindow const&) = delete;
    MainWindow& operator=(MainWindow&&) = delete;

    BOOL createMainWindow(HINSTANCE hInstance, int nCmdShow,
                          TCHAR const* class_name, TCHAR const* window_title);

    LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


    void onCheckStarted(uint32_t n_files) override;
    void onProgress(std::u8string_view file, uint32_t percentage, uint32_t bandwidth_mib_s) override;
    void onFileCompleted(std::u8string_view file, Digest const& checksum, CompletionStatus status) override;
    void onCheckCompleted(Result r) override;
    void onCancelRequested() override;
    void onCanceled() override;
    void onError(quicker_sfv::Error error, std::u8string_view msg) override;

private:
    LRESULT createUiElements(HWND parent_hwnd);

    LRESULT populateListView(NMHDR* nmh);

    void UpdateStats();

    void resize();
};

MainWindow::MainWindow(FileProviders& file_providers, Scheduler& scheduler)
    :m_hInstance(nullptr), m_windowTitle(nullptr), m_hWnd(nullptr), m_hTextFieldLeft(nullptr), m_hTextFieldRight(nullptr),
     m_hListView(nullptr), m_imageList(nullptr),
     m_stats{ },
     m_options{ .has_sse42 = true, .has_avx512 = false}, m_fileProviders(&file_providers), m_scheduler(&scheduler)
{
}

MainWindow::~MainWindow() = default;

struct FileSpec {
    std::vector<COMDLG_FILTERSPEC> fileTypes;
    std::vector<std::u16string> stringPool;
};

FileSpec determineFileTypes(FileProviders const& file_providers, bool include_catchall) {
    FileSpec ret;
    if (include_catchall) {
        ret.stringPool.push_back(u"File Verification Database");
        ret.stringPool.push_back(u"");
    }
    for (auto const& f : file_providers.fileTypes()) {
        ret.stringPool.push_back(convertToUtf16(f.description));
        ret.stringPool.push_back(convertToUtf16(f.extensions));
        if (include_catchall) {
            if (!ret.stringPool[1].empty()) { ret.stringPool[1].append(u";"); }
            ret.stringPool[1].append(ret.stringPool.back());
        }
    }
    if (include_catchall) {
        ret.stringPool.push_back(u"All Files");
        ret.stringPool.push_back(u"*.*");
    }
    ret.fileTypes.reserve(ret.stringPool.size() / 2);
    for (size_t i = 0; i < ret.stringPool.size(); i += 2) {
        ret.fileTypes.push_back(COMDLG_FILTERSPEC{ toWcharStr(ret.stringPool[i]), toWcharStr(ret.stringPool[i + 1]) });
    }
    return ret;
}

std::optional<gui::FileDialogResult> OpenFolder(HWND parent_window) {
    return gui::FileDialog(parent_window, gui::FileDialogAction::OpenFolder, nullptr, {});
}


std::optional<gui::FileDialogResult> OpenFile(HWND parent_window, FileProviders const& file_providers) {
    return gui::FileDialog(parent_window, gui::FileDialogAction::Open, nullptr, determineFileTypes(file_providers, true).fileTypes);
}

std::optional<gui::FileDialogResult> SaveFile(HWND parent_window, FileProviders const& file_providers) {
    return gui::FileDialog(parent_window, gui::FileDialogAction::SaveAs, nullptr, determineFileTypes(file_providers, false).fileTypes);
}

struct FileInfo {
    LPCWSTR absolute_path;
    std::u16string_view relative_path;
    uint64_t size;
};

std::generator<FileInfo> iterateFiles(std::u16string const& base_path) {
    auto const is_dot = [](LPCWSTR p) -> bool { return p[0] == L'.' && p[1] == L'\0'; };
    auto const is_dotdot = [](LPCWSTR p) -> bool { return p[0] == L'.' && p[1] == L'.' && p[2] == L'\0'; };
    auto const appendWildcard = [](std::u16string& str) {
        if (!str.empty() && str.back() == u'*') { str.pop_back(); }
        if (!str.empty() && str.back() == u'\\') { str.pop_back(); }
        str.append(u"\\*");
    };
    auto const relativePathTo = [](std::u16string_view p, std::u16string_view parent_path) -> std::u16string {
        std::u16string ret{ p };
        auto it = std::mismatch(ret.begin(), ret.end(), parent_path.begin(), parent_path.end());
        if (it.first != ret.end() && (*(it.first) == u'\\')) { ++it.first; }
        ret.erase(ret.begin(), it.first);
        return ret;
    };
    std::vector<std::u16string> directories;
    directories.emplace_back(base_path);
    appendWildcard(directories.back());
    while (!directories.empty()) {
        WIN32_FIND_DATA find_data;
        std::u16string const current_path = std::move(directories.back());
        directories.pop_back();
        HANDLE hsearch = FindFirstFileEx(toWcharStr(current_path), FindExInfoBasic, &find_data,
                                         FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
        if (hsearch == INVALID_HANDLE_VALUE) {
            MessageBox(nullptr, TEXT("Unable to open file for finding"), TEXT("Error"), MB_ICONERROR);
        }
        do {
            if (is_dot(find_data.cFileName) || is_dotdot(find_data.cFileName)) { continue; }
            std::u16string p = current_path;
            p.pop_back();   // pop wildcard
            p.append(assumeUtf16(find_data.cFileName));
            uint64_t filesize = (static_cast<uint64_t>(find_data.nFileSizeHigh) << 32ull) | static_cast<uint64_t>(find_data.nFileSizeLow);
            if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                appendWildcard(p);
                directories.emplace_back(std::move(p));
            } else {
                co_yield FileInfo{ .absolute_path = toWcharStr(p), .relative_path = relativePathTo(p, base_path), .size = filesize };
            }
        } while (FindNextFile(hsearch, &find_data) != FALSE);
        bool success = (GetLastError() == ERROR_NO_MORE_FILES);
        FindClose(hsearch);
    }
}

Digest hashFile(HasherPtr hasher, LPCWSTR filepath, uint64_t filesize) {
    HANDLE hin = CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (hin == INVALID_HANDLE_VALUE) {
        std::abort();
    }
    HANDLE hmappedFile = CreateFileMapping(hin, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!hmappedFile) {
        std::abort();
    }
    LPVOID fptr = MapViewOfFile(hmappedFile, FILE_MAP_READ, 0, 0, 0);
    if (!fptr) {
        std::abort();
    }
    std::span<char const> filespan{ reinterpret_cast<char const*>(fptr), (size_t)filesize };
    hasher->addData(filespan);
    UnmapViewOfFile(fptr);
    CloseHandle(hmappedFile);
    CloseHandle(hin);
    return hasher->finalize();
}

LRESULT MainWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_DESTROY) {
        m_scheduler->post(CancelOperation{});
        PostQuitMessage(0);
        return 0;
    } else if (msg == WM_CREATE) {
        return createUiElements(hWnd);
    } else if (msg == WM_COMMAND) {
        if ((lParam == 0) && (HIWORD(wParam) == 0)) {
            // Menu
            if (LOWORD(wParam) == ID_FILE_EXIT) {
                SendMessage(hWnd, WM_CLOSE, 0, 0);
                return 0;
            } else if (LOWORD(wParam) == ID_HELP_ABOUT) {
                MessageBox(hWnd, TEXT("About this!"), TEXT("About QuickerSFV"), MB_ICONINFORMATION);
                return 0;
            } else if (LOWORD(wParam) == ID_FILE_OPEN) {
                if (auto opt = OpenFile(hWnd, *m_fileProviders); opt) {
                    auto const& [source_file_path, selected_provider] = *opt;
                    ChecksumProvider* const checksum_provider =
                        ((selected_provider == 0) || (selected_provider - 1 >= m_fileProviders->fileTypes().size())) ?
                        m_fileProviders->getMatchingProviderFor(convertToUtf8(source_file_path)) :
                        m_fileProviders->getProviderFromIndex(selected_provider - 1);
                    if (checksum_provider) {
                        m_scheduler->post(VerifyOperation{
                            .event_handler = this,
                            .options = m_options,
                            .source_file = source_file_path,
                            .provider = checksum_provider
                        });
                    }
                }
                return 0;
            } else if (LOWORD(wParam) == ID_OPTIONS_USEAVX512) {
                MENUITEMINFO mii{ .cbSize = sizeof(MENUITEMINFO), .fMask = MIIM_STATE };
                GetMenuItemInfo(m_hMenu, ID_OPTIONS_USEAVX512, FALSE, &mii);
                if ((mii.fState & MFS_CHECKED) != 0) {
                    mii.fState &= ~MFS_CHECKED;
                    m_options.has_avx512 = false;
                } else {
                    mii.fState |= MFS_CHECKED;
                    m_options.has_avx512 = true;
                }
                SetMenuItemInfo(m_hMenu, ID_OPTIONS_USEAVX512, FALSE, &mii);
            } else if (LOWORD(wParam) == ID_CREATE_FROM_FOLDER) {
                if (auto const opt = OpenFolder(hWnd); opt) {
                    auto const& [folder_path, _] = *opt;
                    if (auto const opt_s = SaveFile(hWnd, *m_fileProviders); opt_s) {
                        auto const& [target_file_path, selected_provider] = *opt_s;
                        ChecksumProvider* checksum_provider =
                            (selected_provider >= m_fileProviders->fileTypes().size()) ?
                            m_fileProviders->getMatchingProviderFor(convertToUtf8(target_file_path)) :
                            m_fileProviders->getProviderFromIndex(selected_provider);
                        m_scheduler->post(CreateFromFolderOperation{
                            .event_handler = this,
                            .options = m_options,
                            .target_file = target_file_path,
                            .folder_path = folder_path,
                            .provider = checksum_provider,
                        });
                    }
                }
                return 0;
            }
        }
    } else if (msg == WM_NOTIFY) {
        NMHDR* nmh = std::bit_cast<LPNMHDR>(lParam);
        if (nmh->hwndFrom == m_hListView) {
            return populateListView(nmh);
        }
    } else if (msg == WM_SIZE) {
        resize();
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT MainWindow::populateListView(NMHDR* nmh) {
    if (nmh->code == LVN_GETDISPINFO) {
        NMLVDISPINFO* disp_info = std::bit_cast<NMLVDISPINFO*>(nmh);
        if (disp_info->item.iItem > m_listEntries.size()) { enforce(!"Should never happen"); return 0; }
        ListViewEntry& entry = m_listEntries[disp_info->item.iItem];
        if (disp_info->item.mask & LVIF_TEXT) {
            if (disp_info->item.iSubItem == 0) {
                // name
                _tcsncpy_s(disp_info->item.pszText, disp_info->item.cchTextMax,
                    toWcharStr(entry.name), _TRUNCATE);
            } else if (disp_info->item.iSubItem == 1) {
                // checksum
                _tcsncpy_s(disp_info->item.pszText, disp_info->item.cchTextMax,
                    toWcharStr(entry.checksum), _TRUNCATE);
            } else if (disp_info->item.iSubItem == 2) {
                // status
                TCHAR const* status_text[] = { TEXT("OK"), TEXT("FAILED. Checksum mismatch"), TEXT("FAILED. File does not exist"), TEXT("") };
                _tcsncpy_s(disp_info->item.pszText, disp_info->item.cchTextMax,
                    status_text[std::min<int>(entry.status, ARRAYSIZE(status_text) - 1)], _TRUNCATE);
            }
        } 
        if (disp_info->item.mask & LVIF_IMAGE) {
            if (disp_info->item.iSubItem == 0) {
                switch (entry.status) {
                case ListViewEntry::Status::Ok:
                    disp_info->item.iImage = 0;
                    break;
                case ListViewEntry::Status::FailedMismatch:
                    disp_info->item.iImage = 1;
                    break;
                case ListViewEntry::Status::FailedMissing:
                    disp_info->item.iImage = 1;
                    break;
                case ListViewEntry::Status::Information: [[fallthrough]];
                default:
                    disp_info->item.iImage = 2;
                    break;
                }
            }
        }
    } else if (nmh->code == LVN_ODCACHEHINT) {
        // not handled - all items always in memory
    } else if (nmh->code == LVN_ODFINDITEM) {
        // not handled - no finding in results list for now
    }
    return 0;
}

BOOL MainWindow::createMainWindow(HINSTANCE hInstance, int nCmdShow,
                                  TCHAR const* class_name, TCHAR const* window_title)
{
    m_hInstance = hInstance;
    m_windowTitle = window_title;
    m_hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU1));
    if (!m_hMenu) {
        MessageBox(nullptr, TEXT("Error creating menu"), window_title, MB_ICONERROR);
        return FALSE;
    }
    EnableMenuItem(m_hMenu, ID_OPTIONS_UPDATEDB, MF_BYCOMMAND | MF_DISABLED);
    if (quicker_sfv::supportsAvx512()) {
        MENUITEMINFO mii{ .cbSize = sizeof(MENUITEMINFO), .fMask = MIIM_STATE, .fState = MFS_ENABLED | MFS_CHECKED };
        SetMenuItemInfo(m_hMenu, ID_OPTIONS_USEAVX512, FALSE, &mii);
    }

    static_assert((sizeof(MainWindow*) == sizeof(LPARAM)) && (sizeof(MainWindow*) == sizeof(LPVOID)));
    m_hWnd = CreateWindow(
        class_name,
        m_windowTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        400,
        256,
        nullptr,
        m_hMenu,
        hInstance,
        std::bit_cast<LPVOID>(this)
    );
    if (!m_hWnd) {
        MessageBox(nullptr, TEXT("Error creating main window"), m_windowTitle, MB_ICONERROR);
        DestroyMenu(m_hMenu);
        m_hMenu = nullptr;
        return FALSE;
    }

    SetWindowLongPtr(m_hWnd, 0, std::bit_cast<LONG_PTR>(this));

    ShowWindow(m_hWnd, nCmdShow);
    if (!UpdateWindow(m_hWnd)) {
        MessageBox(nullptr, TEXT("Error updating main window"), m_windowTitle, MB_ICONERROR);
        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
        DestroyMenu(m_hMenu);
        m_hMenu = nullptr;
        return FALSE;
    }

    return TRUE;
}

LRESULT MainWindow::createUiElements(HWND parent_hwnd) {
    RECT parent_rect;
    if (!GetWindowRect(parent_hwnd, &parent_rect)) {
        MessageBox(nullptr, TEXT("Error retrieving window rect"), m_windowTitle, MB_ICONERROR);
        return -1;
    }
    WORD const cyChar = HIWORD(GetDialogBaseUnits());
    m_hTextFieldLeft = CreateWindow(TEXT("STATIC"),
        TEXT(""),
        WS_CHILD | SS_LEFT | WS_VISIBLE | SS_SUNKEN,
        0,
        0,
        (parent_rect.right - parent_rect.left) / 2,
        cyChar * 2,
        parent_hwnd,
        std::bit_cast<HMENU>(0x123ull),
        m_hInstance,
        0
    );
    if (!m_hTextFieldLeft) {
        MessageBox(nullptr, TEXT("Error creating window ui element"), m_windowTitle, MB_ICONERROR);
        return -1;
    }
    Static_SetText(m_hTextFieldLeft, TEXT("Completed files: 0/0\nOk: 0"));
    m_hTextFieldRight = CreateWindow(TEXT("STATIC"),
        TEXT(""),
        WS_CHILD | SS_LEFT | WS_VISIBLE | SS_SUNKEN,
        (parent_rect.right - parent_rect.left) / 2,
        0,
        (parent_rect.right - parent_rect.left) / 2,
        cyChar * 2,
        parent_hwnd,
        std::bit_cast<HMENU>(0x124ull),
        m_hInstance,
        0
    );
    if (!m_hTextFieldRight) {
        MessageBox(nullptr, TEXT("Error creating window ui element"), m_windowTitle, MB_ICONERROR);
        return -1;
    }
    Static_SetText(m_hTextFieldRight, TEXT("Bad: 0\nMissing: 0"));

    m_hListView = CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_LISTVIEW,
        TEXT("Blub"),
        WS_TABSTOP | WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_AUTOARRANGE | LVS_REPORT | LVS_OWNERDATA,
        0,
        cyChar * 2,
        parent_rect.right - parent_rect.left,
        parent_rect.bottom - cyChar * 2,
        parent_hwnd,
        std::bit_cast<HMENU>(0x125ull),
        m_hInstance,
        0
    );
    if (!m_hListView) {
        MessageBox(nullptr, TEXT("Error creating window ui element"), m_windowTitle, MB_ICONERROR);
        return -1;
    }

    TCHAR column_name1[] = TEXT("Name");
    TCHAR column_name2[] = TEXT("Checksum");
    TCHAR column_name3[] = TEXT("Status");
    struct {
        TCHAR* name;
        int width;
    } columns[] = { { column_name1, 150 }, { column_name2, 110 }, { column_name3, 105 } };
    for (int i = 0; i < 3; ++i) {
        LV_COLUMN lv_column{
            .mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
            .fmt = LVCFMT_LEFT,
            .cx = columns[i].width,
            .pszText = columns[i].name,
        };
        ListView_InsertColumn(m_hListView, i, &lv_column);
    }
    ListView_DeleteAllItems(m_hListView);

    SetWindowFont(m_hTextFieldLeft, GetWindowFont(m_hListView), TRUE);
    SetWindowFont(m_hTextFieldRight, GetWindowFont(m_hListView), TRUE);

    constexpr WORD const icon_id_list[] = { IDI_ICON_CHECKMARK, IDI_ICON_CROSS, IDI_ICON_INFO };
    constexpr int const number_of_icons = ARRAYSIZE(icon_id_list);
    int const icon_size_x = GetSystemMetrics(SM_CXSMICON);
    int const icon_size_y = GetSystemMetrics(SM_CYSMICON);
    m_imageList = ImageList_Create(icon_size_x, icon_size_y, ILC_MASK | ILC_COLORDDB, number_of_icons, 0);
    if (!m_imageList) {
        MessageBox(nullptr, TEXT("Error creating window ui element"), m_windowTitle, MB_ICONERROR);
        return -1;
    }

    for (int i = 0; i < number_of_icons; ++i) {
        HANDLE hicon = LoadImage(m_hInstance, MAKEINTRESOURCE(icon_id_list[i]), IMAGE_ICON, icon_size_x, icon_size_y, LR_DEFAULTCOLOR);
        if (!hicon) {
            MessageBox(nullptr, TEXT("Error creating window ui element"), m_windowTitle, MB_ICONERROR);
            return -1;
        }
        if (ImageList_AddIcon(m_imageList, static_cast<HICON>(hicon)) != i) {
            MessageBox(nullptr, TEXT("Error creating window ui element"), m_windowTitle, MB_ICONERROR);
            return -1;
        }
    }
    ListView_SetImageList(m_hListView, m_imageList, LVSIL_SMALL);

    return 0;
}

void MainWindow::resize() {
    RECT rect;
    GetClientRect(m_hWnd, &rect);
    LONG const new_width = rect.right - rect.left;
    WORD const textFieldHeight = HIWORD(GetDialogBaseUnits()) * 2;
    MoveWindow(m_hTextFieldLeft, 0, 0, new_width / 2, textFieldHeight, TRUE);
    MoveWindow(m_hTextFieldRight, new_width / 2, 0, new_width / 2, textFieldHeight, TRUE);
    MoveWindow(m_hListView, 0, textFieldHeight, new_width, rect.bottom - textFieldHeight, TRUE);
}

void MainWindow::onCheckStarted(uint32_t n_files) {
    ListView_DeleteAllItems(m_hListView);
    m_listEntries.clear();
    quicker_sfv::Version const v = quicker_sfv::getVersion();
    m_listEntries.push_back(ListViewEntry{ .name = formatString(50, TEXT("QuickerSfv v%d.%d.%d"), v.major, v.minor, v.patch), .checksum = {}, .status = ListViewEntry::Status::Information });
    m_stats = Stats{ .total = n_files };
    UpdateStats();
    ListView_SetItemCount(m_hListView, 1);
}

void MainWindow::onProgress(std::u8string_view file, uint32_t percentage, uint32_t bandwidth_mib_s) {
    UNREFERENCED_PARAMETER(file);
    m_stats.progress = percentage;
    m_stats.bandwidth = bandwidth_mib_s;
    UpdateStats();
}

void MainWindow::onFileCompleted(std::u8string_view file, Digest const& checksum, CompletionStatus status) {
    ++m_stats.completed;
    m_stats.progress = 0;
    m_stats.bandwidth = 0;
    switch (status) {
    case CompletionStatus::Ok:
        m_listEntries.push_back(ListViewEntry{ .name = convertToUtf16(file), .checksum = convertToUtf16(checksum.toString()), .status = ListViewEntry::Status::Ok});
        ++m_stats.ok;
        break;
    case CompletionStatus::Missing:
        m_listEntries.push_back(ListViewEntry{ .name = convertToUtf16(file), .checksum = {}, .status = ListViewEntry::Status::FailedMissing });
        ++m_stats.missing;
        break;
    case CompletionStatus::Bad:
        m_listEntries.push_back(ListViewEntry{ .name = convertToUtf16(file), .checksum = convertToUtf16(checksum.toString()), .status = ListViewEntry::Status::FailedMismatch});
        ++m_stats.bad;
        break;
    }
    ListView_SetItemCount(m_hListView, m_listEntries.size());
    ListView_EnsureVisible(m_hListView, m_listEntries.size() - 1, FALSE);
    UpdateStats();
}

void MainWindow::onCheckCompleted(Result r) {
    m_stats.ok = r.ok;
    m_stats.bad = r.bad;
    m_stats.missing = r.missing;
    m_stats.completed = r.ok + r.bad + r.missing;
    m_stats.progress = 0;
    m_stats.bandwidth = 0;
    m_listEntries.push_back(ListViewEntry{ .name = formatString(30, TEXT("%d files checked"), m_stats.completed), .checksum = {}, .status = ListViewEntry::Status::Information});
    if ((m_stats.missing == 0) && (m_stats.bad == 0)) {
        m_listEntries.push_back(ListViewEntry{ .name = u"All files OK", .checksum = {}, .status = ListViewEntry::Status::Ok });
    } 
    if (m_stats.missing > 0) {
        m_listEntries.push_back(ListViewEntry{
            .name = formatString(30, TEXT("%d file%s missing"), m_stats.missing, ((m_stats.missing == 1) ? TEXT("") : TEXT("s"))),
            .checksum = {},
            .status = ListViewEntry::Status::FailedMissing
        });
    }
    if (m_stats.bad > 0) {
        m_listEntries.push_back(ListViewEntry{
            .name = formatString(30, TEXT("%d file%s failed"), m_stats.bad, ((m_stats.bad == 1) ? TEXT("") : TEXT("s"))),
            .checksum = {},
            .status = ListViewEntry::Status::FailedMismatch
        });
    }
    ListView_SetItemCount(m_hListView, m_listEntries.size());
    ListView_EnsureVisible(m_hListView, m_listEntries.size() - 1, FALSE);
    UpdateStats();
}

void MainWindow::onCancelRequested() {
}

void MainWindow::onCanceled() {
}

void MainWindow::onError(quicker_sfv::Error error, std::u8string_view msg) {
    UNREFERENCED_PARAMETER(error);
    m_listEntries.push_back(ListViewEntry{ .name = u"ERROR: " + convertToUtf16(msg), .checksum = {}, .status = ListViewEntry::Status::Information });
    ListView_SetItemCount(m_hListView, m_listEntries.size());
}

void MainWindow::UpdateStats() {
    TCHAR buffer[50];
    constexpr size_t const buffer_size = ARRAYSIZE(buffer);
    if (m_stats.progress == 0) {
        Static_SetText(m_hTextFieldLeft, formatString(buffer, buffer_size, TEXT("Completed files: %d/%d\nOk: %d"), m_stats.completed, m_stats.total, m_stats.ok));
    } else {
        Static_SetText(m_hTextFieldLeft, formatString(buffer, buffer_size, TEXT("Completed files: %d/%d (File: %d%% %dMiB/s)\nOk: %d"), m_stats.completed, m_stats.total, m_stats.progress, m_stats.bandwidth, m_stats.ok));
    }
    Static_SetText(m_hTextFieldRight, formatString(buffer, buffer_size, TEXT("Bad: %d\nMissing: %d"), m_stats.bad, m_stats.missing));
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MainWindow* main_window_ptr = std::bit_cast<MainWindow*>(GetWindowLongPtr(hWnd, 0));
    if (!main_window_ptr) {
        if (msg == WM_CREATE) {
            CREATESTRUCT const* cs = reinterpret_cast<CREATESTRUCT const*>(lParam);
            MainWindow* main_window_create_ptr = std::bit_cast<MainWindow*>(cs->lpCreateParams);
            if (!main_window_create_ptr) { return -1; }
            return main_window_create_ptr->WndProc(hWnd, msg, wParam, lParam);
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return main_window_ptr->WndProc(hWnd, msg, wParam, lParam);
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE /* hPrevInstance */,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    enforce(GetACP() == 65001);  // utf-8 codepage

    FileProviders file_providers;

    TCHAR const class_name[] = TEXT("quicker_sfv");
    TCHAR const window_title[] = TEXT("QuickerSFV");

    WNDCLASS wndClass{
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = WndProc,
        .cbClsExtra = 0,
        .cbWndExtra = sizeof(MainWindow*),
        .hInstance = hInstance,
        .hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON_MAIN_WINDOW)),
        .hCursor = LoadCursor(nullptr, IDC_ARROW),
        .hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)),
        .lpszMenuName = nullptr,
        .lpszClassName = class_name
    };

    ATOM registered_class = RegisterClass(&wndClass);
    if (!registered_class) {
        MessageBox(nullptr, TEXT("Error registering class"), window_title, MB_ICONERROR);
        return 0;
    }

    Scheduler scheduler;
    MainWindow main_window(file_providers, scheduler);
    if (!main_window.createMainWindow(hInstance, nCmdShow, class_name, window_title)) {
        return 0;
    }

    scheduler.start();
    MSG message;
    for (;;) {
        scheduler.run();
        BOOL const bret = GetMessage(&message, nullptr, 0, 0);
        if (bret == 0) {
            break;
        } else if (bret == -1) {
            MessageBox(nullptr, TEXT("Error in GetMessage"), window_title, MB_ICONERROR);
            return 0;
        } else {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
    }
    scheduler.shutdown();
    return static_cast<int>(message.wParam);
}
