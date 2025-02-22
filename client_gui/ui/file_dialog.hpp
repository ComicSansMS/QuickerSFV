#ifndef INCLUDE_GUARD_QUICKER_SFV_GUI_UI_FILE_DIALOG_HPP
#define INCLUDE_GUARD_QUICKER_SFV_GUI_UI_FILE_DIALOG_HPP

#include <shobjidl_core.h>
#include <atlbase.h>

#include <atomic>

namespace quicker_sfv::gui{

class FileDialogEventHandler: public IFileDialogEvents {
private:
    std::atomic<ULONG> m_refCount;
private:
    ~FileDialogEventHandler();
    FileDialogEventHandler(FileDialogEventHandler const&) = delete;
    FileDialogEventHandler& operator=(FileDialogEventHandler const&) = delete;
public:
    FileDialogEventHandler();
    HRESULT QueryInterface(IID const& iid, void** ppv) override;
    ULONG AddRef() override;
    ULONG Release() override;
    HRESULT OnFileOk(IFileDialog* pfd) override;
    HRESULT OnFolderChanging(IFileDialog* pfd, IShellItem* psiFolder) override;
    HRESULT OnFolderChange(IFileDialog* pfd) override;
    HRESULT OnSelectionChange(IFileDialog* pfd) override;
    HRESULT OnShareViolation(IFileDialog* pfd, IShellItem* psi, FDE_SHAREVIOLATION_RESPONSE* pResponse) override;
    HRESULT OnTypeChange(IFileDialog* pfd) override;
    HRESULT OnOverwrite(IFileDialog* pfd, IShellItem* psi, FDE_OVERWRITE_RESPONSE* pResponse) override;
};

CComPtr<FileDialogEventHandler> createFileDialogEventHandler();

}


#endif
