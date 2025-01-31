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
#ifndef WIN32_LEAN_AND_MEAN
#   define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#   define NOMINMAX
#endif
#include <Windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <uxtheme.h>
#include <tchar.h>

#include <resource.h>

#include <cstring>
#include <bit>
#include <optional>
#include <memory>
#include <utility>
#include <vector>

class MainWindow {
private:
    HINSTANCE m_hInstance;
    TCHAR const* m_windowTitle;
    HWND m_hWnd;
    HWND m_hTextFieldLeft;
    HWND m_hTextFieldRight;
    HWND m_hListView;
    struct ListViewEntry {
        std::unique_ptr<TCHAR[]> name;
        std::unique_ptr<TCHAR[]> crc;
        bool status_ok;
    };
    std::vector<ListViewEntry> m_listEntries;
private:
    explicit MainWindow(HINSTANCE hInstance);
public:
    MainWindow();

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

MainWindow::MainWindow()
    :m_hInstance(nullptr), m_windowTitle(nullptr), m_hWnd(nullptr), m_hTextFieldLeft(nullptr), m_hTextFieldRight(nullptr),
     m_hListView(nullptr)
{
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
        } else if (disp_info->item.mask & LVIF_IMAGE) {
            // @todo
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
    HMENU hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU1));
    if (!hMenu) {
        MessageBox(nullptr, TEXT("Error creating menu"), window_title, MB_ICONERROR);
        return FALSE;
    }

    static_assert((sizeof(MainWindow*) == sizeof(LPARAM)) && (sizeof(MainWindow*) == sizeof(LPVOID)));
    m_hWnd = CreateWindow(
        class_name,
        m_windowTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        688,
        520,
        nullptr,
        hMenu,
        hInstance,
        std::bit_cast<LPVOID>(this)
    );
    if (!m_hWnd) {
        MessageBox(nullptr, TEXT("Error creating main window"), m_windowTitle, MB_ICONERROR);
        DestroyMenu(hMenu);
        return FALSE;
    }

    SetWindowLongPtr(m_hWnd, 0, std::bit_cast<LONG_PTR>(this));

    ShowWindow(m_hWnd, nCmdShow);
    if (!UpdateWindow(m_hWnd)) {
        MessageBox(nullptr, TEXT("Error updating main window"), m_windowTitle, MB_ICONERROR);
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
    } columns[] = { { column_name1, 305 }, { column_name2, 192 }, { column_name3, 120 } };
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

    MainWindow main_window;
    if (!main_window.createMainWindow(hInstance, nCmdShow, class_name, window_title)) {
        return 0;
    }

    MSG message;
    for (;;) {
        BOOL bret = GetMessage(&message, nullptr, 0, 0);
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

