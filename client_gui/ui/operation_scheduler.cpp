#include <ui/operation_scheduler.hpp>

#include <ui/string_helper.hpp>
#include <ui/user_messages.hpp>

#include <quicker_sfv/utf.hpp>

#include <algorithm>
#include <array>
#include <generator>
#include <numeric>

namespace quicker_sfv::gui {

namespace {
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

Digest hashFile(Hasher& hasher, LPCWSTR filepath, uint64_t filesize) {
    hasher.reset();
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
    hasher.addData(filespan);
    UnmapViewOfFile(fptr);
    CloseHandle(hmappedFile);
    CloseHandle(hin);
    return hasher.finalize();
}
} // anonymous namespace


OperationScheduler::OperationScheduler()
    :m_shutdownRequested(false), m_cancelEvent(nullptr)
{
}

OperationScheduler::~OperationScheduler() {
    if (m_worker.joinable()) {
        shutdown();
    }
    CloseHandle(m_cancelEvent);
}

void OperationScheduler::start() {
    m_startingThreadId = GetCurrentThreadId();
    m_cancelEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!m_cancelEvent) {
        throwException(Error::Failed);
    }
    m_worker = std::thread([this]() { worker(); });
}

void OperationScheduler::shutdown() {
    {
        std::scoped_lock lk(m_mtxOps);
        m_shutdownRequested = true;
    }
    m_cvOps.notify_all();
    SetEvent(m_cancelEvent);
    m_worker.join();
}

void OperationScheduler::run() {
    std::vector<Event> pending_events;
    {
        std::scoped_lock lk(m_mtxEvents);
        swap(pending_events, m_eventsQueue);
    }
    for (auto const& e : pending_events) {
        std::visit([r = e.recipient](auto&& ev) { dispatchEvent(r, ev); }, e.event);
    }
}

void OperationScheduler::post(Operation::Verify op) {
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

void OperationScheduler::post(Operation::Cancel) {
    SetEvent(m_cancelEvent);
}

void OperationScheduler::post(Operation::CreateFromFolder op) {
    std::scoped_lock lk(m_mtxOps);
    m_opsQueue.push_back(OperationState{
        .cancel_requested = false,
        .event_handler = op.event_handler,
        .checksum_provider = op.provider,
        .kind = OperationState::Op::Create,
        .checksum_file = ChecksumFile{},
        .checksum_path = op.target_file,
        .folder_path = op.folder_path,
        .hasher = op.provider->createHasher(op.options)
        });
    m_cvOps.notify_one();
}

void OperationScheduler::worker() {
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
                case OperationState::Op::Create:
                    doCreate(op);
                    break;
                }
            } catch (Exception& e) {
                signalError(op.event_handler, e.code(), e.what8());
            }
        }
    }
}

namespace {

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

} // anonymous namespace

void OperationScheduler::doVerify(OperationState& op) {
    auto const offsetLow = [](int64_t i) -> DWORD { return static_cast<DWORD>(i & 0xffffffffull); };
    auto const offsetHigh = [](int64_t i) -> DWORD { return static_cast<DWORD>((i >> 32ull) & 0xffffffffull); };

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

        std::u16string const file_path = resolvePath(op.checksum_path, f.path);
        HANDLE fin = CreateFile(toWcharStr(file_path), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_OVERLAPPED,
                                nullptr);
        if (fin == INVALID_HANDLE_VALUE) {
            // file missing
            ++result.missing;
            signalFileCompleted(op.event_handler, f.path, Digest{}, convertToUtf8(file_path), EventHandler::CompletionStatus::Missing);
            continue;
        }
        HandleGuard guard_fin(fin);

        int64_t read_offset = 0;
        int64_t bytes_hashed = 0;
        bool is_eof = false;
        bool is_canceled = false;
        bool is_error = false;
        int64_t const file_size = [&is_error, fin]() -> int64_t {
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
            signalFileCompleted(op.event_handler, f.path, std::move(digest), convertToUtf8(file_path), EventHandler::CompletionStatus::Ok);
            ++result.ok;
        } else {
            signalFileCompleted(op.event_handler, f.path, std::move(digest), convertToUtf8(file_path), EventHandler::CompletionStatus::Bad);
            ++result.bad;
        }
    }
    signalCheckCompleted(op.event_handler, result);
}

void OperationScheduler::doCreate(OperationState& op) {
    for (auto const& [abs, rel, size] : iterateFiles(op.folder_path)) {
        op.checksum_file.addEntry(convertToUtf8(rel), hashFile(*op.hasher, abs, size));
    }
    FileOutputWin32 writer(op.checksum_path);
    op.checksum_provider->serialize(writer, op.checksum_file);
}

void OperationScheduler::signalCheckStarted(EventHandler* recipient, uint32_t n_files) {
    std::scoped_lock lk(m_mtxEvents);
    m_eventsQueue.emplace_back(Event{
        .recipient = recipient,
        .event = Event::ECheckStarted {
            .n_files = n_files
        }
        });
}

void OperationScheduler::signalProgress(EventHandler* recipient, std::u8string_view file, uint32_t percentage, uint32_t bandwidth_mib_s) {
    std::scoped_lock lk(m_mtxEvents);
    m_eventsQueue.emplace_back(Event{
        .recipient = recipient,
        .event = Event::EProgress {
            .file = std::u8string(file),
            .percentage = percentage,
            .bandwidth_mib_s = bandwidth_mib_s
        }
        });
    PostThreadMessage(m_startingThreadId, WM_SCHEDULER_WAKEUP, 0, 0);
}

void OperationScheduler::signalFileCompleted(EventHandler* recipient, std::u8string_view file, Digest checksum,
    std::u8string absolute_file_path, EventHandler::CompletionStatus status) {
    std::scoped_lock lk(m_mtxEvents);
    m_eventsQueue.emplace_back(Event{
        .recipient = recipient,
        .event = Event::EFileCompleted {
            .file = std::u8string(file),
            .checksum = std::move(checksum),
            .absolute_file_path = std::move(absolute_file_path),
            .status = status,
        }
        });
    PostThreadMessage(m_startingThreadId, WM_SCHEDULER_WAKEUP, 0, 0);
}

void OperationScheduler::signalCheckCompleted(EventHandler* recipient, EventHandler::Result r) {
    std::scoped_lock lk(m_mtxEvents);
    m_eventsQueue.emplace_back(Event{
        .recipient = recipient,
        .event = Event::ECheckCompleted {
            .r = r
        }
        });
    PostThreadMessage(m_startingThreadId, WM_SCHEDULER_WAKEUP, 0, 0);
}

void OperationScheduler::signalCancelRequested(EventHandler* recipient) {
    std::scoped_lock lk(m_mtxEvents);
    m_eventsQueue.emplace_back(Event{
        .recipient = recipient,
        .event = Event::ECancelRequested {
        }
        });
    PostThreadMessage(m_startingThreadId, WM_SCHEDULER_WAKEUP, 0, 0);
}

void OperationScheduler::signalCanceled(EventHandler* recipient) {
    std::scoped_lock lk(m_mtxEvents);
    m_eventsQueue.emplace_back(Event{
        .recipient = recipient,
        .event = Event::ECanceled {
        }
        });
    PostThreadMessage(m_startingThreadId, WM_SCHEDULER_WAKEUP, 0, 0);
}

void OperationScheduler::signalError(EventHandler* recipient, quicker_sfv::Error error, std::u8string_view msg) {
    std::scoped_lock lk(m_mtxEvents);
    m_eventsQueue.emplace_back(Event{
        .recipient = recipient,
        .event = Event::EError {
            .error = error,
            .msg = std::u8string(msg)
        }
        });
    PostThreadMessage(m_startingThreadId, WM_SCHEDULER_WAKEUP, 0, 0);
}

void OperationScheduler::dispatchEvent(EventHandler* recipient, Event::ECheckStarted const& e) {
    recipient->onCheckStarted(e.n_files);
}

void OperationScheduler::dispatchEvent(EventHandler* recipient, Event::EProgress const& e) {
    recipient->onProgress(e.file, e.percentage, e.bandwidth_mib_s);
}

void OperationScheduler::dispatchEvent(EventHandler* recipient, Event::EFileCompleted const& e) {
    recipient->onFileCompleted(e.file, e.checksum, e.absolute_file_path, e.status);
}

void OperationScheduler::dispatchEvent(EventHandler* recipient, Event::ECheckCompleted const& e) {
    recipient->onCheckCompleted(e.r);
}

void OperationScheduler::dispatchEvent(EventHandler* recipient, Event::ECancelRequested const&) {
    recipient->onCancelRequested();
}

void OperationScheduler::dispatchEvent(EventHandler* recipient, Event::ECanceled const&) {
    recipient->onCanceled();
}

void OperationScheduler::dispatchEvent(EventHandler* recipient, Event::EError const& e) {
    recipient->onError(e.error, e.msg);
}


}
