#ifndef INCLUDE_GUARD_QUICKER_SFV_GUI_UI_EVENT_HANDLER_HPP
#define INCLUDE_GUARD_QUICKER_SFV_GUI_UI_EVENT_HANDLER_HPP

#include <quicker_sfv/error.hpp>
#include <quicker_sfv/digest.hpp>

#include <cstdint>
#include <string_view>

namespace quicker_sfv::gui {

struct EventHandler {
    EventHandler& operator=(EventHandler&&) = delete;

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
    virtual void onFileStarted(std::u8string_view file, std::u8string_view absolute_file_path) = 0;
    virtual void onProgress(uint32_t percentage, uint32_t bandwidth_mib_s) = 0;
    virtual void onFileCompleted(std::u8string_view file, Digest const& checksum, std::u8string_view absolute_file_path,
        CompletionStatus status) = 0;
    virtual void onCheckCompleted(Result r) = 0;
    virtual void onCancelRequested() = 0;
    virtual void onCanceled() = 0;
    virtual void onError(quicker_sfv::Error error, std::u8string_view msg) = 0;
};

}

#endif
