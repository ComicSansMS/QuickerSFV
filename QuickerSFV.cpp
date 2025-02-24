/*
    QuickerSFV - A fast checksum checker
    Copyright (C) 2025  Andreas Weis (der_ghulbus@ghulbus-inc.de)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <quicker_sfv/quicker_sfv.hpp>

#include <ui/file_dialog.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#   define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#   define NOMINMAX
#endif
#include <Windows.h>
#include <windowsx.h>
#include <atlbase.h>
#include <CommCtrl.h>
#include <ShObjIdl_core.h>
#include <tchar.h>
#include <uxtheme.h>

#include <resource.h>

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstring>
#include <generator>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

using namespace quicker_sfv;

class FileInputWin32 : public FileInput {
private:
    HANDLE m_fin;
    bool m_eof;
public:
    FileInputWin32(wchar_t const* filename)
        :m_eof(false)
    {
        m_fin = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (m_fin == INVALID_HANDLE_VALUE) {
            std::abort();
        }
    }

    ~FileInputWin32() override {
        CloseHandle(m_fin);
    }

    size_t read(std::span<std::byte> read_buffer) override {
        if (read_buffer.size() >= std::numeric_limits<DWORD>::max()) { std::abort(); }
        if (m_eof) { return FileInput::RESULT_END_OF_FILE; }
        DWORD bytes_read = 0;
        if (!ReadFile(m_fin, read_buffer.data(), static_cast<DWORD>(read_buffer.size()), &bytes_read, nullptr)) {
            std::abort();
        }
        if (bytes_read == 0) {
            m_eof = true;
            return FileInput::RESULT_END_OF_FILE;
        }
        return bytes_read;
    }
};


class FileOutputWin32 : public FileOutput {
private:
    HANDLE m_fout;
public:
    FileOutputWin32(wchar_t const* filename)
    {
        m_fout = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (m_fout == INVALID_HANDLE_VALUE) {
            std::abort();
        }
    }

    ~FileOutputWin32() override {
        CloseHandle(m_fout);
    }

    size_t write(std::span<std::byte const> bytes_to_write) override {
        if (bytes_to_write.size() >= std::numeric_limits<DWORD>::max()) { std::abort(); }
        DWORD bytes_written = 0;
        if (!WriteFile(m_fout, bytes_to_write.data(), static_cast<DWORD>(bytes_to_write.size()), &bytes_written, nullptr)) {
            return 0;
        }
        return bytes_written;
    }
};

std::u8string toUtf8(LPCWSTR z_str) {
    return quicker_sfv::convertToUtf8(std::u16string_view(reinterpret_cast<char16_t const*>(z_str)));
}

class FileProviders {
private:
    std::vector<quicker_sfv::ChecksumFilePtr> m_files;
public:
    FileProviders()
    {
        m_files.emplace_back(quicker_sfv::createSfvFile());
        m_files.emplace_back(quicker_sfv::createMD5File());
        // @todo:
        //  - .ckz
        //  - .par
        //  - .csv
        //  - .txt
    }

    ChecksumFile* getMatchingProviderFor(std::u8string_view filename) {
        for (auto const& f: m_files) {
            std::u8string_view exts = f->fileExtensions();
            // split extensions
            
            for (std::u8string_view::size_type it = 0; it != std::u8string_view::npos;) {
                auto const it_end = exts.find(u8';');
                auto ext = exts.substr(it, it_end);
                if (ext[0] == u8'*') {
                    ext = ext.substr(1);
                    if (filename.ends_with(ext)) {
                        return f.get();
                    }
                } else {
                    assert(!"Invalid extension provided");
                }
                it = it_end;
            }
        }
        return nullptr;
    }
};

class MainWindow {
private:
    HINSTANCE m_hInstance;
    TCHAR const* m_windowTitle;
    HWND m_hWnd;
    HMENU m_hMenu;
    HWND m_hTextFieldLeft;
    HWND m_hTextFieldRight;
    HWND m_hListView;
    struct ListViewEntry {
        std::unique_ptr<TCHAR[]> name;
        std::unique_ptr<TCHAR[]> crc;
        bool status_ok;
    };
    std::vector<ListViewEntry> m_listEntries;
    HIMAGELIST m_imageList;

    FileProviders* m_fileProviders;
    ChecksumFile* m_checksumFile;
public:
    explicit MainWindow(FileProviders& file_providers);

    MainWindow(MainWindow const&) = delete;
    MainWindow(MainWindow&&) = delete;
    MainWindow& operator=(MainWindow const&) = delete;
    MainWindow& operator=(MainWindow&&) = delete;

    BOOL createMainWindow(HINSTANCE hInstance, int nCmdShow,
                          TCHAR const* class_name, TCHAR const* window_title);

    LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    LRESULT createUiElements(HWND parent_hwnd);

    LRESULT populateListView(NMHDR* nmh);

    void resize();
};

MainWindow::MainWindow(FileProviders& file_providers)
    :m_hInstance(nullptr), m_windowTitle(nullptr), m_hWnd(nullptr), m_hTextFieldLeft(nullptr), m_hTextFieldRight(nullptr),
     m_hListView(nullptr), m_imageList(nullptr), m_fileProviders(&file_providers), m_checksumFile(nullptr)
{
}

class AdviseGuard {
private:
    IFileDialog* m_file_dialog;
    DWORD m_cookie;
public:
    explicit AdviseGuard(IFileDialog* file_dialog, DWORD cookie) 
        :m_file_dialog(file_dialog), m_cookie(cookie)
    { }
    ~AdviseGuard() {
        m_file_dialog->Unadvise(m_cookie);
    }
    AdviseGuard(AdviseGuard const&) = delete;
    AdviseGuard& operator=(AdviseGuard const&) = delete;
};

class U16Path {
private:
    std::vector<WCHAR> m_path;
public:
    explicit U16Path(LPCWSTR zero_terminated_string)
    {
        auto const len = wcslen(zero_terminated_string);
        m_path.assign(zero_terminated_string, zero_terminated_string + len);
        m_path.push_back(L'\0');
        
    }

    LPCWSTR str() const noexcept {
        return m_path.data();
    }

    void prependAbsolutePathToRemoveMaxPathLimit() {
        if ((m_path.size() < 4) || (m_path[0] != L'\\') || (m_path[1] != L'\\')) {
            m_path.insert(m_path.begin(), { L'\\', L'\\', L'?', L'\\' });
        }
    }

    void append(LPCWSTR path) {
        if (!m_path.empty()) { m_path.pop_back(); }
        if (!m_path.empty() && m_path.back() == L'*') { m_path.pop_back(); }
        if (!m_path.empty() && m_path.back() == L'\\') { m_path.pop_back(); }
        m_path.push_back(L'\\');
        m_path.insert(m_path.end(), path, path + wcslen(path));
        m_path.push_back(L'\0');
    }

    U16Path relativeTo(U16Path const& parent_path) const {
        U16Path ret{ *this };
        auto it = std::mismatch(ret.m_path.begin(), ret.m_path.end(), parent_path.m_path.begin(), parent_path.m_path.end());
        ret.m_path.erase(ret.m_path.begin(), it.first);
        return ret;
    }
};


const COMDLG_FILTERSPEC g_FileTypes[] =
{
    {L"File Verification Database", L"*.sfv;*.crc;*.txt;*.ckz;*.csv;*.par;*.md5"},
    {L"All Files",                  L"*.*"}
};

enum class FileType : UINT {
    VerificationDB = 0,
    AllFiles = 1
};

enum class FileDialogAction {
    Open,
    OpenFolder,
    SaveAs,
};

std::optional<U16Path> FileDialog(HWND parent_window, FileDialogAction action, LPCWSTR dialog_title) {
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
        hres = file_open_dialog->SetFileTypes(ARRAYSIZE(g_FileTypes), g_FileTypes);
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
        U16Path ret{ filename };
        ret.prependAbsolutePathToRemoveMaxPathLimit();

        CoTaskMemFree(filename);
        return ret;
    }
    return std::nullopt;
}

std::optional<U16Path> OpenFolder(HWND parent_window) {
    return FileDialog(parent_window, FileDialogAction::OpenFolder, nullptr);
}


std::optional<U16Path> OpenFile(HWND parent_window) {
    return FileDialog(parent_window, FileDialogAction::Open, nullptr);
}

std::optional<U16Path> SaveFile(HWND parent_window) {
    return FileDialog(parent_window, FileDialogAction::SaveAs, nullptr);
}


bool is_dot(LPCWSTR p) {
    return p[0] == L'.' && p[1] == L'\0';
}

bool is_dotdot(LPCWSTR p) {
    return p[0] == L'.' && p[1] == L'.' && p[2] == L'\0';
}

struct FileInfo {
    LPCWSTR absolute_path;
    LPCWSTR relative_path;
    uint64_t size;
};

std::generator<FileInfo> iterateFiles(LPCWSTR path) {
    std::vector<U16Path> directories;
    U16Path base_path{ path };
    directories.emplace_back(path);
    while (!directories.empty()) {
        WIN32_FIND_DATA find_data;
        U16Path const current_path = std::move(directories.back());
        directories.pop_back();
        HANDLE hsearch = FindFirstFileEx(current_path.str(), FindExInfoBasic, &find_data, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
        if (hsearch == INVALID_HANDLE_VALUE) {
            MessageBox(nullptr, TEXT("Unable to open file for finding"), TEXT("Error"), MB_ICONERROR);
        }
        do {
            if (is_dot(find_data.cFileName) || is_dotdot(find_data.cFileName)) { continue; }
            U16Path p = current_path;
            p.append(find_data.cFileName);
            uint64_t filesize = (static_cast<uint64_t>(find_data.nFileSizeHigh) << 32ull) | static_cast<uint64_t>(find_data.nFileSizeLow);
            if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                p.append(L"*");
                directories.emplace_back(std::move(p));
            } else {
                co_yield FileInfo{ .absolute_path = p.str(), .relative_path = p.relativeTo(base_path).str(), .size = filesize };
            }
        } while (FindNextFile(hsearch, &find_data) != FALSE);
        bool success = (GetLastError() == ERROR_NO_MORE_FILES);
        FindClose(hsearch);
    }
}

Digest hashFile(HasherPtr hasher, LPCWSTR filepath, uint64_t filesize) {
    HANDLE hin = CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (hin == INVALID_HANDLE_VALUE) {
        std::abort();
    }
    HANDLE hmappedFile = CreateFileMapping(hin, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!hmappedFile) {
        std::abort();
    }
    LPVOID fptr = MapViewOfFile(hmappedFile, FILE_MAP_READ, 0, 0, 0);
    if (!fptr) {
        std::abort();
    }
    std::span<char const> filespan{ reinterpret_cast<char const*>(fptr), (size_t)filesize };
    hasher->addData(filespan);
    UnmapViewOfFile(fptr);
    CloseHandle(hmappedFile);
    CloseHandle(hin);
    return hasher->finalize();
}

LRESULT MainWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    } else if (msg == WM_CREATE) {
        return createUiElements(hWnd);
    } else if (msg == WM_COMMAND) {
        if ((lParam == 0) && (HIWORD(wParam) == 0)) {
            // Menu
            if (LOWORD(wParam) == ID_FILE_EXIT) {
                SendMessage(hWnd, WM_CLOSE, 0, 0);
                return 0;
            } else if (LOWORD(wParam) == ID_HELP_ABOUT) {
                MessageBox(hWnd, TEXT("About this!"), TEXT("About QuickerSFV"), MB_ICONINFORMATION);
                return 0;
            } else if (LOWORD(wParam) == ID_FILE_OPEN) {
                if (auto opt = OpenFile(hWnd); opt) {
                    ChecksumFile* checksum_file = m_fileProviders->getMatchingProviderFor(toUtf8(opt->str()));
                    if (checksum_file) {
                        FileInputWin32 reader(opt->str());
                        checksum_file->readFromFile(reader);
                        if (m_checksumFile) { m_checksumFile->clear(); }
                        m_checksumFile = checksum_file;
                    }
                }
                return 0;
            } else if (LOWORD(wParam) == ID_OPTIONS_USEAVX512) {
                MENUITEMINFO mii{ .cbSize = sizeof(MENUITEMINFO), .fMask = MIIM_STATE };
                GetMenuItemInfo(m_hMenu, ID_OPTIONS_USEAVX512, FALSE, &mii);
                if ((mii.fState & MFS_CHECKED) != 0) {
                    mii.fState &= ~MFS_CHECKED;
                } else {
                    mii.fState |= MFS_CHECKED;
                }
                SetMenuItemInfo(m_hMenu, ID_OPTIONS_USEAVX512, FALSE, &mii);

            } else if ((LOWORD(wParam) == ID_CREATE_CRC) || (LOWORD(wParam) == ID_CREATE_MD5)) {
                ChecksumFile* checksum_file = nullptr;
                if (LOWORD(wParam) == ID_CREATE_MD5) {
                    checksum_file = m_fileProviders->getMatchingProviderFor(u8"*.md5");
                }
                if (!checksum_file) { return 0; }
                if (auto opt = OpenFolder(hWnd); opt) {
                    opt->append(L"*");
                    if (auto opt_s = SaveFile(hWnd); opt_s) {
                        std::vector<U16Path> ps;
                        U16Path const base_path{ opt->str() };
                        for (FileInfo const& info : iterateFiles(base_path.str())) {
                            checksum_file->addEntry(toUtf8(info.relative_path),
                                hashFile(checksum_file->createHasher(), info.absolute_path, info.size));
                        }
                        auto filename = opt_s->str();
                        FileOutputWin32 writer(filename);
                        checksum_file->serialize(writer);
                    }
                }
                return 0;
            }
        }
    } else if (msg == WM_NOTIFY) {
        NMHDR* nmh = std::bit_cast<LPNMHDR>(lParam);
        if (nmh->hwndFrom == m_hListView) {
            return populateListView(nmh);
        }
    } else if (msg == WM_SIZE) {
        resize();
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT MainWindow::populateListView(NMHDR* nmh) {
    if (nmh->code == LVN_GETDISPINFO) {
        NMLVDISPINFO* disp_info = std::bit_cast<NMLVDISPINFO*>(nmh);
        if (disp_info->item.mask & LVIF_TEXT) {
            // name
            if (disp_info->item.iSubItem == 0) {
                TCHAR const test[] = TEXT("Name");
                _tcsncpy_s(disp_info->item.pszText, disp_info->item.cchTextMax,
                    test, _TRUNCATE);
            } else if (disp_info->item.iSubItem == 1) {
                // crc
                TCHAR const test[] = TEXT("Lorem ipsum");
                _tcsncpy_s(disp_info->item.pszText, disp_info->item.cchTextMax,
                    test, _TRUNCATE);
            } else if (disp_info->item.iSubItem == 2) {
                // status
                TCHAR const test[] = TEXT("Lorem ipsum2");
                _tcsncpy_s(disp_info->item.pszText, disp_info->item.cchTextMax,
                    test, _TRUNCATE);
            }
        } 
        if (disp_info->item.mask & LVIF_IMAGE) {
            // @todo
            if (disp_info->item.iSubItem == 0) {
                disp_info->item.iImage = disp_info->item.iItem % 3;
            }
        }
    } else if (nmh->code == LVN_ODCACHEHINT) {
        // @todo
    } else if (nmh->code == LVN_ODFINDITEM) {
        // @todo
    }
    return 0;
}

BOOL MainWindow::createMainWindow(HINSTANCE hInstance, int nCmdShow,
                                  TCHAR const* class_name, TCHAR const* window_title)
{
    m_hInstance = hInstance;
    m_windowTitle = window_title;
    m_hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU1));
    if (!m_hMenu) {
        MessageBox(nullptr, TEXT("Error creating menu"), window_title, MB_ICONERROR);
        return FALSE;
    }
    EnableMenuItem(m_hMenu, ID_OPTIONS_UPDATEDB, MF_BYCOMMAND | MF_DISABLED);
    if (quicker_sfv::supportsAvx512()) {
        MENUITEMINFO mii{ .cbSize = sizeof(MENUITEMINFO), .fMask = MIIM_STATE, .fState = MFS_ENABLED | MFS_CHECKED };
        SetMenuItemInfo(m_hMenu, ID_OPTIONS_USEAVX512, FALSE, &mii);
    }

    static_assert((sizeof(MainWindow*) == sizeof(LPARAM)) && (sizeof(MainWindow*) == sizeof(LPVOID)));
    m_hWnd = CreateWindow(
        class_name,
        m_windowTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        400,
        256,
        nullptr,
        m_hMenu,
        hInstance,
        std::bit_cast<LPVOID>(this)
    );
    if (!m_hWnd) {
        MessageBox(nullptr, TEXT("Error creating main window"), m_windowTitle, MB_ICONERROR);
        DestroyMenu(m_hMenu);
        m_hMenu = nullptr;
        return FALSE;
    }

    SetWindowLongPtr(m_hWnd, 0, std::bit_cast<LONG_PTR>(this));

    ShowWindow(m_hWnd, nCmdShow);
    if (!UpdateWindow(m_hWnd)) {
        MessageBox(nullptr, TEXT("Error updating main window"), m_windowTitle, MB_ICONERROR);
        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
        DestroyMenu(m_hMenu);
        m_hMenu = nullptr;
        return FALSE;
    }

    return TRUE;
}

LRESULT MainWindow::createUiElements(HWND parent_hwnd) {
    RECT parent_rect;
    if (!GetWindowRect(parent_hwnd, &parent_rect)) {
        MessageBox(nullptr, TEXT("Error retrieving window rect"), m_windowTitle, MB_ICONERROR);
        return -1;
    }
    WORD const cyChar = HIWORD(GetDialogBaseUnits());
    m_hTextFieldLeft = CreateWindow(TEXT("STATIC"),
        TEXT(""),
        WS_CHILD | SS_LEFT | WS_VISIBLE | SS_SUNKEN,
        0,
        0,
        (parent_rect.right - parent_rect.left) / 2,
        cyChar * 2,
        parent_hwnd,
        std::bit_cast<HMENU>(0x123ull),
        m_hInstance,
        0
    );
    if (!m_hTextFieldLeft) {
        MessageBox(nullptr, TEXT("Error creating window ui element"), m_windowTitle, MB_ICONERROR);
        return -1;
    }
    Static_SetText(m_hTextFieldLeft, TEXT("Completed files: 0/0\nOk: 0"));
    m_hTextFieldRight = CreateWindow(TEXT("STATIC"),
        TEXT(""),
        WS_CHILD | SS_LEFT | WS_VISIBLE | SS_SUNKEN,
        (parent_rect.right - parent_rect.left) / 2,
        0,
        (parent_rect.right - parent_rect.left) / 2,
        cyChar * 2,
        parent_hwnd,
        std::bit_cast<HMENU>(0x124ull),
        m_hInstance,
        0
    );
    if (!m_hTextFieldRight) {
        MessageBox(nullptr, TEXT("Error creating window ui element"), m_windowTitle, MB_ICONERROR);
        return -1;
    }
    Static_SetText(m_hTextFieldRight, TEXT("Bad: 0\nMissing: 0"));

    m_hListView = CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_LISTVIEW,
        TEXT("Blub"),
        WS_TABSTOP | WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_AUTOARRANGE | LVS_REPORT | LVS_OWNERDATA,
        0,
        cyChar * 2,
        parent_rect.right - parent_rect.left,
        parent_rect.bottom - cyChar * 2,
        parent_hwnd,
        std::bit_cast<HMENU>(0x125ull),
        m_hInstance,
        0
    );
    if (!m_hListView) {
        MessageBox(nullptr, TEXT("Error creating window ui element"), m_windowTitle, MB_ICONERROR);
        return -1;
    }

    TCHAR column_name1[] = TEXT("Name");
    TCHAR column_name2[] = TEXT("CRC");
    TCHAR column_name3[] = TEXT("Status");
    struct {
        TCHAR* name;
        int width;
    } columns[] = { { column_name1, 110 }, { column_name2, 110 }, { column_name3, 160 } };
    for (int i = 0; i < 3; ++i) {
        LV_COLUMN lv_column{
            .mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
            .fmt = LVCFMT_LEFT,
            .cx = columns[i].width,
            .pszText = columns[i].name,
        };
        ListView_InsertColumn(m_hListView, i, &lv_column);
    }

    ListView_DeleteAllItems(m_hListView);
    ListView_SetItemCount(m_hListView, 5);

    SetWindowFont(m_hTextFieldLeft, GetWindowFont(m_hListView), TRUE);
    SetWindowFont(m_hTextFieldRight, GetWindowFont(m_hListView), TRUE);

    constexpr WORD const icon_id_list[] = { IDI_ICON_CHECKMARK, IDI_ICON_CROSS, IDI_ICON_INFO };
    constexpr int const number_of_icons = ARRAYSIZE(icon_id_list);
    int const icon_size_x = GetSystemMetrics(SM_CXSMICON);
    int const icon_size_y = GetSystemMetrics(SM_CYSMICON);
    m_imageList = ImageList_Create(icon_size_x, icon_size_y, ILC_MASK | ILC_COLORDDB, number_of_icons, 0);
    if (!m_imageList) {
        MessageBox(nullptr, TEXT("Error creating window ui element"), m_windowTitle, MB_ICONERROR);
        return -1;
    }

    for (int i = 0; i < number_of_icons; ++i) {
        HANDLE hicon = LoadImage(m_hInstance, MAKEINTRESOURCE(icon_id_list[i]), IMAGE_ICON, icon_size_x, icon_size_y, LR_DEFAULTCOLOR);
        if (!hicon) {
            MessageBox(nullptr, TEXT("Error creating window ui element"), m_windowTitle, MB_ICONERROR);
            return -1;
        }
        if (ImageList_AddIcon(m_imageList, static_cast<HICON>(hicon)) != i) {
            MessageBox(nullptr, TEXT("Error creating window ui element"), m_windowTitle, MB_ICONERROR);
            return -1;
        }
    }
    ListView_SetImageList(m_hListView, m_imageList, LVSIL_SMALL);

    return 0;
}

void MainWindow::resize() {
    RECT rect;
    GetClientRect(m_hWnd, &rect);
    LONG const new_width = rect.right - rect.left;
    WORD const textFieldHeight = HIWORD(GetDialogBaseUnits()) * 2;
    MoveWindow(m_hTextFieldLeft, 0, 0, new_width / 2, textFieldHeight, TRUE);
    MoveWindow(m_hTextFieldRight, new_width / 2, 0, new_width / 2, textFieldHeight, TRUE);
    MoveWindow(m_hListView, 0, textFieldHeight, new_width, rect.bottom - textFieldHeight, TRUE);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MainWindow* main_window_ptr = std::bit_cast<MainWindow*>(GetWindowLongPtr(hWnd, 0));
    if (!main_window_ptr) {
        if (msg == WM_CREATE) {
            CREATESTRUCT const* cs = reinterpret_cast<CREATESTRUCT const*>(lParam);
            MainWindow* main_window_create_ptr = std::bit_cast<MainWindow*>(cs->lpCreateParams);
            if (!main_window_create_ptr) { return -1; }
            return main_window_create_ptr->WndProc(hWnd, msg, wParam, lParam);
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return main_window_ptr->WndProc(hWnd, msg, wParam, lParam);
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE /* hPrevInstance */,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    assert(GetACP() == 65001);  // utf-8 codepage

    FileProviders file_providers;

    TCHAR const class_name[] = TEXT("quicker_sfv");
    TCHAR const window_title[] = TEXT("QuickerSFV");

    WNDCLASS wndClass{
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = WndProc,
        .cbClsExtra = 0,
        .cbWndExtra = sizeof(MainWindow*),
        .hInstance = hInstance,
        .hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON_MAIN_WINDOW)),
        .hCursor = LoadCursor(nullptr, IDC_ARROW),
        .hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)),
        .lpszMenuName = nullptr,
        .lpszClassName = class_name
    };

    ATOM registered_class = RegisterClass(&wndClass);
    if (!registered_class) {
        MessageBox(nullptr, TEXT("Error registering class"), window_title, MB_ICONERROR);
        return 0;
    }

    MainWindow main_window(file_providers);
    if (!main_window.createMainWindow(hInstance, nCmdShow, class_name, window_title)) {
        return 0;
    }

    MSG message;
    for (;;) {
        BOOL const bret = GetMessage(&message, nullptr, 0, 0);
        if (bret == 0) {
            break;
        } else if (bret == -1) {
            MessageBox(nullptr, TEXT("Error in GetMessage"), window_title, MB_ICONERROR);
            return 0;
        } else {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
    }
    return static_cast<int>(message.wParam);
}
