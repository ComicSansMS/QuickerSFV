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
#include <quicker_sfv/quicker_sfv.hpp>

#include <quicker_sfv/ui/command_line_parser.hpp>
#include <quicker_sfv/ui/enforce.hpp>
#include <quicker_sfv/ui/event_handler.hpp>
#include <quicker_sfv/ui/file_dialog.hpp>
#include <quicker_sfv/ui/operation_scheduler.hpp>
#include <quicker_sfv/ui/plugin_support.hpp>
#include <quicker_sfv/ui/resource_guard.hpp>
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
            HMODULE hmod = LoadLibraryEx(find_data.cFileName, nullptr, LOAD_LIBRARY_SEARCH_APPLICATION_DIR);
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

    struct WindowPlacementConfig {
        int32_t pos_x;
        int32_t pos_y;
        int32_t width;
        int32_t height;
        int32_t name_column_width;
        int32_t checksum_column_width;
        int32_t status_column_width;
    };

    bool m_saveConfigToRegistry;
public:
    explicit MainWindow(FileProviders& file_providers, OperationScheduler& scheduler);

    ~MainWindow();

    MainWindow(MainWindow const&) = delete;
    MainWindow(MainWindow&&) = delete;
    MainWindow& operator=(MainWindow const&) = delete;
    MainWindow& operator=(MainWindow&&) = delete;

    void createMainWindow(HINSTANCE hInstance, int nCmdShow,
                          TCHAR const* class_name, TCHAR const* window_title);

    LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND getHwnd() const;
    HasherOptions getOptions() const;

    void onOperationStarted(uint32_t n_files) override;
    void onFileStarted(std::u8string_view file, std::u8string_view absolute_file_path) override;
    void onProgress(uint32_t percentage, uint32_t bandwidth_mib_s) override;
    void onFileCompleted(std::u8string_view file, Digest const& checksum, std::u8string_view absolute_file_path, CompletionStatus status) override;
    void onOperationCompleted(Result r) override;
    void onCanceled() override;
    void onError(quicker_sfv::Error error, std::u8string_view msg) override;

    void setOutFile(std::u16string out_file);
    void writeResultsToFile() const;

    void setOptionUseAvx512(bool use_avx512);
    void setOptionSaveConfiguration(bool save_config);

    void loadConfigurationFromRegistry();
    void saveConfigurationToRegistry();
private:
    void createUiElements(HWND parent_hwnd);

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
    std::vector<ListViewEntry const*> getMarkedFilesForDeletion();
    void doDeleteMarkedFiles();

    static TCHAR const* getStatusTextForStatus(ListViewEntry::Status s, std::u16string_view checksum);
};

MainWindow::MainWindow(FileProviders& file_providers, OperationScheduler& scheduler)
    :m_hInstance(nullptr), m_windowTitle(nullptr), m_hWnd(nullptr), m_hMenu(nullptr), m_hTextFieldLeft(nullptr),
     m_hTextFieldRight(nullptr), m_hListView(nullptr), m_imageList(nullptr), m_hPopupMenu(nullptr),
     m_stats{}, m_listSort{ .sort_column = 0, .order = ListViewSort::Order::Original },
     m_options{ .has_sse42 = quicker_sfv::supportsSse42(), .has_avx512 = false},
     m_fileProviders(&file_providers), m_scheduler(&scheduler), m_saveConfigToRegistry(false)
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
        if (m_saveConfigToRegistry) { saveConfigurationToRegistry(); }
        PostQuitMessage(0);
        return 0;
    } else if (msg == WM_CREATE) {
        createUiElements(hWnd);
        return 0;
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
                setOptionUseAvx512(!m_options.has_avx512);
            } else if (LOWORD(wParam) == ID_OPTIONS_SAVECONFIGURATION) {
                setOptionSaveConfiguration(!m_saveConfigToRegistry);
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

TCHAR const* MainWindow::getStatusTextForStatus(ListViewEntry::Status s, std::u16string_view checksum) {
    switch (s) {
    case ListViewEntry::Ok:
        return TEXT("OK");
    case ListViewEntry::FailedMismatch:
        if (checksum.empty()) {
            return TEXT("FAILED. Unable to read file");
        } else {
            return TEXT("FAILED. Checksum mismatch");
        }
    case ListViewEntry::FailedMissing:
        return TEXT("FAILED. File does not exist");
    default:
        return TEXT("");
    }
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
                           getStatusTextForStatus(entry.status, entry.checksum), _TRUNCATE);
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
        // context menu
        POINT mouse_cursor;
        if (!GetCursorPos(&mouse_cursor)) { return 0; }
        EnableMenuItem(m_hPopupMenu, ID_CONTEXTMENU_COPY,
            MF_BYCOMMAND | ((getSelectedListViewItems().empty()) ? MF_DISABLED : MF_ENABLED));
        EnableMenuItem(m_hPopupMenu, ID_CONTEXTMENU_DELETEMARKEDFILES,
            MF_BYCOMMAND | ((getMarkedFilesForDeletion().empty()) ? MF_DISABLED : MF_ENABLED));
        if (!TrackPopupMenu(GetSubMenu(m_hPopupMenu, 0), TPM_RIGHTBUTTON,
                            mouse_cursor.x, mouse_cursor.y, 0, m_hWnd, nullptr)) {
            return 0;
        }
    }
    return 0;
}

LRESULT MainWindow::paintListViewHeader(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (m_listSort.order == ListViewSort::Order::Original) {
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    // first draw the entirety of the header control as normal, but redirect drawing to a back buffer to combat flickering
    RECT header_rect;
    if (!GetClientRect(hWnd, &header_rect)) { return 0; }
    if (!InvalidateRect(hWnd, &header_rect, TRUE)) { return 0; }

    HDC hdc = GetDC(hWnd);
    if (!hdc) { return 0; }
    ResourceGuard guard_hdc(hdc, [hWnd](HDC h) { ReleaseDC(hWnd, h); });
    int const header_width = header_rect.right - header_rect.left;
    int const header_height = header_rect.bottom - header_rect.top;
    HBITMAP back_buffer_bmp = CreateCompatibleBitmap(hdc, header_width, header_height);
    if (!back_buffer_bmp) { return 0; }
    ResourceGuard guard_back_buffer_bmp(back_buffer_bmp, DeleteObject);
    HDC back_buffer_dc = CreateCompatibleDC(hdc);
    if (!back_buffer_dc) { return 0; }
    ResourceGuard guard_back_buffer_dc(back_buffer_dc, DeleteDC);

    if (!SelectObject(back_buffer_dc, back_buffer_bmp)) { return 0; }
    DefSubclassProc(hWnd, uMsg, std::bit_cast<WPARAM>(back_buffer_dc), lParam);

    // then draw the glyph over it in the back buffer
    RECT rect;
    Header_GetItemRect(hWnd, m_listSort.sort_column, &rect);
    Gdiplus::Rect update(rect.left, rect.top, (rect.right - rect.left), (rect.bottom - rect.top));

    Gdiplus::Point const top_midpoint((rect.right - rect.left) / 2 + rect.left, rect.top);
    INT const arrow_width = 10;
    INT const arrow_height = 5;
    Gdiplus::Point points[3];
    if (m_listSort.order == ListViewSort::Order::Ascending) {
        points[0] = top_midpoint + Gdiplus::Point(-(arrow_width / 2), arrow_height);
        points[1] = top_midpoint;
        points[2] = top_midpoint + Gdiplus::Point((arrow_width / 2), arrow_height);
    } else {
        points[0] = top_midpoint + Gdiplus::Point(-(arrow_width / 2), 0);
        points[1] = top_midpoint + Gdiplus::Point(0, arrow_height);
        points[2] = top_midpoint + Gdiplus::Point((arrow_width / 2), 0);
    }

    Gdiplus::Graphics gdi(back_buffer_dc);
    if (gdi.SetClip(update) != Gdiplus::Status::Ok) { return 0; }
    Gdiplus::Pen p(Gdiplus::Color(255, 92, 92, 92));
    if (gdi.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias8x8) != Gdiplus::Status::Ok) { return 0; }
    if (gdi.DrawLines(&p, points, 3) != Gdiplus::Status::Ok) { return 0; }

    // finally, blit the entire back buffer into the client window
    if (!BitBlt(hdc, header_rect.left, header_rect.top, header_width, header_height, back_buffer_dc, 0, 0, SRCCOPY)) {
        return 0;
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
    size_t const total_string_length = std::accumulate(begin(selected_items), end(selected_items), std::size_t{ 1 },
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

std::vector<MainWindow::ListViewEntry const*> MainWindow::getMarkedFilesForDeletion() {
    std::vector<ListViewEntry const*> selected_items = getSelectedListViewItems();
    selected_items.erase(
        std::remove_if(begin(selected_items), end(selected_items), [](ListViewEntry const* e) {
                return (e->status != ListViewEntry::Status::Ok) &&
                       (e->status != ListViewEntry::Status::FailedMismatch);
            }), end(selected_items));
    return selected_items;
}

void MainWindow::doDeleteMarkedFiles() {
    std::vector<ListViewEntry const*> const selected_items = getMarkedFilesForDeletion();
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

void MainWindow::createMainWindow(HINSTANCE hInstance, int nCmdShow,
                                  TCHAR const* class_name, TCHAR const* window_title)
{
    m_hInstance = hInstance;
    m_windowTitle = window_title;
    m_hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU1));
    if (!m_hMenu) { throwException(Error::SystemError); }
    if (quicker_sfv::supportsAvx512()) {
        MENUITEMINFO mii{ .cbSize = sizeof(MENUITEMINFO), .fMask = MIIM_STATE, .fState = MFS_ENABLED | MFS_CHECKED };
        SetMenuItemInfo(m_hMenu, ID_OPTIONS_USEAVX512, FALSE, &mii);
        m_options.has_avx512 = true;
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
        DestroyMenu(m_hMenu);
        m_hMenu = nullptr;
        throwException(Error::SystemError);
    }

    SetWindowLongPtr(m_hWnd, 0, std::bit_cast<LONG_PTR>(this));

    ShowWindow(m_hWnd, nCmdShow);
    if (!UpdateWindow(m_hWnd)) {
        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
        DestroyMenu(m_hMenu);
        m_hMenu = nullptr;
        throwException(Error::SystemError);
    }
}

void MainWindow::createUiElements(HWND parent_hwnd) {
    RECT parent_rect;
    if (!GetWindowRect(parent_hwnd, &parent_rect)) { throwException(Error::SystemError); }
    WORD const cyChar = HIWORD(GetDialogBaseUnits());
    HMENU const field_left_id =
#ifdef _WIN64
        std::bit_cast<HMENU>(0x123ull);
#else
        std::bit_cast<HMENU>(0x123u);
#endif
    m_hTextFieldLeft = CreateWindowW(WC_STATIC,
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
    if (!m_hTextFieldLeft) { throwException(Error::SystemError); }

    HMENU const field_right_id =
#ifdef _WIN64
        std::bit_cast<HMENU>(0x124ull);
#else
        std::bit_cast<HMENU>(0x124u);
#endif
    m_hTextFieldRight = CreateWindow(WC_STATIC,
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
    if (!m_hTextFieldRight) { throwException(Error::SystemError); }

    HMENU const list_view_id =
#ifdef _WIN64
        std::bit_cast<HMENU>(0x125ull);
#else
        std::bit_cast<HMENU>(0x125u);
#endif
    m_hListView = CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_LISTVIEW,
        TEXT(""),
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
    if (!m_hListView) { throwException(Error::SystemError); }

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
    if (!m_imageList) { throwException(Error::SystemError); }

    for (int i = 0; i < number_of_icons; ++i) {
        HANDLE hicon = LoadImage(m_hInstance, MAKEINTRESOURCE(icon_id_list[i]), IMAGE_ICON, icon_size_x, icon_size_y, LR_DEFAULTCOLOR);
        if (!hicon) { throwException(Error::SystemError); }
        if (ImageList_AddIcon(m_imageList, static_cast<HICON>(hicon)) != i) { throwException(Error::SystemError); }
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
    if (!m_hPopupMenu) { throwException(Error::SystemError); }
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

void MainWindow::onOperationStarted(uint32_t n_files) {
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

void MainWindow::onOperationCompleted(Result r) {
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
    HandleGuard guard_fout(fout);
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
                convertToUtf8(assumeUtf16(getStatusTextForStatus(e.status, e.checksum)));
        } else {
            msg = convertToUtf8(e.name);
        }
        msg.push_back(u8'\r');
        msg.push_back(u8'\n');
        if (!WriteFile(fout, msg.data(), static_cast<DWORD>(msg.size()), nullptr, nullptr)) { break; }
    }
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

void MainWindow::setOptionUseAvx512(bool use_avx512) {
    MENUITEMINFO mii{ .cbSize = sizeof(MENUITEMINFO), .fMask = MIIM_STATE };
    GetMenuItemInfo(m_hMenu, ID_OPTIONS_USEAVX512, FALSE, &mii);
    if (use_avx512) {
        mii.fState |= MFS_CHECKED;
    } else {
        mii.fState &= ~MFS_CHECKED;
    }
    SetMenuItemInfo(m_hMenu, ID_OPTIONS_USEAVX512, FALSE, &mii);
    m_options.has_avx512 = use_avx512;
}

void MainWindow::setOptionSaveConfiguration(bool save_config) {
    if (!save_config) {
        if (MessageBox(m_hWnd, TEXT("Do you want to remove the current saved configuration?"), TEXT("QuickerSFV"),
                       MB_ICONQUESTION | MB_YESNO) == IDYES) {
            HKEY reg_key;
            if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software"), 0, KEY_SET_VALUE | DELETE, &reg_key) == ERROR_SUCCESS) {
                RegDeleteTree(reg_key, TEXT("QuickerSFV"));
                RegCloseKey(reg_key);
            }
        }
    }
    MENUITEMINFO mii{ .cbSize = sizeof(MENUITEMINFO), .fMask = MIIM_STATE };
    GetMenuItemInfo(m_hMenu, ID_OPTIONS_SAVECONFIGURATION, FALSE, &mii);
    if (save_config) {
        mii.fState |= MFS_CHECKED;
    } else {
        mii.fState &= ~MFS_CHECKED;
    }
    SetMenuItemInfo(m_hMenu, ID_OPTIONS_SAVECONFIGURATION, FALSE, &mii);
    m_saveConfigToRegistry = save_config;
}

void MainWindow::loadConfigurationFromRegistry() {
    HKEY reg_key;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\QuickerSFV"), 0, KEY_WRITE | KEY_READ, &reg_key) != ERROR_SUCCESS) {
        // no registry key found
        return;
    }
    ResourceGuard guar_reg_key(reg_key, RegCloseKey);
    setOptionSaveConfiguration(true);
    WindowPlacementConfig placement{};
    DWORD size = sizeof(placement);
    if ((RegGetValue(reg_key, nullptr, TEXT("WindowDimensions"), RRF_RT_REG_BINARY, nullptr, &placement, &size) == ERROR_SUCCESS) &&
        (size == sizeof(placement))) {
        SetWindowPos(m_hWnd, nullptr, placement.pos_x, placement.pos_y, placement.width, placement.height, SWP_ASYNCWINDOWPOS);
        ListView_SetColumnWidth(m_hListView, 0, placement.name_column_width);
        ListView_SetColumnWidth(m_hListView, 1, placement.checksum_column_width);
        ListView_SetColumnWidth(m_hListView, 2, placement.status_column_width);
    }
    if (m_options.has_avx512) {
        DWORD use_avx;
        size = sizeof(DWORD);
        if ((RegGetValue(reg_key, nullptr, TEXT("UseAvx"), RRF_RT_REG_DWORD, nullptr, &use_avx, &size) == ERROR_SUCCESS) &&
            (size == sizeof(DWORD))) {
            if (use_avx == 1) {
                setOptionUseAvx512(true);
            } else if (use_avx == 0) {
                setOptionUseAvx512(false);
            }
        }
    }
}

void MainWindow::saveConfigurationToRegistry() {
    HKEY reg_key;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\QuickerSFV"), 0, nullptr, 0, KEY_WRITE, nullptr, &reg_key, nullptr) != ERROR_SUCCESS) {
        return;
    }
    ResourceGuard guard_reg_key(reg_key, RegCloseKey);
    if (RECT rect; GetWindowRect(m_hWnd, &rect)) {
        WindowPlacementConfig const placement{
            .pos_x = rect.left,
            .pos_y = rect.top,
            .width = (rect.right - rect.left),
            .height = (rect.bottom - rect.top),
            .name_column_width = ListView_GetColumnWidth(m_hListView, 0),
            .checksum_column_width = ListView_GetColumnWidth(m_hListView, 1),
            .status_column_width = ListView_GetColumnWidth(m_hListView, 2),
        };
        RegSetValueEx(reg_key, TEXT("WindowDimensions"), 0, REG_BINARY, reinterpret_cast<BYTE const*>(&placement), sizeof(placement));
    }
    DWORD use_avx = (m_options.has_avx512) ? 1 : 0;
    RegSetValueEx(reg_key, TEXT("UseAvx"), 0, REG_DWORD, reinterpret_cast<BYTE const*>(&use_avx), sizeof(DWORD));
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

int WinMainImpl(HINSTANCE hInstance, LPSTR lpCmdLine, int nCmdShow)
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
        throwException(Error::SystemError);
    }

    if (!no_gui_window) {
        INITCOMMONCONTROLSEX init_cc_ex{
            .dwSize = sizeof(INITCOMMONCONTROLSEX),
            .dwICC = ICC_LINK_CLASS
        };
        if (!InitCommonControlsEx(&init_cc_ex)) {
            throwException(Error::SystemError);
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
            throwException(Error::SystemError);
        }
    }

    OperationScheduler scheduler;
    MainWindow main_window(file_providers, scheduler);
    HACCEL hAccelerators = nullptr;
    if (no_gui_window) {
        main_window.setOutFile(command_line_opts.outFile);
    } else {
        main_window.createMainWindow(hInstance, nCmdShow, class_name, window_title);
        hAccelerators = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));
        if (!hAccelerators) {
            throwException(Error::SystemError);
        }
    }
    main_window.loadConfigurationFromRegistry();

    scheduler.start();

    for (auto const& f : command_line_opts.filesToCheck) {
        auto* p = file_providers.getMatchingProviderFor(convertToUtf8(f), false);
        if (!p) {
            auto const msg = formatString(f.size() + 64, TEXT("Cannot determine format for filename: \"%s\""), f.c_str());
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
            throwException(Error::SystemError);
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

int APIENTRY WinMain(HINSTANCE hInstance,
    HINSTANCE /* hPrevInstance */,
    LPSTR     lpCmdLine,
    int       nCmdShow) {
    try {
        return WinMainImpl(hInstance, lpCmdLine, nCmdShow);
    } catch (Exception& e) {
        MessageBoxA(nullptr, toStr(std::u8string(e.what8())), "QuickerSFV", MB_ICONERROR | MB_OK);
    }
    return 0;
}
