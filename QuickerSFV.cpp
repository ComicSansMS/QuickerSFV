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

#include <ui/enforce.hpp>
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
#include <cstring>
#include <generator>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

using namespace quicker_sfv;
using quicker_sfv::gui::enforce;

namespace {
inline char16_t const* assumeUtf16(LPCWSTR z_str) {
    return reinterpret_cast<char16_t const*>(z_str);
}

inline LPCWSTR toWcharStr(char16_t const* z_str) {
    return reinterpret_cast<LPCWSTR>(z_str);
}
}

class FileInputWin32 : public FileInput {
private:
    HANDLE m_fin;
    bool m_eof;
public:
    FileInputWin32(std::u16string const& filename)
        :m_eof(false)
    {
        m_fin = CreateFile(toWcharStr(filename.c_str()), GENERIC_READ, FILE_SHARE_READ, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
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
    FileOutputWin32(std::u16string const& filename)
    {
        m_fout = CreateFile(toWcharStr(filename.c_str()), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
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

class FileProviders {
private:
    std::vector<quicker_sfv::ChecksumProviderPtr> m_providers;
    struct FileType {
        std::u8string extensions;
        std::u8string description;
    };
    std::vector<FileType> m_fileTypes;
public:
    FileProviders()
    {
        m_providers.emplace_back(quicker_sfv::createSfvProvider());
        registerFileType(*m_providers.back());
        m_providers.emplace_back(quicker_sfv::createMD5Provider());
        registerFileType(*m_providers.back());
        // @todo:
        //  - .ckz
        //  - .par
        //  - .csv
        //  - .txt
    }

    ChecksumProvider* getMatchingProviderFor(std::u8string_view filename) {
        for (auto const& p: m_providers) {
            std::u8string_view exts = p->fileExtensions();
            // split extensions
            for (std::u8string_view::size_type it = 0; it != std::u8string_view::npos;) {
                auto const it_end = exts.find(u8';');
                auto ext = exts.substr(it, it_end);
                if (ext[0] == u8'*') {
                    ext = ext.substr(1);
                    if (filename.ends_with(ext)) {
                        return p.get();
                    }
                } else {
                    enforce(!"Invalid extension provided");
                }
                it = it_end;
            }
        }
        return nullptr;
    }

    std::span<FileType const> fileTypes() const {
        return m_fileTypes;
    }
private:
    void registerFileType(ChecksumProvider const& provider) {
        m_fileTypes.emplace_back(std::u8string(provider.fileExtensions()), std::u8string(provider.fileDescription()));
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
    ChecksumFile m_checksumFile;
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
     m_hListView(nullptr), m_imageList(nullptr), m_fileProviders(&file_providers)
{
}

const COMDLG_FILTERSPEC g_FileTypes[] =
{
    {L"File Verification Database", L"*.sfv;*.crc;*.txt;*.ckz;*.csv;*.par;*.md5"},
    {L"All Files",                  L"*.*"}
};

std::optional<std::u16string> OpenFolder(HWND parent_window) {
    return gui::FileDialog(parent_window, gui::FileDialogAction::OpenFolder, nullptr, {});
}


std::optional<std::u16string> OpenFile(HWND parent_window, FileProviders const& file_providers) {
    return gui::FileDialog(parent_window, gui::FileDialogAction::Open, nullptr, g_FileTypes);
}

std::optional<std::u16string> SaveFile(HWND parent_window, FileProviders const& file_providers) {
    return gui::FileDialog(parent_window, gui::FileDialogAction::SaveAs, nullptr, g_FileTypes);
}

struct FileInfo {
    LPCWSTR absolute_path;
    std::u16string_view relative_path;
    uint64_t size;
};

std::generator<FileInfo> iterateFiles(std::u16string const& base_path) {
    auto const is_dot = [](LPCWSTR p) -> bool { return p[0] == L'.' && p[1] == L'\0'; };
    auto const is_dotdot = [](LPCWSTR p) -> bool { return p[0] == L'.' && p[1] == L'.' && p[2] == L'\0'; };
    auto const appendWildcard = [](std::u16string& str) {
        if (!str.empty() && str.back() == u'*') { str.pop_back(); }
        if (!str.empty() && str.back() == u'\\') { str.pop_back(); }
        str.append(u"\\*");
    };
    auto const relativePathTo = [](std::u16string_view p, std::u16string_view parent_path) -> std::u16string {
        std::u16string ret{ p };
        auto it = std::mismatch(ret.begin(), ret.end(), parent_path.begin(), parent_path.end());
        if (it.first != ret.end() && (*(it.first) == u'\\')) { ++it.first; }
        ret.erase(ret.begin(), it.first);
        return ret;
    };
    std::vector<std::u16string> directories;
    directories.emplace_back(base_path);
    appendWildcard(directories.back());
    while (!directories.empty()) {
        WIN32_FIND_DATA find_data;
        std::u16string const current_path = std::move(directories.back());
        directories.pop_back();
        HANDLE hsearch = FindFirstFileEx(toWcharStr(current_path.c_str()), FindExInfoBasic, &find_data,
                                         FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
        if (hsearch == INVALID_HANDLE_VALUE) {
            MessageBox(nullptr, TEXT("Unable to open file for finding"), TEXT("Error"), MB_ICONERROR);
        }
        do {
            if (is_dot(find_data.cFileName) || is_dotdot(find_data.cFileName)) { continue; }
            std::u16string p = current_path;
            p.pop_back();   // pop wildcard
            p.append(assumeUtf16(find_data.cFileName));
            uint64_t filesize = (static_cast<uint64_t>(find_data.nFileSizeHigh) << 32ull) | static_cast<uint64_t>(find_data.nFileSizeLow);
            if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                appendWildcard(p);
                directories.emplace_back(std::move(p));
            } else {
                co_yield FileInfo{ .absolute_path = toWcharStr(p.c_str()), .relative_path = relativePathTo(p, base_path), .size = filesize };
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
                if (auto opt = OpenFile(hWnd, *m_fileProviders); opt) {
                    ChecksumProvider* checksum_provider = m_fileProviders->getMatchingProviderFor(convertToUtf8(*opt));
                    if (checksum_provider) {
                        FileInputWin32 reader(*opt);
                        m_checksumFile = checksum_provider->readFromFile(reader);
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
                ChecksumProvider* checksum_provider = nullptr;
                if (LOWORD(wParam) == ID_CREATE_MD5) {
                    checksum_provider = m_fileProviders->getMatchingProviderFor(u8"*.md5");
                }
                if (!checksum_provider) { return 0; }
                if (auto const opt = OpenFolder(hWnd); opt) {
                    ChecksumFile new_file;
                    HasherOptions hasher_options{ .has_sse42 = true, .has_avx512 = false };
                    if (auto const opt_s = SaveFile(hWnd, *m_fileProviders); opt_s) {
                        for (FileInfo const& info : iterateFiles(*opt)) {
                            new_file.addEntry(convertToUtf8(info.relative_path),
                                hashFile(checksum_provider->createHasher(hasher_options), info.absolute_path, info.size));
                        }
                        FileOutputWin32 writer(*opt_s);
                        new_file.sortEntries();
                        checksum_provider->serialize(writer, new_file);
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
    enforce(GetACP() == 65001);  // utf-8 codepage

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
