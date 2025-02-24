#include <ui/file_dialog.hpp>

#include <ui/enforce.hpp>

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

CComPtr<FileDialogEventHandler> createFileDialogEventHandler() {
    FileDialogEventHandler* p = new (std::nothrow) FileDialogEventHandler();
    if (!p) {
        return nullptr;
    }
    return p;
}

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

} // anonymous namespace

std::optional<std::u16string> FileDialog(HWND parent_window, FileDialogAction action, LPCWSTR dialog_title, std::span<COMDLG_FILTERSPEC const> filter_types) {
    CComPtr<IFileDialog> file_open_dialog = nullptr;
    CLSID const dialog_clsid = (action == FileDialogAction::SaveAs) ? CLSID_FileSaveDialog : CLSID_FileOpenDialog;
    HRESULT hres = CoCreateInstance(dialog_clsid, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&file_open_dialog));
    if (!SUCCEEDED(hres)) {
        MessageBox(nullptr, TEXT("Error creating file dialog"), TEXT("Error"), MB_ICONERROR);
        return std::nullopt;
    }
    CComPtr<gui::FileDialogEventHandler> file_dialog_event_handler = gui::createFileDialogEventHandler();
    DWORD cookie;
    file_open_dialog->Advise(file_dialog_event_handler, &cookie);
    if (!SUCCEEDED(hres)) {
        MessageBox(nullptr, TEXT("Error advising dialog event handler"), TEXT("Error"), MB_ICONERROR);
        return std::nullopt;
    }
    AdviseGuard advise_guard{ file_open_dialog, cookie };

    FILEOPENDIALOGOPTIONS opts;
    hres = file_open_dialog->GetOptions(&opts);
    if (!SUCCEEDED(hres)) {
        MessageBox(nullptr, TEXT("Error retrieving file dialog options"), TEXT("Error"), MB_ICONERROR);
        return std::nullopt;
    }
    opts |= FOS_FORCEFILESYSTEM;
    if (action == FileDialogAction::Open) {
        opts |= FOS_FILEMUSTEXIST;
    } else if (action == FileDialogAction::OpenFolder) {
        opts |= FOS_FORCEFILESYSTEM | FOS_PICKFOLDERS | FOS_DONTADDTORECENT;
    }
    hres = file_open_dialog->SetOptions(opts);
    if (!SUCCEEDED(hres)) {
        MessageBox(nullptr, TEXT("Error setting file dialog options"), TEXT("Error"), MB_ICONERROR);
        return std::nullopt;
    }
    if (action != FileDialogAction::OpenFolder) {
        enforce(filter_types.size() < std::numeric_limits<UINT>::max());
        hres = file_open_dialog->SetFileTypes(static_cast<UINT>(filter_types.size()), filter_types.data());
        if (!SUCCEEDED(hres)) {
            MessageBox(nullptr, TEXT("Error setting file dialog file types"), TEXT("Error"), MB_ICONERROR);
            return std::nullopt;
        }
        hres = file_open_dialog->SetFileTypeIndex(static_cast<UINT>(FileType::VerificationDB));
        if (!SUCCEEDED(hres)) {
            MessageBox(nullptr, TEXT("Error setting file dialog file type index"), TEXT("Error"), MB_ICONERROR);
            return std::nullopt;
        }
    }
    if (dialog_title) {
        file_open_dialog->SetTitle(dialog_title);
    }
    hres = file_open_dialog->Show(parent_window);
    if (hres == S_OK) {
        IShellItem* shell_result;
        hres = file_open_dialog->GetResult(&shell_result);
        if (hres != S_OK) {
            MessageBox(nullptr, TEXT("Error retrieving file dialog result"), TEXT("Error"), MB_ICONERROR);
            return std::nullopt;
        }
        LPWSTR filename;
        hres = shell_result->GetDisplayName(SIGDN_FILESYSPATH, &filename);  // works always because FOS_FORCEFILESYSTEM above
        if (hres != S_OK) {
            MessageBox(nullptr, TEXT("Error retrieving file dialog result display name"), TEXT("Error"), MB_ICONERROR);
            return std::nullopt;
        }
        std::u16string ret{ reinterpret_cast<char16_t const*>(filename) };
        // prepend to remove absolute path limit
        if (!ret.starts_with(u"\\\\")) {
            ret.insert(0, u"\\\\?\\");
        }
        CoTaskMemFree(filename);
        return ret;
    }
    return std::nullopt;
}

}
