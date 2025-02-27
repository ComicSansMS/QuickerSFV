#ifndef INCLUDE_GUARD_QUICKER_SFV_GUI_UI_FILE_DIALOG_HPP
#define INCLUDE_GUARD_QUICKER_SFV_GUI_UI_FILE_DIALOG_HPP

#include <shobjidl_core.h>
#include <atlbase.h>

#include <atomic>
#include <optional>
#include <span>
#include <string>

namespace quicker_sfv::gui{

class FileDialogEventHandler: public IFileDialogEvents {
private:
    std::atomic<ULONG> m_refCount;
    std::span<COMDLG_FILTERSPEC const> m_filterTypes;
private:
    ~FileDialogEventHandler();
    FileDialogEventHandler(FileDialogEventHandler const&) = delete;
    FileDialogEventHandler& operator=(FileDialogEventHandler const&) = delete;
public:
    explicit FileDialogEventHandler(std::span<COMDLG_FILTERSPEC const> filter_types);
    HRESULT STDMETHODCALLTYPE QueryInterface(IID const& iid, void** ppv) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;
    HRESULT STDMETHODCALLTYPE OnFileOk(IFileDialog* pfd) override;
    HRESULT STDMETHODCALLTYPE OnFolderChanging(IFileDialog* pfd, IShellItem* psiFolder) override;
    HRESULT STDMETHODCALLTYPE OnFolderChange(IFileDialog* pfd) override;
    HRESULT STDMETHODCALLTYPE OnSelectionChange(IFileDialog* pfd) override;
    HRESULT STDMETHODCALLTYPE OnShareViolation(IFileDialog* pfd, IShellItem* psi, FDE_SHAREVIOLATION_RESPONSE* pResponse) override;
    HRESULT STDMETHODCALLTYPE OnTypeChange(IFileDialog* pfd) override;
    HRESULT STDMETHODCALLTYPE OnOverwrite(IFileDialog* pfd, IShellItem* psi, FDE_OVERWRITE_RESPONSE* pResponse) override;
};

CComPtr<FileDialogEventHandler> createFileDialogEventHandler(std::span<COMDLG_FILTERSPEC const> filter_types);

enum class FileDialogAction {
    Open,
    OpenFolder,
    SaveAs,
};

struct FileDialogResult {
    std::u16string path;
    UINT selected_file_type;
};

std::optional<FileDialogResult> FileDialog(HWND parent_window, FileDialogAction action,
                                           LPCWSTR dialog_title, std::span<COMDLG_FILTERSPEC const> filter_types);

}

#endif
