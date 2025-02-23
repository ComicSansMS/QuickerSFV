#include <ui/file_dialog.hpp>

#include <cassert>

namespace quicker_sfv::gui {

FileDialogEventHandler::FileDialogEventHandler()
    :m_refCount(0)
{}

FileDialogEventHandler::~FileDialogEventHandler()
{
}

HRESULT FileDialogEventHandler::QueryInterface(IID const& iid, void** ppv) {
    if (iid == IID_IFileDialogEvents) {
        IFileDialogEvents* p = this;
        *ppv = p;
        p->AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

ULONG FileDialogEventHandler::AddRef() {
    ULONG prev = m_refCount.fetch_add(1);
    return prev + 1;
}


ULONG FileDialogEventHandler::Release() {
    ULONG prev = m_refCount.fetch_sub(1);
    assert(prev != 0);
    if (prev == 1) {
        delete this;
    }
    return prev - 1;
}

HRESULT FileDialogEventHandler::OnFileOk(IFileDialog* pfd) {
    UNREFERENCED_PARAMETER(pfd);
    return S_OK;
}

HRESULT FileDialogEventHandler::OnFolderChanging(IFileDialog* pfd, IShellItem* psiFolder) {
    UNREFERENCED_PARAMETER(pfd);
    UNREFERENCED_PARAMETER(psiFolder);
    return S_OK;
}

HRESULT FileDialogEventHandler::OnFolderChange(IFileDialog* pfd) {
    UNREFERENCED_PARAMETER(pfd);
    return S_OK;
}

HRESULT FileDialogEventHandler::OnSelectionChange(IFileDialog* pfd) {
    UNREFERENCED_PARAMETER(pfd);
    return S_OK;
}

HRESULT FileDialogEventHandler::OnShareViolation(IFileDialog* pfd, IShellItem* psi, FDE_SHAREVIOLATION_RESPONSE* pResponse) {
    UNREFERENCED_PARAMETER(pfd);
    UNREFERENCED_PARAMETER(psi);
    UNREFERENCED_PARAMETER(pResponse);
    return S_OK;
}

HRESULT FileDialogEventHandler::OnTypeChange(IFileDialog* pfd) {
    UNREFERENCED_PARAMETER(pfd);
    return S_OK;
}

HRESULT FileDialogEventHandler::OnOverwrite(IFileDialog* pfd, IShellItem* psi, FDE_OVERWRITE_RESPONSE* pResponse) {
    UNREFERENCED_PARAMETER(pfd);
    UNREFERENCED_PARAMETER(psi);
    UNREFERENCED_PARAMETER(pResponse);
    return S_OK;
}

CComPtr<FileDialogEventHandler> createFileDialogEventHandler() {
    FileDialogEventHandler* p = new (std::nothrow) FileDialogEventHandler();
    if (!p) {
        return nullptr;
    }
    return p;
}

}
