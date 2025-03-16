#ifndef INCLUDE_GUARD_QUICKER_SFV_GUI_UI_OPERATION_SCHEDULER_HPP
#define INCLUDE_GUARD_QUICKER_SFV_GUI_UI_OPERATION_SCHEDULER_HPP

#include <quicker_sfv/checksum_file.hpp>
#include <quicker_sfv/checksum_provider.hpp>
#include <quicker_sfv/hasher.hpp>

#include <quicker_sfv/ui/event_handler.hpp>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <span>
#include <variant>
#include <vector>

#include <Windows.h>

namespace quicker_sfv::gui {

/** Operations that can be dispatched to an OperationScheduler.
 */
namespace Operation {

/** Verify operation.
 * This operation verifies an existing checksum file on disk.
 */
struct Verify {
    EventHandler* event_handler;
    HasherOptions options;
    std::u16string source_file;
    ChecksumProvider* provider;
};

/** Create from folder operation.
 * This operation creates a new checksum file by checking all files in a folder and
 * all of its subfolders.
 */
struct CreateFromFolder {
    EventHandler* event_handler;
    HasherOptions options;
    std::u16string target_file;
    std::u16string folder_path;
    ChecksumProvider* provider;
};

/** Cancel the currently running operation.
 */
struct Cancel {
    EventHandler* event_handler;
};
} // namespace Operation

/** Operation Scheduler.
 * This class manages the worker thread for performing an Operation and handles all
 * communication with the UI thread.
 * Operations can be posted to the OperationScheduler to be carried out on the worker
 * thread.
 * The OperationScheduler is constructed in an inactive state. Calling start() will
 * spawn the worker thread. Operations may be posted before calling start(), but since
 * all work is carried out on the worker thread, no results will become available
 * before calling start(). Calling shutdown() will cancel all pending work and join
 * the worker thread. This will happen implicitly on destruction if the worker thread
 * is active. The worker thread can not be restarted after shutdown.
 * The OperationScheduler will report progress about each operation to the associated
 * EventHandler for that Operation. Notifications will be posted to a queue and will
 * only result in calls to an EventHandler when calling run() and will happen on the
 * same thread that is calling run().
 */
struct OperationScheduler {
private:
    /** Description of an Operation to be carried out on the worker thread.
     */
    struct OperationState {
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
    std::vector<OperationState> m_opsQueue;     ///< Queue of posted Operations.
    std::mutex m_mtxOps;
    std::condition_variable m_cvOps;
    bool m_shutdownRequested;
    HANDLE m_cancelEvent;

    /** Description of an event to be dispatched to an EventHandler.
     */
    struct Event {
        struct EOperationStarted {
            uint32_t n_files;
        };
        struct EFileStarted {
            std::u8string file;
            std::u8string absolute_file_path;
        };
        struct EProgress {
            uint32_t percentage;
            uint32_t bandwidth_mib_s;
        };
        struct EFileCompleted {
            std::u8string file;
            Digest checksum;
            std::u8string absolute_file_path;
            EventHandler::CompletionStatus status;
        };
        struct EOperationCompleted {
            EventHandler::Result r;
        };
        struct ECanceled {
        };
        struct EError {
            Error error;
            std::u8string msg;
        };
        EventHandler* recipient;
        std::variant<EOperationStarted, EFileStarted, EProgress, EFileCompleted,
                     EOperationCompleted, ECanceled, EError> event;
    };
    std::vector<Event> m_eventsQueue;           ///< Queue of outstanding events.
    std::mutex m_mtxEvents;

    std::thread m_worker;
    DWORD m_startingThreadId;
public:
    /** Constructor.
     * OperationScheduler is constructed in an inactive state. A call to start() is
     * required to spawn the worker thread and allow processing of posted Operations.
     */
    OperationScheduler();
    /** Destructor.
     * If the OperationScheduler has not been shut down manually, this will call
     * shutdown() to join the worker thread.
     */
    ~OperationScheduler();
    OperationScheduler& operator=(OperationScheduler&&) = delete;
    /** Starts the worker thread.
     * @pre Neither start() nor shutdown() have been called before on this object.
     */
    void start();
    /** Cancels all outstanding work and joins the worker thread.
     * @pre The object has previously been started by calling start() and has not
     *      already been shut down by calling shutdown().
     */
    void shutdown();
    /** Processes all outstanding event messages and invokes EventHandler.
     * Each Operation has an associated EventHandler. If the execution of the
     * operation on the worker thread generates any events, they will get posted
     * to a queue, where they remain until run() is called. run() will process all
     * items from the queue and invoke the corresponding callback on the respective
     * EventHandler. All callbacks happen on the same thread that is calling run()
     * and will be completed before run() returns.
     */
    void run();

    /** @name Functions for posting Operations.
     * @{
     */
    /** Post a verify operation.
     */
    void post(Operation::Verify op);
    /** Post a cancel operation.
     */
    void post(Operation::Cancel op);
    /** Post a create operation.
     */
    void post(Operation::CreateFromFolder op);
    /// @}
private:
    /** Main function for the worker thread.
     */
    void worker();
    /** Carries out a verify operation.
     */
    void doVerify(OperationState& op);
    /** Carries out a create operation.
     */
    void doCreate(OperationState& op);

    enum class HashResult {
        DigestReady,        ///< A checksum Digest was computed successfully.
        Canceled,           ///< The computation was canceled.
        Error,              ///< The computation failed due to an error.
    };
    struct HashReadState {
        std::vector<std::byte> buffer;              ///< Buffer for file I/O.
        HANDLE event;                               ///< Event for async file I/O.
        OVERLAPPED overlapped;                      ///< Async file I/O.
        bool pending;                               ///< The state has pending
                                                    ///  async operations.
        std::chrono::steady_clock::time_point t;    ///< Time point for performance
                                                    ///  measurements.
    };
    /** Computes the checksum for a single file.
     * @param[in] event_handler EventHandler associated with the operation which this
     *                          hashing is part of.
     * @param[in] hasher Hasher to carry out the computation of the checksum.
     * @param[in] fin An opened Win32 file handle to the file that is to be hashed.
     * @param[in] read_states Double-buffered structs containing the state for
     *                        carrying out the hashing. The sole reason this is
     *                        included here is to allow reusing the same state for
     *                        all hashings carried out by a single Operation as a
     *                        performance optimisation.
     */
    HashResult hashFile(EventHandler* event_handler, Hasher& hasher,
                        HANDLE fin, std::span<HashReadState, 2> read_states);

    /** @name Functions for posting events to the event queue.
     * These must only be called from the worker thread.
     * @{
     */
    void signalOperationStarted(EventHandler* recipient, uint32_t n_files);
    void signalFileStarted(EventHandler* recipient, std::u8string file, std::u8string absolute_file_path);
    void signalProgress(EventHandler* recipient, uint32_t percentage, uint32_t bandwidth_mib_s);
    void signalFileCompleted(EventHandler* recipient, std::u8string file, Digest checksum,
        std::u8string absolute_file_path, EventHandler::CompletionStatus status);
    void signalOperationCompleted(EventHandler* recipient, EventHandler::Result r);
    void signalCanceled(EventHandler* recipient);
    void signalError(EventHandler* recipient, Error error, std::u8string_view msg);
    /// @}

    /** @name Functions for dispatching events to their EventHandler.
     * These must only be called by run().
     * @{
     */
    static void dispatchEvent(EventHandler* recipient, Event::EOperationStarted const& e);
    static void dispatchEvent(EventHandler* recipient, Event::EFileStarted const& e);
    static void dispatchEvent(EventHandler* recipient, Event::EProgress const& e);
    static void dispatchEvent(EventHandler* recipient, Event::EFileCompleted const& e);
    static void dispatchEvent(EventHandler* recipient, Event::EOperationCompleted const& e);
    static void dispatchEvent(EventHandler* recipient, Event::ECanceled const& e);
    static void dispatchEvent(EventHandler* recipient, Event::EError const& e);
    /// @}
};

}

#endif
