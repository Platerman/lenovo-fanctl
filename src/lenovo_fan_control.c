#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <pthread.h>
#include <Windows.h>
#include <shellapi.h>
#include <tchar.h>

#include "fanctrl.h"

#define WM_TRAYICON (WM_USER + 1)

enum TrayMenuIDs {
    ID_TRAY_APP_ICON = 1001,
    ID_TRAY_STATE,
    ID_TRAY_HIGH_SPEED,
    ID_TRAY_NORMAL_SPEED,
    ID_TRAY_EXIT,
};

enum HotKeyIDs {
    HOTKEY_HIGH_SPEED,
    HOTKEY_NORMAL_SPEED,
};

typedef struct {
    LPCWSTR app_name;
    LPCWSTR note;
    LPCWSTR program_is_running;
    LPCWSTR failed_to_open_driver;
    LPCWSTR state;
    LPCWSTR menu_at_high_speed;
    LPCWSTR menu_at_normal_speed;
    LPCWSTR menu_high_speed;
    LPCWSTR menu_normal_speed;
    LPCWSTR menu_exit;
} LangResources;

const LangResources lang = {
    TEXT("Lenovo Fan Control"),
    TEXT("Note"),
    TEXT("The program is running."),
    TEXT("Failed to open \\\\.\\EnergyDrv. Unsupported device or driver not available."),
    TEXT("State"),
    TEXT("High Speed"),
    TEXT("Normal Speed"),
    TEXT("High Speed\tCtrl+Alt+F11"),
    TEXT("Normal Speed\tCtrl+Alt+F12"),
    TEXT("Exit"),
};

NOTIFYICONDATA nid;
HMENU hMenu;
pthread_t worker_thread;
int worker_active = 0;

enum FanSpeed {
    HIGH_SPEED,
    NORMAL_SPEED
} fan_speed_set_at_start = HIGH_SPEED;

static void* keep_fan_running_func(void *arg) {
    (void)arg;
    keep_fan_running();
    return NULL;
}

static void stop_worker(void) {
    is_keep_fan_running = 0;
    if (worker_active) {
        pthread_join(worker_thread, NULL);
        worker_active = 0;
    }
}

static void set_menu_state(LPCWSTR text) {
    ModifyMenu(hMenu, ID_TRAY_STATE, MF_STRING | MF_DISABLED, ID_TRAY_STATE, text);
}

static void update_tip(LPCWSTR state) {
    _sntprintf(nid.szTip, 128, L"%s\n%s: %s",
               lang.app_name, lang.state, state);
    nid.szTip[127] = 0;
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

static void apply_normal_state(void) {
    fan_control(NORMAL);
    set_menu_state(lang.menu_at_normal_speed);
    update_tip(lang.menu_at_normal_speed);
}

static void toggle_fan_high_speed(void) {
    stop_worker();
    is_keep_fan_running = 1;
    if (pthread_create(&worker_thread, NULL, keep_fan_running_func, NULL) != 0) {
        is_keep_fan_running = 0;
        apply_normal_state();
        return;
    }
    worker_active = 1;
    set_menu_state(lang.menu_at_high_speed);
    update_tip(lang.menu_at_high_speed);
}

static void toggle_fan_normal_speed(void) {
    stop_worker();
    apply_normal_state();
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            nid.cbSize = sizeof(NOTIFYICONDATA);
            nid.hWnd = hwnd;
            nid.uID = ID_TRAY_APP_ICON;
            nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            nid.uCallbackMessage = WM_TRAYICON;
            nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
            _tcscpy(nid.szTip, lang.app_name);
            Shell_NotifyIcon(NIM_ADD, &nid);

            hMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_STRING | MF_DISABLED, ID_TRAY_STATE, lang.menu_at_high_speed);
            AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(hMenu, MF_STRING, ID_TRAY_HIGH_SPEED, lang.menu_high_speed);
            AppendMenu(hMenu, MF_STRING, ID_TRAY_NORMAL_SPEED, lang.menu_normal_speed);
            AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, lang.menu_exit);

            switch (fan_speed_set_at_start) {
                case NORMAL_SPEED:
                    toggle_fan_normal_speed();
                    break;
                case HIGH_SPEED:
                default:
                    toggle_fan_high_speed();
                    break;
            }
            break;
        }

        case WM_TRAYICON: {
            if (lParam == WM_LBUTTONUP || lParam == WM_RBUTTONUP) {
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hwnd);
                TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN,
                               pt.x, pt.y, 0, hwnd, NULL);
                PostMessage(hwnd, WM_NULL, 0, 0);
            }
            break;
        }

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case ID_TRAY_EXIT:
                    DestroyWindow(hwnd);
                    break;
                case ID_TRAY_HIGH_SPEED:
                    toggle_fan_high_speed();
                    break;
                case ID_TRAY_NORMAL_SPEED:
                    toggle_fan_normal_speed();
                    break;
            }
            break;
        }

        case WM_HOTKEY:
            switch (wParam) {
                case HOTKEY_HIGH_SPEED:
                    toggle_fan_high_speed();
                    break;
                case HOTKEY_NORMAL_SPEED:
                    toggle_fan_normal_speed();
                    break;
            }
            break;

        case WM_DESTROY:
            UnregisterHotKey(hwnd, HOTKEY_HIGH_SPEED);
            UnregisterHotKey(hwnd, HOTKEY_NORMAL_SPEED);
            stop_worker();
            fan_control(NORMAL);
            Shell_NotifyIcon(NIM_DELETE, &nid);
            if (hMenu) {
                DestroyMenu(hMenu);
                hMenu = NULL;
            }
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    int args = 0;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &args);
    if (argv) {
        for (int i = 1; i < args; ++i) {
            if (_tcscmp(argv[i], TEXT("--normal-speed")) == 0) {
                fan_speed_set_at_start = NORMAL_SPEED;
            } else if (_tcscmp(argv[i], TEXT("--high-speed")) == 0) {
                fan_speed_set_at_start = HIGH_SPEED;
            }
        }
        LocalFree(argv);
    }

    HANDLE hMutex = CreateMutex(NULL, TRUE, TEXT("LenovoFanControlMutex"));
    if (hMutex == NULL) {
        MessageBox(NULL, TEXT("Failed to create mutex."), lang.app_name,
                   MB_OK | MB_ICONEXCLAMATION);
        return 1;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(hMutex);
        MessageBox(NULL, lang.program_is_running, lang.note,
                   MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    if (read_state() == -1) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
        MessageBox(NULL, lang.failed_to_open_driver, lang.app_name,
                   MB_ICONEXCLAMATION | MB_OK);
        return 1;
    }

    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = TEXT("LenovoFanControlClass");

    if (!RegisterClassEx(&wc)) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
        MessageBox(NULL, TEXT("Window Registration Failed!"), TEXT("Error"),
                   MB_ICONEXCLAMATION | MB_OK);
        return 1;
    }

    HWND hwnd = CreateWindowEx(0, TEXT("LenovoFanControlClass"), lang.app_name,
                               0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
    if (hwnd == NULL) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
        MessageBox(NULL, TEXT("Window Creation Failed!"), TEXT("Error"),
                   MB_ICONEXCLAMATION | MB_OK);
        return 1;
    }

    RegisterHotKey(hwnd, HOTKEY_HIGH_SPEED, MOD_CONTROL | MOD_ALT, VK_F11);
    RegisterHotKey(hwnd, HOTKEY_NORMAL_SPEED, MOD_CONTROL | MOD_ALT, VK_F12);

    MSG msg;
    int ret = 0;
    while ((ret = GetMessage(&msg, NULL, 0, 0)) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
    return ret < 0 ? 1 : (int)msg.wParam;
}