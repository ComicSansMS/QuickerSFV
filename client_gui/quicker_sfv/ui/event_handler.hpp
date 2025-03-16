#ifndef INCLUDE_GUARD_QUICKER_SFV_GUI_UI_EVENT_HANDLER_HPP
#define INCLUDE_GUARD_QUICKER_SFV_GUI_UI_EVENT_HANDLER_HPP

#include <quicker_sfv/error.hpp>
#include <quicker_sfv/digest.hpp>

#include <cstdint>
#include <string_view>

/** Client support library.
 */
namespace quicker_sfv::gui {

/** Interface for events raised by an OperationScheduler.
 */
struct EventHandler {
    EventHandler& operator=(EventHandler&&) = delete;

    /** Status with which the checking of an individual file completed.
     */
    enum class CompletionStatus {
        Ok,         ///< File was verified with the expected checksum.
        Missing,    ///< File was not found or could not be opened.
        Bad,        ///< File could not be checked or checked to a wrong checksum.
    };
    /** Overall result of a verify or create operation.
     */
    struct Result {
        uint32_t total;     ///< Total number of files checked.
        uint32_t ok;        ///< Number of files that with CompletionStatus::Ok.
        uint32_t bad;       ///< Number of files that with CompletionStatus::Bad.
        uint32_t missing;   ///< Number of files that with CompletionStatus::Missing.
        bool was_canceled;  ///< True if the operation was canceled before completion.
    };

    /** A new verify or create operation for a checksum file was started.
     * @param[in] n_files The expected total number of files to be checked.
     *                    This is always 0 for a create operation and greater than 0
     *                    for a verify operation.
     *
     * @note The total number of files provided here is a hint to for UI display,
     *       but the actual number may be higher or lower.
     * @note An operation always completes with either an onOperationCompleted() or
     *       an onError() event.
     */
    virtual void onOperationStarted(uint32_t n_files) = 0;
    /** A new file is being checked as part of the current operation.
     * @param[in] file Relative path to the file as it appears in the checksum file.
     * @param[in] absolute_file_path Absolute path to the file on the filesystem.
     *
     * @note A file check always completes with an onFileCompleted() event, unless
     *       the enclosing operation is canceled or completes with onError().
     */
    virtual void onFileStarted(std::u8string_view file, std::u8string_view absolute_file_path) = 0;
    /** Signals progress of the current file checking.
     * @param[in] percentage A number between the 0 and 100 indicating the checking
     *                       progress for the current file in percent.
     * @param[in] bandwidth_mib_s The approximate bandwidth of the file checking
     *                            algorithm (file I/O + hashing) in MiB/s.
     */
    virtual void onProgress(uint32_t percentage, uint32_t bandwidth_mib_s) = 0;
    /** A file has completed checking.
     * A checking can complete in the following ways:
     *  - CompletionStatus::Ok - The file was checked successfully and in case of a 
     *    verify operation, the checksums matched. The checksum argument contains the
     *    computed Digest.
     *  - CompletionStatus::Missing - The file could not be found. This can only
     *    occur for verify operations.
     *  - CompletionStatus::Bad - If a file could not be opened or read, the checksum
     *    argument will be the empty Digest. If a file completes checking, but the
     *    checksum does not match in a verify operation, the checksum argument will
     *    contain the computed Digest for the file.
     *
     * @param[in] file Relative path to the file as it appears in the checksum file.
     *                 This is the same value that was sent in an earlier
     *                 onFileStarted() event.
     * @param[in] checksum The checksum Digest computed for the completed file. The
     *                     Digest will be empty if the checksum could not be computed.
     * @param[in] absolute_file_path Absolute path to the file on the filesystem.
     *                               This is the same value that was sent in an
     *                               earlier onFileStarted() event.
     * @param[in] status The result of the checking.
     */
    virtual void onFileCompleted(std::u8string_view file, Digest const& checksum, std::u8string_view absolute_file_path,
        CompletionStatus status) = 0;
    /** A verify or create operation has completed.
     * @param[in] r A summary of the results of the operation.
     *
     * @note An operation will always complete unless an error occurs which is
     *       signalled via onError(). An operation may have completed due to being
     *       canceled. In that case, an onCanceled() event will precede this event
     *       and the Result::was_canceled field of the argument will be set to true.
     */
    virtual void onOperationCompleted(Result r) = 0;
    /** An operation has been canceled.
     * The operation has received the cancel notification and will complete in the
     * quickest way possible. Partial results will be communicated with a subsequent
     * onOperationCompleted() event with Result::was_canceled set to true.
     */
    virtual void onCanceled() = 0;
    /** A critical error occurred that prevents the operation from continuing.
     * This event signals that one of the following occurred:
     *  - A non-I/O system API call fails.
     *  - An I/O error while reading the checksum file for a verify operation.
     *  - An I/O error while listing the contents of a directory for a
     *    create operation.
     *
     * @param[in] error The value of Exception::code().
     * @param[in] msg The value of Exception::what8().
     *
     * @note Critical errors cause the running operation to be aborted immediately.
     *       No onOperationCompleted() event will be raised.
     */
    virtual void onError(quicker_sfv::Error error, std::u8string_view msg) = 0;
};

}

#endif
