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

#include <quicker_sfv/ui/command_line_parser.hpp>
#include <quicker_sfv/ui/enforce.hpp>
#include <quicker_sfv/ui/event_handler.hpp>
#include <quicker_sfv/ui/file_dialog.hpp>
#include <quicker_sfv/ui/operation_scheduler.hpp>
#include <quicker_sfv/ui/plugin_support.hpp>
#include <quicker_sfv/ui/string_helper.hpp>
#include <quicker_sfv/ui/user_messages.hpp>

#ifndef QUICKER_SFV_BUILD_SELF_CONTAINED
#   include <quicker_sfv/plugin/plugin_sdk.h>
#endif

#include <Windows.h>
#include <windowsx.h>
#include <atlbase.h>
#include <CommCtrl.h>
#include <gdiplus.h>
#include <shellapi.h>
#include <ShObjIdl_core.h>
#include <strsafe.h>
#include <tchar.h>
#include <uxtheme.h>

#include <resource.h>

#include <algorithm>
#include <bit>
#include <cstring>
#include <memory>
#include <numeric>
#include <optional>
#include <utility>
#include <vector>

#include <format>

using namespace quicker_sfv;
using namespace quicker_sfv::gui;

class FileProviders {
public:
    struct FileType {
        std::u8string extensions;
        std::u8string description;
        size_t provider_index;
    };
private:
    std::vector<ChecksumProviderPtr> m_providers;
    std::vector<FileType> m_fileTypesVerify;
    std::vector<FileType> m_fileTypesCreate;
public:
    FileProviders()
    {
        addProvider(quicker_sfv::createSfvProvider());
        addProvider(quicker_sfv::createMD5Provider());
    }

    ChecksumProvider* getMatchingProviderFor(std::u8string_view filename, bool supports_create) {
        for (auto const& p: m_providers) {
            std::u8string_view exts = p->fileExtensions();
            // split extensions
            for (std::u8string_view::size_type it = 0; it != std::u8string_view::npos;) {
                auto const it_end = exts.find(u8';');
                auto ext = exts.substr(it, it_end);
                if (ext[0] == u8'*') {
                    ext = ext.substr(1);
                    if (filename.ends_with(ext) && (!supports_create || p->getCapabilities() == ProviderCapabilities::Full)) {
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

    std::span<FileType const> fileTypesVerify() const {
        return m_fileTypesVerify;
    }

    std::span<FileType const> fileTypesCreate() const {
        return m_fileTypesCreate;
    }

    ChecksumProvider* getProviderFromIndex(size_t provider_index) {
        if ((provider_index >= 0) && (provider_index < m_providers.size())) {
            return m_providers[provider_index].get();
        }
        return nullptr;
    }

    std::u16string getExeDirectory() {
        char* exe_file_path = nullptr;
        if (_get_pgmptr(&exe_file_path) != 0) { return {}; }
        return extractBasePathFromFilePath(convertToUtf16(assumeUtf8(exe_file_path)));
    }

    void loadPlugins() {
#ifndef QUICKER_SFV_BUILD_SELF_CONTAINED
        WIN32_FIND_DATA find_data;
        std::u16string search_path = getExeDirectory();
        if (search_path.empty()) { return; }
        search_path.append(u"*.dll");
        HANDLE hsearch = FindFirstFile(toWcharStr(search_path), &find_data);
        if (hsearch == INVALID_HANDLE_VALUE) {
            return;
        }
        do {
            HMODULE hmod = LoadLibrary(find_data.cFileName);
            if (hmod) {
                if (auto loader = (QuickerSFV_LoadPluginFunc)GetProcAddress(hmod, "QuickerSFV_LoadPlugin"); loader) {
                    addProvider(quicker_sfv::gui::loadPlugin(loader));
                } else if (auto loader_cpp = (QuickerSFV_LoadPluginCppFunc)GetProcAddress(hmod, "QuickerSFV_LoadPlugin_Cpp"); loader_cpp) {
                    addProvider(quicker_sfv::gui::loadPluginCpp(loader_cpp));
                }

            }
        } while (FindNextFile(hsearch, &find_data) != FALSE);
#endif
    }

private:
    void addProvider(ChecksumProviderPtr&& p) {
        if (p->getCapabilities() == ProviderCapabilities::Full) {
            m_fileTypesCreate.emplace_back(std::u8string(p->fileExtensions()), std::u8string(p->fileDescription()), m_providers.size());
        }
        m_fileTypesVerify.emplace_back(std::u8string(p->fileExtensions()), std::u8string(p->fileDescription()), m_providers.size());
        m_providers.emplace_back(std::move(p));
    }
};

class MainWindow: public EventHandler {
private:
    HINSTANCE m_hInstance;
    TCHAR const* m_windowTitle;
    HWND m_hWnd;
    HMENU m_hMenu;
    HWND m_hTextFieldLeft;
    HWND m_hTextFieldRight;
    HWND m_hListView;
    HIMAGELIST m_imageList;
    HMENU m_hPopupMenu;

    struct Stats {
        uint32_t total;
        uint32_t completed;
        uint32_t progress;
        uint32_t ok;
        uint32_t bad;
        uint32_t missing;
        uint32_t bandwidth;
    } m_stats;

    struct ListViewEntry {
        std::u16string name;
        std::u16string checksum;
        enum Status {
            Ok,
            FailedMismatch,
            FailedMissing,
            Information,
            MessageOk,
            MessageBad,
        } status;
        uint32_t original_position;
        std::u16string absolute_file_path;
    };
    std::vector<ListViewEntry> m_listEntries;
    struct ListViewSort {
        int sort_column;
        enum class Order {
            Original,
            Ascending,
            Descending,
        } order;
    } m_listSort;

    HasherOptions m_options;
    FileProviders* m_fileProviders;
    OperationScheduler* m_scheduler;

    std::u16string m_outFile;
public:
    explicit MainWindow(FileProviders& file_providers, OperationScheduler& scheduler);

    ~MainWindow();

    MainWindow(MainWindow const&) = delete;
    MainWindow(MainWindow&&) = delete;
    MainWindow& operator=(MainWindow const&) = delete;
    MainWindow& operator=(MainWindow&&) = delete;

    BOOL createMainWindow(HINSTANCE hInstance, int nCmdShow,
                          TCHAR const* class_name, TCHAR const* window_title);

    LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND getHwnd() const;
    HasherOptions getOptions() const;

    void onCheckStarted(uint32_t n_files) override;
    void onFileStarted(std::u8string_view file, std::u8string_view absolute_file_path) override;
    void onProgress(uint32_t percentage, uint32_t bandwidth_mib_s) override;
    void onFileCompleted(std::u8string_view file, Digest const& checksum, std::u8string_view absolute_file_path, CompletionStatus status) override;
    void onCheckCompleted(Result r) override;
    void onCancelRequested() override;
    void onCanceled() override;
    void onError(quicker_sfv::Error error, std::u8string_view msg) override;

    void setOutFile(std::u16string out_file);
    void writeResultsToFile() const;
private:
    LRESULT createUiElements(HWND parent_hwnd);

    LRESULT populateListView(NMHDR* nmh);
    static constexpr UINT_PTR const ListViewSubclassId = 1;
    LRESULT paintListViewHeader(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void UpdateStats();

    void resize();

    void addListEntry(std::u16string name, std::u16string checksum = {},
                      ListViewEntry::Status status = ListViewEntry::Status::Information,
                      std::u16string absolute_file_path = {});

    std::vector<ListViewEntry const*> getSelectedListViewItems() const;
    void doCopySelectionToClipboard();
    void doMarkBadFiles();
    void doDeleteMarkedFiles();

    static TCHAR const* getStatusTextForStatus(ListViewEntry::Status s);
};

MainWindow::MainWindow(FileProviders& file_providers, OperationScheduler& scheduler)
    :m_hInstance(nullptr), m_windowTitle(nullptr), m_hWnd(nullptr), m_hMenu(nullptr), m_hTextFieldLeft(nullptr),
     m_hTextFieldRight(nullptr), m_hListView(nullptr), m_imageList(nullptr), m_hPopupMenu(nullptr),
     m_stats{}, m_listSort{ .sort_column = 0, .order = ListViewSort::Order::Original },
     m_options{ .has_sse42 = quicker_sfv::supportsSse42(), .has_avx512 = false},
     m_fileProviders(&file_providers), m_scheduler(&scheduler)
{
}

MainWindow::~MainWindow() {
    if (m_hPopupMenu) {
        DestroyMenu(m_hPopupMenu);
    }
}

struct FileSpec {
    std::vector<COMDLG_FILTERSPEC> fileTypes;
    std::vector<std::u16string> stringPool;
};

FileSpec determineFileTypes(std::span<FileProviders::FileType const> const& file_types, bool include_catchall) {
    FileSpec ret;
    if (include_catchall) {
        ret.stringPool.push_back(u"File Verification Database");
        ret.stringPool.push_back(u"");
    }
    for (auto const& f : file_types) {
        ret.stringPool.push_back(convertToUtf16(f.description));
        ret.stringPool.push_back(convertToUtf16(f.extensions));
        if (include_catchall) {
            if (!ret.stringPool[1].empty()) { ret.stringPool[1].append(u";"); }
            ret.stringPool[1].append(ret.stringPool.back());
        }
    }
    if (include_catchall) {
        ret.stringPool.push_back(u"All Files");
        ret.stringPool.push_back(u"*.*");
    }
    ret.fileTypes.reserve(ret.stringPool.size() / 2);
    for (size_t i = 0; i < ret.stringPool.size(); i += 2) {
        ret.fileTypes.push_back(COMDLG_FILTERSPEC{ toWcharStr(ret.stringPool[i]), toWcharStr(ret.stringPool[i + 1]) });
    }
    return ret;
}

std::optional<gui::FileDialogResult> OpenFolder(HWND parent_window) {
    return gui::FileDialog(parent_window, gui::FileDialogAction::OpenFolder, nullptr, {});
}


std::optional<gui::FileDialogResult> OpenFile(HWND parent_window, FileProviders const& file_providers) {
    return gui::FileDialog(parent_window, gui::FileDialogAction::Open, nullptr, determineFileTypes(file_providers.fileTypesVerify(), true).fileTypes);
}

std::optional<gui::FileDialogResult> SaveFile(HWND parent_window, FileProviders const& file_providers) {
    return gui::FileDialog(parent_window, gui::FileDialogAction::SaveAs, nullptr, determineFileTypes(file_providers.fileTypesCreate(), false).fileTypes);
}

namespace {
class GlobalAllocGuard {
private:
    HGLOBAL m_hmem;
public:
    explicit GlobalAllocGuard(HGLOBAL hmem)
        :m_hmem(hmem)
    {}
    GlobalAllocGuard& operator=(GlobalAllocGuard&&) = delete;
    ~GlobalAllocGuard() {
        if (m_hmem) { GlobalFree(m_hmem); }
    }
    void release() { m_hmem = nullptr; }
};

class MemLockGuard {
private:
    HGLOBAL m_hmem;
    char* m_ptr;
public:
    MemLockGuard& operator=(MemLockGuard&&) = delete;

    explicit MemLockGuard(HGLOBAL hmem)
        :m_hmem(hmem)
    {
        m_ptr = reinterpret_cast<char*>(GlobalLock(m_hmem));
    }

    ~MemLockGuard() {
        if (m_ptr) {
            enforce(GlobalUnlock(m_hmem) == 0);
        }
    }

    char* operator()() {
        return m_ptr;
    }
};

struct ClipboardGuard {
    ClipboardGuard& operator=(ClipboardGuard&&) = delete;
    explicit ClipboardGuard(HWND hwnd) {
        OpenClipboard(hwnd);
    }
    ~ClipboardGuard() {
        CloseClipboard();
    }
};
}

INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_INITDIALOG: {
        HWND hStatic = GetDlgItem(hDlg, IDC_STATIC_HEADER_TEXT);
        LOGFONT logfont {
            .lfHeight = 32, .lfWidth = 0, .lfEscapement = 0, .lfOrientation = 0, .lfWeight = FW_BOLD, .lfItalic = FALSE,
            .lfUnderline = FALSE, .lfStrikeOut = FALSE, .lfCharSet = ANSI_CHARSET, .lfOutPrecision = OUT_TT_PRECIS,
            .lfClipPrecision = CLIP_DEFAULT_PRECIS, .lfQuality = CLEARTYPE_QUALITY, .lfPitchAndFamily = VARIABLE_PITCH,
            .lfFaceName = TEXT("Segoe")
        };
        HFONT font = CreateFontIndirect(&logfont);
        SendMessage(hStatic, WM_SETFONT, std::bit_cast<WPARAM>(font), TRUE);
        Version const v = quicker_sfv::getVersion();
        auto const text = formatString(50, TEXT("QuickerSFV v%d.%d.%d"), v.major, v.minor, v.patch);
        SendMessage(hStatic, WM_SETTEXT, 0, std::bit_cast<LPARAM>(toWcharStr(text)));
    } return TRUE;
    case WM_DESTROY: {
        HWND hStatic = GetDlgItem(hDlg, IDC_STATIC_HEADER_TEXT);
        HFONT font = std::bit_cast<HFONT>(SendMessage(hStatic, WM_GETFONT, 0, 0));
        DeleteFont(font);
    } return FALSE;
    case WM_NOTIFY: {
        LPNMHDR nmh = std::bit_cast<LPNMHDR>(lParam);
        if ((nmh->code == NM_CLICK) || (nmh->code == NM_RETURN)) {
            if ((nmh->hwndFrom == GetDlgItem(hDlg, IDC_SYSLINK2)) || (nmh->hwndFrom == GetDlgItem(hDlg, IDC_SYSLINK3))) {
                PNMLINK pnmlink = std::bit_cast<PNMLINK>(lParam);
                ShellExecute(nullptr, TEXT("open"), pnmlink->item.szUrl, nullptr, nullptr, SW_SHOW);
                return TRUE;
            }
        }
    } break;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            EndDialog(hDlg, 1);
        }
        break;
    }
    return FALSE;
}

LRESULT MainWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_DESTROY) {
        m_scheduler->post(Operation::Cancel{});
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
                DialogBox(m_hInstance, MAKEINTRESOURCE(IDD_DIALOG_ABOUT), m_hWnd, AboutDlgProc);
                return 0;
            } else if (LOWORD(wParam) == ID_FILE_OPEN) {
                if (auto opt = OpenFile(hWnd, *m_fileProviders); opt) {
                    auto const& [source_file_path, selected_provider] = *opt;
                    ChecksumProvider* const checksum_provider =
                        ((selected_provider == 0) || (selected_provider - 1 >= m_fileProviders->fileTypesVerify().size())) ?
                        m_fileProviders->getMatchingProviderFor(convertToUtf8(source_file_path), false) :
                        m_fileProviders->getProviderFromIndex(m_fileProviders->fileTypesVerify()[selected_provider - 1].provider_index);
                    if (checksum_provider) {
                        m_scheduler->post(Operation::Verify{
                            .event_handler = this,
                            .options = m_options,
                            .source_file = source_file_path,
                            .provider = checksum_provider
                        });
                    }
                }
                return 0;
            } else if (LOWORD(wParam) == ID_OPTIONS_USEAVX512) {
                MENUITEMINFO mii{ .cbSize = sizeof(MENUITEMINFO), .fMask = MIIM_STATE };
                GetMenuItemInfo(m_hMenu, ID_OPTIONS_USEAVX512, FALSE, &mii);
                if ((mii.fState & MFS_CHECKED) != 0) {
                    mii.fState &= ~MFS_CHECKED;
                    m_options.has_avx512 = false;
                } else {
                    mii.fState |= MFS_CHECKED;
                    m_options.has_avx512 = true;
                }
                SetMenuItemInfo(m_hMenu, ID_OPTIONS_USEAVX512, FALSE, &mii);
            } else if (LOWORD(wParam) == ID_CREATE_FROM_FOLDER) {
                if (auto const opt = OpenFolder(hWnd); opt) {
                    auto const& [folder_path, _] = *opt;
                    if (auto const opt_s = SaveFile(hWnd, *m_fileProviders); opt_s) {
                        auto const& [target_file_path, selected_provider] = *opt_s;
                        ChecksumProvider* checksum_provider =
                            (selected_provider >= m_fileProviders->fileTypesCreate().size()) ?
                            m_fileProviders->getMatchingProviderFor(convertToUtf8(target_file_path), true) :
                            m_fileProviders->getProviderFromIndex(m_fileProviders->fileTypesCreate()[selected_provider].provider_index);
                        m_scheduler->post(Operation::CreateFromFolder{
                            .event_handler = this,
                            .options = m_options,
                            .target_file = target_file_path,
                            .folder_path = folder_path,
                            .provider = checksum_provider,
                        });
                    }
                }
                return 0;
            } else if (LOWORD(wParam) == ID_CONTEXTMENU_COPY) {
                doCopySelectionToClipboard();
            } else if (LOWORD(wParam) == ID_CONTEXTMENU_MARKBADFILES) {
                doMarkBadFiles();
            } else if (LOWORD(wParam) == ID_CONTEXTMENU_DELETEMARKEDFILES) {
                doDeleteMarkedFiles();
            }
        } else if (LOWORD(wParam) == ID_ACCELERATOR_COPY) {
            doCopySelectionToClipboard();
        } else if (LOWORD(wParam) == ID_ACCELERATOR_SELECT_ALL) {
            ListView_SetItemState(m_hListView, -1, LVIS_SELECTED, LVIS_SELECTED);
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

TCHAR const* MainWindow::getStatusTextForStatus(ListViewEntry::Status s) {
    TCHAR const* status_texts[] = { TEXT("OK"), TEXT("FAILED. Checksum mismatch"), TEXT("FAILED. File does not exist"), TEXT("") };
    return status_texts[std::min<int>(s, ARRAYSIZE(status_texts) - 1)];
}

LRESULT MainWindow::populateListView(NMHDR* nmh) {
    if (nmh->code == LVN_GETDISPINFO) {
        NMLVDISPINFO* disp_info = std::bit_cast<NMLVDISPINFO*>(nmh);
        if ((disp_info->item.iItem < 0) || (static_cast<size_t>(disp_info->item.iItem) >= m_listEntries.size())) { enforce(!"Should never happen"); return 0; }
        ListViewEntry& entry = m_listEntries[disp_info->item.iItem];
        if (disp_info->item.mask & LVIF_TEXT) {
            if (disp_info->item.iSubItem == 0) {
                // name
                _tcsncpy_s(disp_info->item.pszText, disp_info->item.cchTextMax,
                    toWcharStr(entry.name), _TRUNCATE);
            } else if (disp_info->item.iSubItem == 1) {
                // checksum
                _tcsncpy_s(disp_info->item.pszText, disp_info->item.cchTextMax,
                    toWcharStr(entry.checksum), _TRUNCATE);
            } else if (disp_info->item.iSubItem == 2) {
                // status
                _tcsncpy_s(disp_info->item.pszText, disp_info->item.cchTextMax,
                           getStatusTextForStatus(entry.status), _TRUNCATE);
            }
        } 
        if (disp_info->item.mask & LVIF_IMAGE) {
            if (disp_info->item.iSubItem == 0) {
                switch (entry.status) {
                case ListViewEntry::Status::Ok:
                    disp_info->item.iImage = 0;
                    break;
                case ListViewEntry::Status::FailedMismatch:
                    disp_info->item.iImage = 1;
                    break;
                case ListViewEntry::Status::FailedMissing:
                    disp_info->item.iImage = 1;
                    break;
                case ListViewEntry::Status::MessageOk:
                    disp_info->item.iImage = 0;
                    break;
                case ListViewEntry::Status::MessageBad:
                    disp_info->item.iImage = 1;
                    break;
                case ListViewEntry::Status::Information: [[fallthrough]];
                default:
                    disp_info->item.iImage = 2;
                    break;
                }
            }
        }
    } else if (nmh->code == LVN_ODCACHEHINT) {
        // not handled - all items always in memory
    } else if (nmh->code == LVN_ODFINDITEM) {
        // not handled - no finding in results list for now
    } else if (nmh->code == LVN_COLUMNCLICK) {
        NMLISTVIEW* nmlv = std::bit_cast<NMLISTVIEW*>(nmh);
        int const column_index = nmlv->iSubItem;
        if (column_index != m_listSort.sort_column) {
            m_listSort.sort_column = column_index;
            m_listSort.order = ListViewSort::Order::Ascending;
        } else {
            m_listSort.order = static_cast<ListViewSort::Order>((static_cast<int>(m_listSort.order) + 1) % 3);
        }
        std::stable_sort(begin(m_listEntries), end(m_listEntries),
            [sort = m_listSort](auto const& lhs, auto const& rhs) {
                if (sort.order == ListViewSort::Order::Original) {
                    return lhs.original_position < rhs.original_position;
                } else if (sort.order == ListViewSort::Order::Ascending) {
                    if (sort.sort_column == 0) {
                        return lstrcmpi(toWcharStr(lhs.name), toWcharStr(rhs.name)) < 0;
                    } else if (sort.sort_column == 1) {
                        return lhs.checksum < rhs.checksum;
                    } else {
                        return lhs.status < rhs.status;
                    }
                } else {
                    if (sort.sort_column == 0) {
                        return lstrcmpi(toWcharStr(lhs.name), toWcharStr(rhs.name)) > 0;
                    } else if (sort.sort_column == 1) {
                        return lhs.checksum > rhs.checksum;
                    } else {
                        return lhs.status > rhs.status;
                    }
                }
            });
        ListView_RedrawItems(m_hListView, 0, m_listEntries.size());
        // Force redraw of all headers to clear leftover glyphs
        RECT header_rect;
        GetWindowRect(ListView_GetHeader(m_hListView), &header_rect);
        POINT top_left{ .x = header_rect.left, .y = header_rect.top };
        POINT bottom_right{ .x = header_rect.right, .y = header_rect.bottom };
        ScreenToClient(m_hWnd, &top_left);
        ScreenToClient(m_hWnd, &bottom_right);
        header_rect = RECT{ .left = top_left.x, .top = top_left.y, .right = bottom_right.x, .bottom = bottom_right.y };
        InvalidateRect(m_hWnd, &header_rect, FALSE);
    } else if (nmh->code == NM_RCLICK) {
        POINT mouse_cursor;
        if (!GetCursorPos(&mouse_cursor)) { return 0; }
        if (!TrackPopupMenu(GetSubMenu(m_hPopupMenu, 0), TPM_RIGHTBUTTON,
                            mouse_cursor.x, mouse_cursor.y, 0, m_hWnd, nullptr)) {
            return 0;
        }
    }
    return 0;
}

LRESULT MainWindow::paintListViewHeader(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // first draw the header as normal
    RECT update_rect_world;

    RECT rect_header;
    Header_GetItemRect(hWnd, m_listSort.sort_column, &rect_header);
    GetUpdateRect(hWnd, &update_rect_world, FALSE);

    DefSubclassProc(hWnd, uMsg, wParam, lParam);

    // then draw the glyph over it, if we have a sort column
    if (m_listSort.order != ListViewSort::Order::Original) {
        //OutputDebugStringA(std::format("Paint {} - W {} {} {} {}\n", std::bit_cast<uint64_t>(hWnd), update_rect_world.left, update_rect_world.right, update_rect_world.top, update_rect_world.bottom).c_str());
        HDC hdc = GetDC(hWnd);
        // convert world coordinates to client coordintes in update
        POINT update_rect_points[2]{
            POINT{ .x = update_rect_world.left, .y = update_rect_world.top },
            POINT{ .x = update_rect_world.right, .y = update_rect_world.bottom }
        };
        LPtoDP(hdc, update_rect_points, 2);
        RECT const update_rect_client{ .left = update_rect_points[0].x, .top = update_rect_points[0].y, .right = update_rect_points[1].x, .bottom = update_rect_points[1].y };
        //OutputDebugStringA(std::format("Paint {} - C {} {} {} {}\n", std::bit_cast<uint64_t>(hWnd), update_rect_client.left, update_rect_client.right, update_rect_client.top, update_rect_client.bottom).c_str());
        Gdiplus::Rect update(update_rect_client.left, update_rect_client.top, (update_rect_client.right - update_rect_client.left), (update_rect_client.bottom - update_rect_client.top));

        RECT rect;
        Header_GetItemRect(hWnd, m_listSort.sort_column, &rect);

        Gdiplus::Point points[3];
        Gdiplus::Point const top_midpoint((rect.right - rect.left) / 2 + rect.left, rect.top);
        INT const arrow_width = 10;
        INT const arrow_height = 5;
        if (m_listSort.order == ListViewSort::Order::Ascending) {
            points[0] = top_midpoint + Gdiplus::Point(-(arrow_width / 2), arrow_height);
            points[1] = top_midpoint;
            points[2] = top_midpoint + Gdiplus::Point((arrow_width / 2), arrow_height);
        } else {
            points[0] = top_midpoint + Gdiplus::Point(-(arrow_width / 2), 0);
            points[1] = top_midpoint + Gdiplus::Point(0, arrow_height);
            points[2] = top_midpoint + Gdiplus::Point((arrow_width / 2), 0);
        }

        if (!update.Contains(points[0]) && !update.Contains(points[1]) && !update.Contains(points[2])) {
            // glyph is not in the updated region; get out
            return 0;
        }
        Gdiplus::Graphics gdi(hdc);
        gdi.SetClip(Gdiplus::Rect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top));
        Gdiplus::Pen p(Gdiplus::Color(255, 92, 92, 92));
        gdi.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias8x8);
        gdi.DrawLines(&p, points, 3);
    }
    return 0;
}

std::vector<MainWindow::ListViewEntry const*> MainWindow::getSelectedListViewItems() const {
    std::vector<ListViewEntry const*> selected_items;
    for (int i = 0, i_end = static_cast<int>(m_listEntries.size()); i != i_end; ++i) {
        if (ListView_GetItemState(m_hListView, i, LVIS_SELECTED) == LVIS_SELECTED) {
            selected_items.push_back(&m_listEntries[i]);
        }
    }
    return selected_items;
}

void MainWindow::doCopySelectionToClipboard() {
    std::vector<ListViewEntry const*> selected_items = getSelectedListViewItems();
    size_t const total_string_length = std::accumulate(begin(selected_items), end(selected_items), 1ull,
        [](size_t acc, ListViewEntry const* e) { return acc + e->name.size() + 2; });
    if (selected_items.empty()) { return; }
    HGLOBAL hmem = GlobalAlloc(GHND, total_string_length * sizeof(TCHAR));
    if (!hmem) { return; }
    GlobalAllocGuard guard_hmem(hmem);
    {
        MemLockGuard mem_lock(hmem);
        char* mem = mem_lock();
        if (!mem) { return; }

        size_t offset = 0;
        for (auto const& e : selected_items) {
            std::memcpy(mem + offset, e->name.c_str(), e->name.size() * sizeof(TCHAR));
            offset += e->name.size() * sizeof(TCHAR);
            WCHAR const line_break[] = TEXT("\r\n");
            std::memcpy(mem + offset, line_break, 2*sizeof(TCHAR));
            offset += 2;
        }
    }
    {
        ClipboardGuard guard_clipboard(m_hWnd);
        if (!EmptyClipboard()) { return; }
        if (!SetClipboardData(CF_UNICODETEXT, hmem)) { return; }
        guard_hmem.release();
    }
}

void MainWindow::doMarkBadFiles() {
    // clear previous selection
    ListView_SetItemState(m_hListView, -1, 0, LVIS_SELECTED);
    // mark all bad
    for (size_t i = 0, i_end = m_listEntries.size(); i != i_end; ++i) {
        if ((m_listEntries[i].status == ListViewEntry::Status::FailedMismatch) ||
            (m_listEntries[i].status == ListViewEntry::Status::FailedMissing))
        {
            ListView_SetItemState(m_hListView, i, LVIS_SELECTED, LVIS_SELECTED);
        }
    }
}

void MainWindow::doDeleteMarkedFiles() {
    std::vector<ListViewEntry const*> selected_items = getSelectedListViewItems();
    selected_items.erase(
        std::remove_if(begin(selected_items), end(selected_items), [](ListViewEntry const* e) {
                return (e->status != ListViewEntry::Status::Ok) &&
                    (e->status != ListViewEntry::Status::FailedMismatch);
            }), end(selected_items));
    if (selected_items.empty()) { return; }
    int const answer = MessageBox(m_hWnd,
        toWcharStr(formatString(72, TEXT("%d file%s will be deleted from disk.\n\nAre you sure?"),
                                selected_items.size(), (selected_items.size() == 1 ? TEXT("") : TEXT("s")))), 
        TEXT("QuickerSFV"), MB_YESNO | MB_ICONEXCLAMATION);
    if (answer != IDYES) { return; }
    for (auto const& f : selected_items) {
        if (DeleteFile(toWcharStr(f->absolute_file_path)) != 0) {
            auto it = std::find_if(begin(m_listEntries), end(m_listEntries),
                [p = f->original_position](ListViewEntry const& l) { return l.original_position == p; });
            if (it != end(m_listEntries)) { m_listEntries.erase(it); }
        }
    }
    ListView_SetItemCountEx(m_hListView, m_listEntries.size(), 0);
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
    HMENU const field_left_id =
#ifdef _WIN64
        std::bit_cast<HMENU>(0x123ull);
#else
        std::bit_cast<HMENU>(0x123u);
#endif
    m_hTextFieldLeft = CreateWindowW(TEXT("STATIC"),
        TEXT(""),
        WS_CHILD | SS_LEFT | WS_VISIBLE | SS_SUNKEN,
        0,
        0,
        (parent_rect.right - parent_rect.left) / 2,
        cyChar * 2,
        parent_hwnd,
        field_left_id,
        m_hInstance,
        0
    );
    if (!m_hTextFieldLeft) {
        MessageBox(nullptr, TEXT("Error creating window ui element"), m_windowTitle, MB_ICONERROR);
        return -1;
    }
    Static_SetText(m_hTextFieldLeft, TEXT("Completed files: 0/0\nOk: 0"));
    HMENU const field_right_id =
#ifdef _WIN64
        std::bit_cast<HMENU>(0x124ull);
#else
        std::bit_cast<HMENU>(0x124u);
#endif
    m_hTextFieldRight = CreateWindow(TEXT("STATIC"),
        TEXT(""),
        WS_CHILD | SS_LEFT | WS_VISIBLE | SS_SUNKEN,
        (parent_rect.right - parent_rect.left) / 2,
        0,
        (parent_rect.right - parent_rect.left) / 2,
        cyChar * 2,
        parent_hwnd,
        field_right_id,
        m_hInstance,
        0
    );
    if (!m_hTextFieldRight) {
        MessageBox(nullptr, TEXT("Error creating window ui element"), m_windowTitle, MB_ICONERROR);
        return -1;
    }
    Static_SetText(m_hTextFieldRight, TEXT("Bad: 0\nMissing: 0"));

    HMENU const list_view_id =
#ifdef _WIN64
        std::bit_cast<HMENU>(0x125ull);
#else
        std::bit_cast<HMENU>(0x125u);
#endif
    m_hListView = CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_LISTVIEW,
        TEXT("Blub"),
        WS_TABSTOP | WS_CHILD | WS_VISIBLE | LVS_AUTOARRANGE | LVS_REPORT | LVS_OWNERDATA,
        0,
        cyChar * 2,
        parent_rect.right - parent_rect.left,
        parent_rect.bottom - cyChar * 2,
        parent_hwnd,
        list_view_id,
        m_hInstance,
        0
    );
    if (!m_hListView) {
        MessageBox(nullptr, TEXT("Error creating window ui element"), m_windowTitle, MB_ICONERROR);
        return -1;
    }

    TCHAR column_name1[] = TEXT("Name");
    TCHAR column_name2[] = TEXT("Checksum");
    TCHAR column_name3[] = TEXT("Status");
    struct {
        TCHAR* name;
        int width;
    } columns[] = { { column_name1, 150 }, { column_name2, 110 }, { column_name3, 105 } };
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

    // Customize ListView header drawing to draw sort glyphs
    SetWindowSubclass(ListView_GetHeader(m_hListView),
        [](HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) -> LRESULT {
            if ((uMsg == WM_PAINT) && uIdSubclass == MainWindow::ListViewSubclassId) {
                MainWindow* main_window = std::bit_cast<MainWindow*>(dwRefData);
                return main_window->paintListViewHeader(hWnd, uMsg, wParam, lParam);
            }
            return DefSubclassProc(hWnd, uMsg, wParam, lParam);
        }, ListViewSubclassId, std::bit_cast<DWORD_PTR>(this));

    m_hPopupMenu = LoadMenu(m_hInstance, MAKEINTRESOURCE(IDR_MENU_POPUP));
    if (!m_hPopupMenu) {
        MessageBox(nullptr, TEXT("Error creating popup menu"), m_windowTitle, MB_ICONERROR);
        return -1;
    }

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

void MainWindow::addListEntry(std::u16string name, std::u16string checksum, ListViewEntry::Status status, std::u16string absolute_file_path) {
    m_listEntries.push_back(ListViewEntry{
        .name = std::move(name),
        .checksum = std::move(checksum),
        .status = status,
        .original_position = static_cast<uint32_t>(m_listEntries.size()),
        .absolute_file_path = std::move(absolute_file_path)
    });
    ListView_SetItemCountEx(m_hListView, m_listEntries.size(), 0);
}

HWND MainWindow::getHwnd() const {
    return m_hWnd;
}

HasherOptions MainWindow::getOptions() const {
    return m_options;
}

void MainWindow::onCheckStarted(uint32_t n_files) {
    ListView_DeleteAllItems(m_hListView);
    m_listEntries.clear();
    quicker_sfv::Version const v = quicker_sfv::getVersion();
    addListEntry(formatString(50, TEXT("QuickerSFV v%d.%d.%d"), v.major, v.minor, v.patch));
    m_stats = Stats{ .total = n_files };
    UpdateStats();
}

void MainWindow::onFileStarted(std::u8string_view file, std::u8string_view absolute_file_path) {
    UNREFERENCED_PARAMETER(file);
    UNREFERENCED_PARAMETER(absolute_file_path);
}

void MainWindow::onProgress(uint32_t percentage, uint32_t bandwidth_mib_s) {
    m_stats.progress = percentage;
    m_stats.bandwidth = bandwidth_mib_s;
    UpdateStats();
}

void MainWindow::onFileCompleted(std::u8string_view file, Digest const& checksum, std::u8string_view absolute_file_path, CompletionStatus status) {
    ++m_stats.completed;
    m_stats.progress = 0;
    m_stats.bandwidth = 0;
    switch (status) {
    case CompletionStatus::Ok:
        addListEntry(convertToUtf16(file), convertToUtf16(checksum.toString()), ListViewEntry::Status::Ok, convertToUtf16(absolute_file_path));
        ++m_stats.ok;
        break;
    case CompletionStatus::Missing:
        addListEntry(convertToUtf16(file), {}, ListViewEntry::Status::FailedMissing, convertToUtf16(absolute_file_path));
        ++m_stats.missing;
        break;
    case CompletionStatus::Bad:
        addListEntry(convertToUtf16(file), convertToUtf16(checksum.toString()), ListViewEntry::Status::FailedMismatch, convertToUtf16(absolute_file_path));
        ++m_stats.bad;
        break;
    }
    ListView_EnsureVisible(m_hListView, m_listEntries.size() - 1, FALSE);
    UpdateStats();
}

void MainWindow::onCheckCompleted(Result r) {
    m_stats.ok = r.ok;
    m_stats.bad = r.bad;
    m_stats.missing = r.missing;
    m_stats.completed = r.ok + r.bad + r.missing;
    m_stats.progress = 0;
    m_stats.bandwidth = 0;
    addListEntry(formatString(30, TEXT("%d files checked"), m_stats.completed));
    if ((m_stats.missing == 0) && (m_stats.bad == 0)) {
        addListEntry(u"All files OK", {}, ListViewEntry::Status::MessageOk);
    } else {
        std::u16string msg;
        if (m_stats.bad > 0) {
            msg = assumeUtf16((m_stats.bad > 1) ? TEXT("There were ") : TEXT("There was "));
        } else {
            msg = assumeUtf16((m_stats.missing > 1) ? TEXT("There were ") : TEXT("There was "));
        }
        if (m_stats.bad > 0) {
            msg += formatString(40, TEXT("%d bad file%s"), m_stats.bad, ((m_stats.bad == 1) ? TEXT("") : TEXT("s")));
            if (m_stats.missing > 0) { msg += assumeUtf16(TEXT(" and ")); }
        }
        if (m_stats.missing > 0) {
            msg += formatString(40, TEXT("%d missing file%s"), m_stats.missing, ((m_stats.missing == 1) ? TEXT("") : TEXT("s")));
        }
        addListEntry(msg, {}, ListViewEntry::MessageBad);
    }
    ListView_EnsureVisible(m_hListView, m_listEntries.size() - 1, FALSE);
    UpdateStats();
    if (!m_hWnd) {
        // we're running in no-gui mode; write results to file and quit
        writeResultsToFile();
        PostQuitMessage(0);
    }
}

void MainWindow::onCancelRequested() {
}

void MainWindow::onCanceled() {
}

void MainWindow::onError(quicker_sfv::Error error, std::u8string_view msg) {
    UNREFERENCED_PARAMETER(error);
    addListEntry(formatString(255, TEXT("ERROR: %s (%d)"), convertToUtf16(msg).c_str(), static_cast<int>(error)), {}, ListViewEntry::MessageBad);
}

void MainWindow::setOutFile(std::u16string out_file) {
    m_outFile = std::move(out_file);
}

void MainWindow::writeResultsToFile() const {
    // all operations in here are best effort, as we have no sensible way to report errors
    HANDLE fout = CreateFile(toWcharStr(m_outFile.c_str()), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                             CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (fout == INVALID_HANDLE_VALUE) {
        return;
    }
    for (auto const& e : m_listEntries) {
        std::u8string msg;
        if ((e.status == ListViewEntry::Status::Ok) ||
            (e.status == ListViewEntry::Status::FailedMissing) ||
            (e.status == ListViewEntry::Status::FailedMismatch))
        {
            msg = convertToUtf8(e.name) + u8": " +
                convertToUtf8(e.checksum) + u8":  " +
                convertToUtf8(assumeUtf16(getStatusTextForStatus(e.status)));
        } else {
            msg = convertToUtf8(e.name);
        }
        msg.push_back(u8'\r');
        msg.push_back(u8'\n');
        if (!WriteFile(fout, msg.data(), static_cast<DWORD>(msg.size()), nullptr, nullptr)) { break; }
    }
    CloseHandle(fout);
}

void MainWindow::UpdateStats() {
    TCHAR buffer[50];
    constexpr size_t const buffer_size = ARRAYSIZE(buffer);
    if (m_stats.progress == 0) {
        Static_SetText(m_hTextFieldLeft, formatString(buffer, buffer_size, TEXT("Completed files: %d/%d\nOk: %d"), m_stats.completed, m_stats.total, m_stats.ok));
    } else {
        Static_SetText(m_hTextFieldLeft, formatString(buffer, buffer_size, TEXT("Completed files: %d/%d (File: %d%% %dMiB/s)\nOk: %d"), m_stats.completed, m_stats.total, m_stats.progress, m_stats.bandwidth, m_stats.ok));
    }
    Static_SetText(m_hTextFieldRight, formatString(buffer, buffer_size, TEXT("Bad: %d\nMissing: %d"), m_stats.bad, m_stats.missing));
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

    TCHAR const class_name[] = TEXT("quicker_sfv");
    TCHAR const window_title[] = TEXT("QuickerSFV");

    gui::CommandLineOptions const command_line_opts = gui::parseCommandLine(assumeUtf8(lpCmdLine));
    bool const no_gui_window = !command_line_opts.outFile.empty();

    FileProviders file_providers;
    file_providers.loadPlugins();

    ULONG_PTR gdiplus_token;
    Gdiplus::GdiplusStartupInput gdiplus_startup_in;
    Gdiplus::GdiplusStartupOutput gdiplus_startup_out;
    if (Gdiplus::GdiplusStartup(&gdiplus_token, &gdiplus_startup_in, &gdiplus_startup_out) != Gdiplus::Status::Ok) {
        MessageBox(nullptr, TEXT("Error initializing GDI+"), window_title, MB_ICONERROR | MB_OK);
        return 0;
    }

    if (!no_gui_window) {
        INITCOMMONCONTROLSEX init_cc_ex{
            .dwSize = sizeof(INITCOMMONCONTROLSEX),
            .dwICC = ICC_LINK_CLASS
        };
        if (!InitCommonControlsEx(&init_cc_ex)) {
            return 0;
        }

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
    }

    OperationScheduler scheduler;
    MainWindow main_window(file_providers, scheduler);
    HACCEL hAccelerators = nullptr;
    if (no_gui_window) {
        main_window.setOutFile(command_line_opts.outFile);
    } else {
        if (!main_window.createMainWindow(hInstance, nCmdShow, class_name, window_title)) {
            return 0;
        }
        hAccelerators = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));
        if (!hAccelerators) {
            return 0;
        }
    }

    scheduler.start();

    for (auto const& f : command_line_opts.filesToCheck) {
        auto* p = file_providers.getMatchingProviderFor(convertToUtf8(f), false);
        if (!p) {
            auto const msg = formatString(1111, TEXT("Cannot determine format for filename: \"%s\""), f.c_str());
            MessageBox(main_window.getHwnd(), toWcharStr(msg), window_title, MB_ICONERROR | MB_OK);
            continue;
        }
        scheduler.post(Operation::Verify{
            .event_handler = &main_window,
            .options = main_window.getOptions(),
            .source_file = f,
            .provider = p
        });
    }

    MSG message;
    for (;;) {
        scheduler.run();
        BOOL const bret = GetMessage(&message, nullptr, 0, 0);
        if (bret == 0) {
            break;
        } else if (bret == -1) {
            MessageBox(nullptr, TEXT("Error in GetMessage"), window_title, MB_ICONERROR);
            return 0;
        } else {
            if (!hAccelerators || !TranslateAccelerator(main_window.getHwnd(), hAccelerators, &message))
            {
                TranslateMessage(&message);
                DispatchMessage(&message);
            }
        }
    }
    scheduler.shutdown();
    Gdiplus::GdiplusShutdown(gdiplus_token);
    return static_cast<int>(message.wParam);
}
