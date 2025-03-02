#ifndef INCLUDE_GUARD_QUICKER_SFV_GUI_UI_OPERATION_SCHEDULER_HPP
#define INCLUDE_GUARD_QUICKER_SFV_GUI_UI_OPERATION_SCHEDULER_HPP

#include <quicker_sfv/checksum_file.hpp>
#include <quicker_sfv/checksum_provider.hpp>
#include <quicker_sfv/hasher.hpp>

#include <quicker_sfv/ui/event_handler.hpp>

#include <condition_variable>
#include <mutex>
#include <variant>
#include <vector>

#include <Windows.h>

namespace quicker_sfv::gui {

namespace Operation {
struct Create {
    ChecksumFile output;
    std::vector<std::u16string> paths_to_scan;
    std::u16string target_file_path;
};

struct Verify {
    EventHandler* event_handler;
    HasherOptions options;
    std::u16string source_file;
    ChecksumProvider* provider;
};

struct CreateFromFolder {
    EventHandler* event_handler;
    HasherOptions options;
    std::u16string target_file;
    std::u16string folder_path;
    ChecksumProvider* provider;
};

struct Cancel {
    EventHandler* event_handler;
};
} // namespace Operation

struct OperationScheduler {
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
        std::u16string folder_path;
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
            std::u8string absolute_file_path;
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
    OperationScheduler();
    ~OperationScheduler();
    OperationScheduler& operator=(OperationScheduler&&) = delete;
    void start();
    void shutdown();
    void run();
    void post(Operation::Verify op);
    void post(Operation::Cancel op);
    void post(Operation::CreateFromFolder op);
private:
    void worker();
    void doVerify(OperationState& op);
    void doCreate(OperationState& op);

    void signalCheckStarted(EventHandler* recipient, uint32_t n_files);
    void signalProgress(EventHandler* recipient, std::u8string_view file, uint32_t percentage, uint32_t bandwidth_mib_s);
    void signalFileCompleted(EventHandler* recipient, std::u8string_view file, Digest checksum,
        std::u8string absolute_file_path, EventHandler::CompletionStatus status);
    void signalCheckCompleted(EventHandler* recipient, EventHandler::Result r);
    void signalCancelRequested(EventHandler* recipient);
    void signalCanceled(EventHandler* recipient);
    void signalError(EventHandler* recipient, Error error, std::u8string_view msg);

    static void dispatchEvent(EventHandler* recipient, Event::ECheckStarted const& e);
    static void dispatchEvent(EventHandler* recipient, Event::EProgress const& e);
    static void dispatchEvent(EventHandler* recipient, Event::EFileCompleted const& e);
    static void dispatchEvent(EventHandler* recipient, Event::ECheckCompleted const& e);
    static void dispatchEvent(EventHandler* recipient, Event::ECancelRequested const& e);
    static void dispatchEvent(EventHandler* recipient, Event::ECanceled const& e);
    static void dispatchEvent(EventHandler* recipient, Event::EError const& e);
};

}

#endif
