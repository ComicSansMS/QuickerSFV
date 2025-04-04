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
#include <quicker_sfv/ui/file_dialog.hpp>

#include <quicker_sfv/ui/enforce.hpp>

#include <quicker_sfv/error.hpp>

namespace quicker_sfv::gui {

namespace {

class AdviseGuard {
private:
    IFileDialog* m_file_dialog;
    DWORD m_cookie;
public:
    explicit AdviseGuard(IFileDialog* file_dialog, DWORD cookie)
        :m_file_dialog(file_dialog), m_cookie(cookie)
    {
    }
    ~AdviseGuard() {
        m_file_dialog->Unadvise(m_cookie);
    }
    AdviseGuard(AdviseGuard const&) = delete;
    AdviseGuard& operator=(AdviseGuard const&) = delete;
};

std::u16string defaultExtension(COMDLG_FILTERSPEC const& fs) {
    std::u16string_view fs_view(reinterpret_cast<char16_t const*>(fs.pszSpec));
    auto first_dot = fs_view.find(u'.');
    auto first_semicolon = fs_view.find(u';');
    if ((first_dot == std::u16string_view::npos) ||
        (first_dot + 1 >= first_semicolon) ||
        (first_dot + 2 >= first_semicolon) ||
        (fs_view[first_dot + 1] == u'*'))
    {
        return {};
    }
    return std::u16string{ fs_view.substr(first_dot + 1, first_semicolon - first_dot - 1) };
}

} // anonymous namespace

FileDialogEventHandler::FileDialogEventHandler(std::span<COMDLG_FILTERSPEC const> filter_types)
    :m_refCount(0), m_filterTypes(filter_types)
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
    enforce(prev != 0);
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

CComPtr<FileDialogEventHandler> createFileDialogEventHandler(std::span<COMDLG_FILTERSPEC const> filter_types) {
    FileDialogEventHandler* p = new FileDialogEventHandler(filter_types);
    return p;
}

std::optional<FileDialogResult> FileDialog(HWND parent_window, FileDialogAction action, LPCWSTR dialog_title, std::span<COMDLG_FILTERSPEC const> filter_types) {
    CComPtr<IFileDialog> file_dialog = nullptr;
    CLSID const dialog_clsid = (action == FileDialogAction::SaveAs) ? CLSID_FileSaveDialog : CLSID_FileOpenDialog;
    HRESULT hres = CoCreateInstance(dialog_clsid, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&file_dialog));
    if (!SUCCEEDED(hres)) { throwException(Error::SystemError); }
    CComPtr<gui::FileDialogEventHandler> file_dialog_event_handler = gui::createFileDialogEventHandler(filter_types);
    DWORD cookie;
    file_dialog->Advise(file_dialog_event_handler, &cookie);
    if (!SUCCEEDED(hres)) { throwException(Error::SystemError); }
    AdviseGuard advise_guard{ file_dialog, cookie };

    FILEOPENDIALOGOPTIONS opts;
    hres = file_dialog->GetOptions(&opts);
    if (!SUCCEEDED(hres)) { throwException(Error::SystemError); }
    opts |= FOS_FORCEFILESYSTEM;
    if (action == FileDialogAction::Open) {
        opts |= FOS_FILEMUSTEXIST;
    } else if (action == FileDialogAction::OpenFolder) {
        opts |= FOS_FORCEFILESYSTEM | FOS_PICKFOLDERS | FOS_DONTADDTORECENT;
    }
    hres = file_dialog->SetOptions(opts);
    if (!SUCCEEDED(hres)) { throwException(Error::SystemError); }
    if (action != FileDialogAction::OpenFolder) {
        enforce(filter_types.size() < std::numeric_limits<UINT>::max());
        hres = file_dialog->SetFileTypes(static_cast<UINT>(filter_types.size()), filter_types.data());
        if (!SUCCEEDED(hres)) { throwException(Error::SystemError); }
        hres = file_dialog->SetFileTypeIndex(1);
        if (!SUCCEEDED(hres)) { throwException(Error::SystemError); }
        auto ext = defaultExtension(filter_types[0]);
        if (!ext.empty()) {
            hres = file_dialog->SetDefaultExtension((LPCTSTR)ext.c_str());
            if (!SUCCEEDED(hres)) { throwException(Error::SystemError); }
        }
    }
    if (dialog_title) {
        file_dialog->SetTitle(dialog_title);
    }
    hres = file_dialog->Show(parent_window);
    if (hres == S_OK) {
        UINT file_type_index;
        hres = file_dialog->GetFileTypeIndex(&file_type_index);
        if (hres != S_OK) { throwException(Error::SystemError); }
        IShellItem* shell_result;
        hres = file_dialog->GetResult(&shell_result);
        if (hres != S_OK) { throwException(Error::SystemError); }
        LPWSTR filename;
        hres = shell_result->GetDisplayName(SIGDN_FILESYSPATH, &filename);  // works always because FOS_FORCEFILESYSTEM above
        if (hres != S_OK) { throwException(Error::SystemError); }
        std::u16string ret{ reinterpret_cast<char16_t const*>(filename) };
        // prepend to remove absolute path limit
        if (!ret.starts_with(u"\\\\")) {
            ret.insert(0, u"\\\\?\\");
        }
        CoTaskMemFree(filename);
        return FileDialogResult{ .path = ret, .selected_file_type = file_type_index - 1 };
    }
    return std::nullopt;
}

}
