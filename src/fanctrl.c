/*
 * lenovo-fanctl — lightweight fork of Lenovo-Fan-Control
 * Upstream: https://github.com/jiarandiana0307/Lenovo-Fan-Control
 * Copyright (c) 2024 Kira Diana (original)
 * Modifications Copyright (c) 2025 Platerman
 */

#include <stdio.h>
#include <Windows.h>

#include "fanctrl.h"

volatile int is_keep_fan_running = 0;

static int NORMAL_MODE_EXPECTED_VALUE = -1;
static int normal_initialized = 0;

int fan_control(enum FanMode mode) {
    HANDLE hndl = CreateFileW(L"\\\\.\\EnergyDrv", GENERIC_WRITE, 0, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hndl == INVALID_HANDLE_VALUE) {
        return -1;
    }
    DWORD inBuffer[3] = { 6, 1, (DWORD)mode };
    DWORD bytesReturned = 0;
    BOOL ok = DeviceIoControl(hndl, 0x831020C0, inBuffer, sizeof(inBuffer),
                              NULL, 0, &bytesReturned, NULL);
    CloseHandle(hndl);
    return ok ? 1 : -1;
}

int read_state(void) {
    if (!normal_initialized) {
        if (fan_control(NORMAL) != 1) {
            return -1;
        }
        Sleep(50);
    }
    HANDLE hndl = CreateFileW(L"\\\\.\\EnergyDrv", GENERIC_READ, 0, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hndl == INVALID_HANDLE_VALUE) {
        return -1;
    }
    DWORD inBuffer[1] = { 14 };
    DWORD outBuffer[1] = { 0 };
    DWORD bytesReturned = 0;
    BOOL ok = DeviceIoControl(hndl, 0x831020C4, inBuffer, sizeof(inBuffer),
                              outBuffer, sizeof(outBuffer), &bytesReturned, NULL);
    CloseHandle(hndl);
    if (!ok || bytesReturned < sizeof(outBuffer)) {
        return -1;
    }
    if (!normal_initialized) {
        NORMAL_MODE_EXPECTED_VALUE = (int)outBuffer[0];
        normal_initialized = 1;
    }
    return outBuffer[0] == (DWORD)NORMAL_MODE_EXPECTED_VALUE ? NORMAL : FAST;
}

void keep_fan_running(void) {
    is_keep_fan_running = 1;
    const DWORD interval = 8980;
    while (is_keep_fan_running) {
        int s = read_state();
        if (s == -1) {
            break;
        }
        if (s != FAST) {
            fan_control(FAST);
            Sleep(10);
            continue;
        }
        DWORD start = GetTickCount();
        while (is_keep_fan_running && (GetTickCount() - start) < interval) {
            Sleep(100);
        }
        if (!is_keep_fan_running) {
            break;
        }
        while (is_keep_fan_running) {
            s = read_state();
            if (s == -1 || s == NORMAL) {
                break;
            }
            fan_control(NORMAL);
            Sleep(10);
        }
        if (s == -1) {
            break;
        }
    }
    fan_control(NORMAL);
}