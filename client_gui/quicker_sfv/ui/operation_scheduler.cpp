/*
 *   QuickerSFV - A fast checksum verifier
 *   Copyright (C) 2025  Andreas Weis (quickersfv@andreas-weis.net)
 *
 *   This file is part of QuickerSFV.
 *
 *   QuickerSFV is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   QuickerSFV is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <quicker_sfv/ui/operation_scheduler.hpp>

#include <quicker_sfv/ui/resource_guard.hpp>
#include <quicker_sfv/ui/string_helper.hpp>
#include <quicker_sfv/ui/user_messages.hpp>

#include <quicker_sfv/string_utilities.hpp>

#include <algorithm>
#include <array>
#include <generator>
#include <numeric>

namespace quicker_sfv::gui {

static constexpr DWORD const HASH_FILE_BUFFER_SIZE = 4 << 20;

namespace {
class FileInputWin32 : public FileInput {
private:
    HANDLE m_fin;
    bool m_eof;
    std::u8string m_originalFullPath;
    std::u8string m_currentFullPath;
    std::u8string m_filename;
    std::u8string m_basePath;
public:
    FileInputWin32(std::u16string const& filename)
        :m_fin(INVALID_HANDLE_VALUE), m_eof(false), m_originalFullPath(convertToUtf8(filename))
    {
        size_t const last_separator = m_originalFullPath.rfind('\\');
        if ((last_separator == std::u8string::npos) || (last_separator >= m_originalFullPath.size() - 1)) {
            throwException(Error::Failed);
        }
        m_currentFullPath = m_originalFullPath;
        m_filename = m_originalFullPath.substr(last_separator + 1);
        m_basePath = m_originalFullPath.substr(0, last_separator + 1);
        m_fin = CreateFile(toWcharStr(filename), GENERIC_READ, FILE_SHARE_READ, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (m_fin == INVALID_HANDLE_VALUE) { quicker_sfv::throwException(Error::FileIO); }
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

    int64_t seek(int64_t offset, SeekStart seek_start) {
        LARGE_INTEGER dist;
        dist.QuadPart = offset;
        LARGE_INTEGER out;
        DWORD const move_method = [](SeekStart s) -> DWORD {
            if (s == SeekStart::CurrentPosition) {
                return FILE_CURRENT;
            } else if (s == SeekStart::FileEnd) {
                return FILE_END;
            } else {
                return FILE_BEGIN;
            }
        }(seek_start);
        if (SetFilePointerEx(m_fin, dist, &out, move_method) == 0) {
            throwException(Error::FileIO);
        }
        m_eof = (SeekStart::CurrentPosition == SeekStart::FileEnd);
        return out.QuadPart;
    }

    int64_t tell() {
        return seek(0, SeekStart::CurrentPosition);
    }

    std::u8string_view current_file() const override {
        return m_filename;
    }

    bool open(std::u8string_view new_file) override {
        std::u8string new_fileu8 = std::u8string(new_file);
        std::u8string new_file_full_path = m_basePath + new_fileu8;
        HANDLE hf = CreateFile(toWcharStr(convertToUtf16(new_file_full_path)), GENERIC_READ, FILE_SHARE_READ, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hf == INVALID_HANDLE_VALUE) { return false; }
        CloseHandle(std::exchange(m_fin, hf));
        m_currentFullPath = std::move(new_file_full_path);
        m_filename = std::move(new_fileu8);
        return true;
    }

    uint64_t file_size() override {
        LARGE_INTEGER i;
        if (!GetFileSizeEx(m_fin, &i)) { throwException(Error::FileIO); }
        return i.QuadPart;
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

    void write(std::span<std::byte const> bytes_to_write) override {
        if (bytes_to_write.size() >= std::numeric_limits<DWORD>::max()) { std::abort(); }
        DWORD bytes_written = 0;
        if (!WriteFile(m_fout, bytes_to_write.data(), static_cast<DWORD>(bytes_to_write.size()), &bytes_written, nullptr)) {
            throwException(Error::FileIO);
        }
    }
};

struct FileInfo {
    LPCWSTR absolute_path;
    std::u16string_view relative_path;
    uint64_t size;
};

std::generator<FileInfo> iterateFiles(std::u16string const& base_path) {
    struct FindGuard {
        HANDLE h;
        ~FindGuard() { FindClose(h); }
        FindGuard& operator=(FindGuard&&) = delete;
    };
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
        if (hsearch == INVALID_HANDLE_VALUE) { throwException(Error::FileIO); }
        FindGuard guard_hsearch{ hsearch };
        do {
            if (is_dot(find_data.cFileName) || is_dotdot(find_data.cFileName)) { continue; }
            std::u16string p = current_path;
            p.pop_back();   // pop '*' wildcard
            p.append(assumeUtf16(find_data.cFileName));
            uint64_t filesize = (static_cast<uint64_t>(find_data.nFileSizeHigh) << 32ull) | static_cast<uint64_t>(find_data.nFileSizeLow);
            if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                appendWildcard(p);
                directories.emplace_back(std::move(p));
            } else {
                co_yield FileInfo{ .absolute_path = toWcharStr(p), .relative_path = relativePathTo(p, base_path), .size = filesize };
            }
        } while (FindNextFile(hsearch, &find_data) != FALSE);
        if (GetLastError() != ERROR_NO_MORE_FILES) { throwException(Error::FileIO); }
    }
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
    if (!m_cancelEvent) { throwException(Error::SystemError); }
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

OperationScheduler::HashResult OperationScheduler::hashFile(EventHandler* event_handler, Hasher& hasher,
                                                            HANDLE fin, int64_t data_offset, int64_t data_size,
                                                            std::span<HashReadState, 2> read_states) {
    auto const offsetLow = [](int64_t i) -> DWORD { return static_cast<DWORD>(i & 0xffffffffull); };
    auto const offsetHigh = [](int64_t i) -> DWORD { return static_cast<DWORD>((i >> 32ull) & 0xffffffffull); };

    SlidingWindow<std::chrono::nanoseconds, 10> bandwidth_track;
    hasher.reset();

    int64_t read_offset = data_offset;
    int64_t bytes_read_pending = 0;
    int64_t bytes_hashed = 0;
    bool is_eof = false;
    bool is_canceled = false;
    bool is_error = false;

    for (auto& rs : read_states) { rs.pending = false; }
    size_t front = 0;
    size_t back = 1;
    read_states[front].overlapped = OVERLAPPED{
        .Offset = offsetLow(read_offset),
        .OffsetHigh = offsetHigh(read_offset),
        .hEvent = read_states[front].event
    };
    read_states[front].t = std::chrono::steady_clock::now();
    DWORD bytes_to_read = static_cast<DWORD>(std::min(static_cast<int64_t>(HASH_FILE_BUFFER_SIZE), data_size - bytes_read_pending));
    if (!ReadFile(fin, read_states[front].buffer.data(), bytes_to_read, nullptr, &read_states[front].overlapped)) {
        DWORD const err = GetLastError();
        if (err == ERROR_HANDLE_EOF) {
            // eof before async
            is_eof = true;
        } else if (err == ERROR_IO_PENDING) {
            read_states[front].pending = true;
        } else {
            // file read error
            is_error = true;
        }
    }
    bytes_read_pending += bytes_to_read;
    read_offset += bytes_to_read;
    uint32_t last_progress = 0;
    while (!is_eof && !is_canceled && !is_error) {
        read_states[back].overlapped = OVERLAPPED{
                .Offset = offsetLow(read_offset),
                .OffsetHigh = offsetHigh(read_offset),
                .hEvent = read_states[back].event
        };
        read_states[back].t = std::chrono::steady_clock::now();
        bytes_to_read = static_cast<DWORD>(std::min(static_cast<int64_t>(HASH_FILE_BUFFER_SIZE), data_size - bytes_read_pending));
        if (!ReadFile(fin, read_states[back].buffer.data(), HASH_FILE_BUFFER_SIZE, nullptr, &read_states[back].overlapped)) {
            DWORD const err = GetLastError();
            if (err == ERROR_HANDLE_EOF) {
                // eof before async
                is_eof = true;
            } else if (err == ERROR_IO_PENDING) {
                read_states[back].pending = true;
            } else {
                // file read error
                is_error = true;
                break;
            }
        }
        bytes_read_pending += bytes_to_read;
        read_offset += bytes_to_read;

        DWORD bytes_read = 0;
        // cancel event has to go first, or it will get starved by completing i/os
        HANDLE event_handles[] = { m_cancelEvent, read_states[front].event };
        DWORD const wait_ret = WaitForMultipleObjects(2, event_handles, FALSE, INFINITE);
        if (wait_ret == WAIT_OBJECT_0 + 1) {
            // read successful
            read_states[front].pending = false;
            if (!GetOverlappedResult(fin, &read_states[front].overlapped, &bytes_read, FALSE)) {
                if (GetLastError() == ERROR_HANDLE_EOF) {
                    // eof
                    is_eof = true;
                } else {
                    // file read error
                    is_error = true;
                    break;
                }
            } else {
                if (bytes_read == HASH_FILE_BUFFER_SIZE) { bandwidth_track.push(std::chrono::steady_clock::now() - read_states[front].t); }
            }
        } else if (wait_ret == WAIT_OBJECT_0) {
            // cancel
            CancelIo(fin);
            is_canceled = true;
            break;
        } else {
            // catastrophic error: unexpected wait result
            throwException(Error::SystemError);
        }

        hasher.addData(std::span<std::byte const>(read_states[front].buffer.data(), bytes_read));
        bytes_hashed += bytes_read;
        if (bytes_hashed == data_size) { break; }
        uint32_t current_progress = (data_size == 0) ? 0u : static_cast<uint32_t>(bytes_hashed * 100 / data_size);
        if (current_progress != last_progress) {
            int64_t const t_avg = bandwidth_track.rollingAverage().count();
            uint32_t const bandwidth_mib_s = static_cast<uint32_t>((t_avg) ? ((static_cast<int64_t>(HASH_FILE_BUFFER_SIZE) * 1'000'000'000ll) / (t_avg * 1'048'576ll)) : 0);
            signalProgress(event_handler, current_progress, bandwidth_mib_s);
            last_progress = current_progress;
        }

        std::swap(front, back);
    }
    for (auto& rs : read_states) {
        if (rs.pending) { WaitForSingleObject(rs.event, INFINITE); DWORD b; GetOverlappedResult(fin, &rs.overlapped, &b, TRUE); }
    }
    if (is_canceled) {
        return HashResult::Canceled;
    }
    if (is_error) {
        return HashResult::Error;
    }
    return HashResult::DigestReady;
}

void OperationScheduler::doVerify(OperationState& op) {
    FileInputWin32 reader(op.checksum_path);
    op.checksum_file = op.checksum_provider->readFromFile(reader);

    HANDLE event_front = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!event_front) { throwException(Error::SystemError); }
    HandleGuard guard_event_front(event_front);
    HANDLE event_back = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!event_back) { throwException(Error::SystemError); }
    HandleGuard guard_event_back(event_back);

    HashReadState read_states[] = {
        HashReadState{
            .buffer = std::vector<std::byte>(HASH_FILE_BUFFER_SIZE),
            .event = event_front,
            .overlapped = {},
            .pending = false,
            .t = {}
        },
        HashReadState{
            .buffer = std::vector<std::byte>(HASH_FILE_BUFFER_SIZE),
            .event = event_back,
            .overlapped = {},
            .pending = false,
            .t = {}
        },
    };

    EventHandler::Result result{};
    result.total = static_cast<uint32_t>(op.checksum_file.getEntries().size());
    signalOperationStarted(op.event_handler, result.total);

    for (auto const& f : op.checksum_file.getEntries()) {
        std::u16string const absolute_file_path = resolvePath(op.checksum_path, f.data.front().path);
        std::u8string const utf8_absolute_file_path = convertToUtf8(absolute_file_path);
        signalFileStarted(op.event_handler, f.display, utf8_absolute_file_path);
        HANDLE fin = CreateFile(toWcharStr(absolute_file_path), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_OVERLAPPED, nullptr);
        if (fin == INVALID_HANDLE_VALUE) {
            if (GetLastError() == ERROR_FILE_NOT_FOUND) {
                signalFileCompleted(op.event_handler, f.display, Digest{}, utf8_absolute_file_path,
                                    EventHandler::CompletionStatus::Missing);
                ++result.missing;
            } else {
                signalFileCompleted(op.event_handler, f.display, Digest{}, utf8_absolute_file_path,
                                    EventHandler::CompletionStatus::Bad);
                ++result.bad;
            }
            continue;
        }
        HandleGuard guard_fin(fin);
        int64_t file_size = f.data.front().data_size;
        if (file_size == -1) {
            LARGE_INTEGER l_file_size;
            if (GetFileSizeEx(fin, &l_file_size)) {
                file_size = l_file_size.QuadPart;
            }
        }
        HashResult const res = (file_size != -1) ?
            hashFile(op.event_handler, *op.hasher, fin, f.data.front().data_offset, file_size, read_states) :
            HashResult::Error;
        if (res == HashResult::DigestReady) {
            auto digest = op.hasher->finalize();
            if (digest == f.digest) {
                signalFileCompleted(op.event_handler, f.display, std::move(digest), utf8_absolute_file_path,
                                    EventHandler::CompletionStatus::Ok);
                ++result.ok;
            } else {
                signalFileCompleted(op.event_handler, f.display, std::move(digest), utf8_absolute_file_path,
                                    EventHandler::CompletionStatus::Bad);
                ++result.bad;
            }
        } else if (res == HashResult::Error) {
            signalFileCompleted(op.event_handler, f.display, Digest{}, utf8_absolute_file_path,
                                EventHandler::CompletionStatus::Bad);
            return;
        } else if (res == HashResult::Canceled) {
            signalCanceled(op.event_handler);
            result.was_canceled = true;
            break;
        }
    }
    signalOperationCompleted(op.event_handler, result);
}

void OperationScheduler::doCreate(OperationState& op) {
    HANDLE event_front = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!event_front) { throwException(Error::SystemError); }
    HandleGuard guard_event_front(event_front);
    HANDLE event_back = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!event_back) { throwException(Error::SystemError); }
    HandleGuard guard_event_back(event_back);

    HashReadState read_states[] = {
        HashReadState{
            .buffer = std::vector<std::byte>(HASH_FILE_BUFFER_SIZE),
            .event = event_front,
            .overlapped = {},
            .pending = false,
            .t = {}
        },
        HashReadState{
            .buffer = std::vector<std::byte>(HASH_FILE_BUFFER_SIZE),
            .event = event_back,
            .overlapped = {},
            .pending = false,
            .t = {}
        },
    };

    signalOperationStarted(op.event_handler, 0);
    EventHandler::Result result = {};
    for (auto const& [absolute_path, relative_path, size] : iterateFiles(op.folder_path)) {
        std::u8string const utf8_relative_path = convertToUtf8(relative_path);
        std::u8string const utf8_absolute_path = convertToUtf8(assumeUtf16(absolute_path));
        signalFileStarted(op.event_handler, utf8_relative_path, utf8_absolute_path);
        HANDLE fin = CreateFile(absolute_path, GENERIC_READ, FILE_SHARE_READ, nullptr,
                                OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_OVERLAPPED, nullptr);
        if (fin == INVALID_HANDLE_VALUE) {
            signalFileCompleted(op.event_handler, utf8_relative_path, Digest{}, utf8_absolute_path,
                                EventHandler::CompletionStatus::Bad);
            ++result.bad;
            continue;
        }
        HandleGuard guard_fin(fin);
        LARGE_INTEGER l_file_size;
        HashResult const res = (GetFileSizeEx(fin, &l_file_size)) ?
            hashFile(op.event_handler, *op.hasher, fin, 0, l_file_size.QuadPart, read_states) :
            HashResult::Error;
        if (res == HashResult::DigestReady) {
            Digest d = op.hasher->finalize();
            signalFileCompleted(op.event_handler, utf8_relative_path, d, utf8_absolute_path,
                                EventHandler::CompletionStatus::Ok);
            op.checksum_file.addEntry(utf8_relative_path, std::move(d));
            ++result.ok;
        } else if (res == HashResult::Error) {
            signalFileCompleted(op.event_handler, utf8_relative_path, {}, utf8_absolute_path,
                                EventHandler::CompletionStatus::Bad);
            ++result.bad;
        } else if (res == HashResult::Canceled) {
            signalCanceled(op.event_handler);
            result.was_canceled = true;
            return;
        }
        ++result.total;
    }
    FileOutputWin32 writer(op.checksum_path);
    op.checksum_provider->writeNewFile(writer, op.checksum_file);
    signalOperationCompleted(op.event_handler, result);
}

void OperationScheduler::signalOperationStarted(EventHandler* recipient, uint32_t n_files) {
    std::scoped_lock lk(m_mtxEvents);
    m_eventsQueue.emplace_back(Event{
        .recipient = recipient,
        .event = Event::EOperationStarted {
            .n_files = n_files
        }
    });
}

void OperationScheduler::signalFileStarted(EventHandler* recipient, std::u8string file, std::u8string absolute_file_path) {
    std::scoped_lock lk(m_mtxEvents);
    m_eventsQueue.emplace_back(Event{
        .recipient = recipient,
        .event = Event::EFileStarted {
            .file = std::move(file),
            .absolute_file_path = std::move(absolute_file_path),
        }
    });
    PostThreadMessage(m_startingThreadId, WM_SCHEDULER_WAKEUP, 0, 0);
}

void OperationScheduler::signalProgress(EventHandler* recipient, uint32_t percentage, uint32_t bandwidth_mib_s) {
    std::scoped_lock lk(m_mtxEvents);
    m_eventsQueue.emplace_back(Event{
        .recipient = recipient,
        .event = Event::EProgress {
            .percentage = percentage,
            .bandwidth_mib_s = bandwidth_mib_s
        }
    });
    PostThreadMessage(m_startingThreadId, WM_SCHEDULER_WAKEUP, 0, 0);
}

void OperationScheduler::signalFileCompleted(EventHandler* recipient, std::u8string file, Digest checksum,
                                             std::u8string absolute_file_path, EventHandler::CompletionStatus status) {
    std::scoped_lock lk(m_mtxEvents);
    m_eventsQueue.emplace_back(Event{
        .recipient = recipient,
        .event = Event::EFileCompleted {
            .file = std::move(file),
            .checksum = std::move(checksum),
            .absolute_file_path = std::move(absolute_file_path),
            .status = status,
        }
    });
    PostThreadMessage(m_startingThreadId, WM_SCHEDULER_WAKEUP, 0, 0);
}

void OperationScheduler::signalOperationCompleted(EventHandler* recipient, EventHandler::Result r) {
    std::scoped_lock lk(m_mtxEvents);
    m_eventsQueue.emplace_back(Event{
        .recipient = recipient,
        .event = Event::EOperationCompleted {
            .r = r
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

void OperationScheduler::dispatchEvent(EventHandler* recipient, Event::EOperationStarted const& e) {
    recipient->onOperationStarted(e.n_files);
}

void OperationScheduler::dispatchEvent(EventHandler* recipient, Event::EFileStarted const& e) {
    recipient->onFileStarted(e.file, e.absolute_file_path);
}

void OperationScheduler::dispatchEvent(EventHandler* recipient, Event::EProgress const& e) {
    recipient->onProgress(e.percentage, e.bandwidth_mib_s);
}

void OperationScheduler::dispatchEvent(EventHandler* recipient, Event::EFileCompleted const& e) {
    recipient->onFileCompleted(e.file, e.checksum, e.absolute_file_path, e.status);
}

void OperationScheduler::dispatchEvent(EventHandler* recipient, Event::EOperationCompleted const& e) {
    recipient->onOperationCompleted(e.r);
}

void OperationScheduler::dispatchEvent(EventHandler* recipient, Event::ECanceled const&) {
    recipient->onCanceled();
}

void OperationScheduler::dispatchEvent(EventHandler* recipient, Event::EError const& e) {
    recipient->onError(e.error, e.msg);
}

}
