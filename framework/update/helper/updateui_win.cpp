/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "updateui_win.h"

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>

#include <cwchar>
#include <string>

#include "../internal/platform/win/winupdateshared.h"

//! Sent to a window whose monitor, or the scaling of it, has changed. Declared
//! here rather than relied on: it only appeared in the Windows 8.1 SDK.
#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

namespace shared = muse::update::win;

namespace {
const wchar_t* WINDOW_CLASS_NAME = L"MuseUpdateProgressWindow";

const UINT WM_UI_COMMAND = WM_APP + 1;

const UINT_PTR MARQUEE_TIMER_ID = 1;
const UINT MARQUEE_INTERVAL_MS = 30;

//! Sizes in logical pixels at 96 DPI; scaled to the DPI of the window.
const int WINDOW_WIDTH = 460;
const int WINDOW_HEIGHT = 108;
const int MARGIN = 28;
const int MESSAGE_TOP = 30;
const int MESSAGE_HEIGHT = 24;
const int BAR_TOP = 70;
const int BAR_HEIGHT = 6;
const int MESSAGE_POINT_SIZE = 10;

enum class Command {
    Title,
    Message,
    Background,
    Accent,
    Foreground,
    Show,
    Progress,
    Close
};

struct Window {
    HWND handle = nullptr;
    UINT dpi = 96;
    HFONT font = nullptr;

    std::wstring message;

    COLORREF background = RGB(0x2E, 0x2E, 0x2E);
    COLORREF accent = RGB(0x0F, 0x9A, 0xF7);
    COLORREF foreground = RGB(0xFF, 0xFF, 0xFF);

    int percent = -1;
    int marqueeOffset = 0;
};

int scaled(const Window& window, int value)
{
    return ::MulDiv(value, static_cast<int>(window.dpi), 96);
}

COLORREF blend(COLORREF from, COLORREF to, int percentOfTo)
{
    auto mix = [percentOfTo](int a, int b) {
        return (a * (100 - percentOfTo) + b * percentOfTo) / 100;
    };

    return RGB(mix(GetRValue(from), GetRValue(to)),
               mix(GetGValue(from), GetGValue(to)),
               mix(GetBValue(from), GetBValue(to)));
}

bool parseColor(const std::string& text, COLORREF& color)
{
    if (text.size() != 7 || text[0] != '#') {
        return false;
    }

    int components[3] = { 0, 0, 0 };

    for (int i = 0; i < 3; ++i) {
        int value = 0;
        for (int j = 0; j < 2; ++j) {
            const char c = text[1 + i * 2 + j];
            int digit = 0;
            if (c >= '0' && c <= '9') {
                digit = c - '0';
            } else if (c >= 'a' && c <= 'f') {
                digit = c - 'a' + 10;
            } else if (c >= 'A' && c <= 'F') {
                digit = c - 'A' + 10;
            } else {
                return false;
            }
            value = value * 16 + digit;
        }
        components[i] = value;
    }

    color = RGB(components[0], components[1], components[2]);
    return true;
}

HFONT createFont(UINT dpi)
{
    LOGFONTW logFont = { };
    logFont.lfHeight = -::MulDiv(MESSAGE_POINT_SIZE, static_cast<int>(dpi), 72);
    logFont.lfWeight = FW_NORMAL;
    logFont.lfCharSet = DEFAULT_CHARSET;
    logFont.lfQuality = CLEARTYPE_QUALITY;
    ::wcscpy_s(logFont.lfFaceName, L"Segoe UI");

    return ::CreateFontIndirectW(&logFont);
}

UINT windowDpi(HWND hwnd)
{
    using GetDpiForWindowFn = UINT(WINAPI*)(HWND);

    if (const HMODULE user32 = ::GetModuleHandleW(L"user32.dll")) {
        const auto getDpiForWindow = reinterpret_cast<GetDpiForWindowFn>(
            reinterpret_cast<void*>(::GetProcAddress(user32, "GetDpiForWindow")));
        if (getDpiForWindow) {
            const UINT dpi = getDpiForWindow(hwnd);
            if (dpi != 0) {
                return dpi;
            }
        }
    }

    const HDC screen = ::GetDC(nullptr);
    const UINT dpi = screen ? static_cast<UINT>(::GetDeviceCaps(screen, LOGPIXELSX)) : 96;
    if (screen) {
        ::ReleaseDC(nullptr, screen);
    }

    return dpi != 0 ? dpi : 96;
}

//! Per-monitor aware, so that the window is drawn at the resolution of the
//! screen it is on rather than stretched by the system. The helper has no
//! manifest of its own to declare this in.
void makeProcessDpiAware()
{
    //! NOTE: The context is passed as a plain handle and the awareness level as
    //! its documented value, so that the helper builds against SDKs older than
    //! the one that introduced `DPI_AWARENESS_CONTEXT`.
    using SetContextFn = BOOL(WINAPI*)(HANDLE);
    const HANDLE perMonitorAwareV2 = reinterpret_cast<HANDLE>(static_cast<intptr_t>(-4));

    const HMODULE user32 = ::GetModuleHandleW(L"user32.dll");
    if (!user32) {
        return;
    }

    const auto setContext = reinterpret_cast<SetContextFn>(
        reinterpret_cast<void*>(::GetProcAddress(user32, "SetProcessDpiAwarenessContext")));
    if (setContext && setContext(perMonitorAwareV2)) {
        return;
    }

    ::SetProcessDPIAware();
}

void centerOnPrimaryScreen(Window& window)
{
    const int width = scaled(window, WINDOW_WIDTH);
    const int height = scaled(window, WINDOW_HEIGHT);

    RECT workArea = { };
    if (!::SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0)) {
        workArea.left = 0;
        workArea.top = 0;
        workArea.right = ::GetSystemMetrics(SM_CXSCREEN);
        workArea.bottom = ::GetSystemMetrics(SM_CYSCREEN);
    }

    const int x = workArea.left + ((workArea.right - workArea.left) - width) / 2;
    const int y = workArea.top + ((workArea.bottom - workArea.top) - height) / 2;

    ::SetWindowPos(window.handle, HWND_TOPMOST, x, y, width, height, SWP_NOACTIVATE);
}

void fillRoundedRect(HDC dc, const RECT& rect, COLORREF color)
{
    const int radius = rect.bottom - rect.top;
    if (radius <= 0 || rect.right <= rect.left) {
        return;
    }

    const HBRUSH brush = ::CreateSolidBrush(color);
    const HPEN pen = ::CreatePen(PS_SOLID, 1, color);

    const HGDIOBJ oldBrush = ::SelectObject(dc, brush);
    const HGDIOBJ oldPen = ::SelectObject(dc, pen);

    ::RoundRect(dc, rect.left, rect.top, rect.right + 1, rect.bottom + 1, radius, radius);

    ::SelectObject(dc, oldBrush);
    ::SelectObject(dc, oldPen);
    ::DeleteObject(brush);
    ::DeleteObject(pen);
}

void paintProgressBar(HDC dc, Window& window, const RECT& bar)
{
    const int trackWidth = bar.right - bar.left;
    if (trackWidth <= 0) {
        return;
    }

    fillRoundedRect(dc, bar, blend(window.background, window.foreground, 20));

    RECT fill = bar;

    if (window.percent >= 0) {
        const int percent = window.percent > 100 ? 100 : window.percent;
        int width = trackWidth * percent / 100;

        // Below the height of the bar the rounded ends would draw as a dot of
        // the wrong shape; nothing is a more honest picture of "just started"
        // than nothing at all.
        const int minimum = bar.bottom - bar.top;
        if (width > 0 && width < minimum) {
            width = minimum;
        }

        if (width <= 0) {
            return;
        }

        fill.right = bar.left + width;
    } else {
        const int chunkWidth = trackWidth * 30 / 100;
        const int period = trackWidth + chunkWidth;
        const int offset = period > 0 ? window.marqueeOffset % period : 0;

        fill.left = bar.left - chunkWidth + offset;
        fill.right = fill.left + chunkWidth;

        // The chunk slides in and out of the track rather than being clipped
        // into a shrinking block at either end.
        ::IntersectClipRect(dc, bar.left, bar.top, bar.right + 1, bar.bottom + 1);
    }

    fillRoundedRect(dc, fill, window.accent);

    if (window.percent < 0) {
        ::SelectClipRgn(dc, nullptr);
    }
}

void paint(Window& window)
{
    PAINTSTRUCT paintStruct = { };
    const HDC dc = ::BeginPaint(window.handle, &paintStruct);
    if (!dc) {
        return;
    }

    RECT client = { };
    ::GetClientRect(window.handle, &client);

    // Everything is drawn into a bitmap first: the marquee repaints tens of
    // times a second and would otherwise flicker.
    const HDC memoryDc = ::CreateCompatibleDC(dc);
    const HBITMAP bitmap = ::CreateCompatibleBitmap(dc, client.right, client.bottom);
    const HGDIOBJ oldBitmap = ::SelectObject(memoryDc, bitmap);

    const HBRUSH backgroundBrush = ::CreateSolidBrush(window.background);
    ::FillRect(memoryDc, &client, backgroundBrush);
    ::DeleteObject(backgroundBrush);

    // A hairline border, so that the window is still a window against a
    // background of the same colour.
    const HBRUSH borderBrush = ::CreateSolidBrush(blend(window.background, window.foreground, 18));
    ::FrameRect(memoryDc, &client, borderBrush);
    ::DeleteObject(borderBrush);

    if (!window.message.empty()) {
        RECT text = {
            scaled(window, MARGIN),
            scaled(window, MESSAGE_TOP),
            client.right - scaled(window, MARGIN),
            scaled(window, MESSAGE_TOP) + scaled(window, MESSAGE_HEIGHT)
        };

        const HGDIOBJ oldFont = ::SelectObject(memoryDc, window.font);
        ::SetBkMode(memoryDc, TRANSPARENT);
        ::SetTextColor(memoryDc, window.foreground);
        ::DrawTextW(memoryDc, window.message.c_str(), static_cast<int>(window.message.size()), &text,
                    DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
        ::SelectObject(memoryDc, oldFont);
    }

    const RECT bar = {
        scaled(window, MARGIN),
        scaled(window, BAR_TOP),
        client.right - scaled(window, MARGIN),
        scaled(window, BAR_TOP) + scaled(window, BAR_HEIGHT)
    };
    paintProgressBar(memoryDc, window, bar);

    ::BitBlt(dc, 0, 0, client.right, client.bottom, memoryDc, 0, 0, SRCCOPY);

    ::SelectObject(memoryDc, oldBitmap);
    ::DeleteObject(bitmap);
    ::DeleteDC(memoryDc);

    ::EndPaint(window.handle, &paintStruct);
}

void setPercent(Window& window, int percent)
{
    if (window.percent == percent) {
        return;
    }

    const bool wasIndeterminate = window.percent < 0;
    window.percent = percent;

    if (percent < 0 && !wasIndeterminate) {
        ::SetTimer(window.handle, MARQUEE_TIMER_ID, MARQUEE_INTERVAL_MS, nullptr);
    } else if (percent >= 0 && wasIndeterminate) {
        ::KillTimer(window.handle, MARQUEE_TIMER_ID);
    }

    ::InvalidateRect(window.handle, nullptr, FALSE);
}

void handleCommand(Window& window, Command command, LPARAM payload)
{
    switch (command) {
    case Command::Title: {
        std::wstring* title = reinterpret_cast<std::wstring*>(payload);
        ::SetWindowTextW(window.handle, title->c_str());
        delete title;
    } break;

    case Command::Message: {
        std::wstring* message = reinterpret_cast<std::wstring*>(payload);
        window.message = *message;
        delete message;
        ::InvalidateRect(window.handle, nullptr, FALSE);
    } break;

    case Command::Background:
        window.background = static_cast<COLORREF>(payload);
        ::InvalidateRect(window.handle, nullptr, FALSE);
        break;

    case Command::Accent:
        window.accent = static_cast<COLORREF>(payload);
        ::InvalidateRect(window.handle, nullptr, FALSE);
        break;

    case Command::Foreground:
        window.foreground = static_cast<COLORREF>(payload);
        ::InvalidateRect(window.handle, nullptr, FALSE);
        break;

    case Command::Show:
        //! NOTE: Shown without activation: the application is quitting at this
        //! point and stealing focus from whatever the user turned to instead
        //! would be worse than being one window down in the z-order.
        ::ShowWindow(window.handle, SW_SHOWNOACTIVATE);
        break;

    case Command::Progress:
        setPercent(window, static_cast<int>(payload));
        break;

    case Command::Close:
        ::DestroyWindow(window.handle);
        break;
    }
}

LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    Window* window = reinterpret_cast<Window*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (message) {
    case WM_NCCREATE: {
        const CREATESTRUCTW* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    } break;

    case WM_UI_COMMAND:
        if (window) {
            handleCommand(*window, static_cast<Command>(wParam), lParam);
        }
        return 0;

    case WM_TIMER:
        if (window && wParam == MARQUEE_TIMER_ID) {
            // Wrapped well short of overflowing; the position is taken modulo
            // the width of the track anyway.
            window->marqueeOffset = (window->marqueeOffset + scaled(*window, 12)) % (1 << 20);
            ::InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_ERASEBKGND:
        // Painted in full in WM_PAINT.
        return 1;

    case WM_PAINT:
        if (window) {
            paint(*window);
            return 0;
        }
        break;

    case WM_NCHITTEST:
        // The window has no title bar, but should still be possible to drag
        // out of the way of whatever it is covering.
        return HTCAPTION;

    case WM_DPICHANGED:
        if (window) {
            window->dpi = HIWORD(wParam);

            if (window->font) {
                ::DeleteObject(window->font);
            }
            window->font = createFont(window->dpi);

            const RECT* suggested = reinterpret_cast<RECT*>(lParam);
            ::SetWindowPos(hwnd, nullptr, suggested->left, suggested->top,
                           suggested->right - suggested->left, suggested->bottom - suggested->top,
                           SWP_NOZORDER | SWP_NOACTIVATE);
            ::InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }

    return ::DefWindowProcW(hwnd, message, wParam, lParam);
}

void postCommand(HWND hwnd, Command command, LPARAM payload)
{
    ::PostMessageW(hwnd, WM_UI_COMMAND, static_cast<WPARAM>(command), payload);
}

void postText(HWND hwnd, Command command, const std::string& value)
{
    postCommand(hwnd, command, reinterpret_cast<LPARAM>(new std::wstring(shared::utf8ToWide(value))));
}

void postColor(HWND hwnd, Command command, const std::string& value)
{
    COLORREF color = 0;
    if (parseColor(value, color)) {
        postCommand(hwnd, command, static_cast<LPARAM>(color));
    }
}

void handleLine(HWND hwnd, const std::string& line)
{
    const size_t space = line.find(' ');
    const std::string name = line.substr(0, space);
    const std::string value = space != std::string::npos ? line.substr(space + 1) : std::string();

    if (name == updateui::command::TITLE) {
        postText(hwnd, Command::Title, value);
    } else if (name == updateui::command::MESSAGE) {
        postText(hwnd, Command::Message, value);
    } else if (name == updateui::command::BACKGROUND) {
        postColor(hwnd, Command::Background, value);
    } else if (name == updateui::command::ACCENT) {
        postColor(hwnd, Command::Accent, value);
    } else if (name == updateui::command::FOREGROUND) {
        postColor(hwnd, Command::Foreground, value);
    } else if (name == updateui::command::SHOW) {
        postCommand(hwnd, Command::Show, 0);
    } else if (name == updateui::command::PROGRESS) {
        int percent = -1;
        try {
            percent = std::stoi(value);
        } catch (...) {
            percent = -1;
        }
        postCommand(hwnd, Command::Progress, static_cast<LPARAM>(percent));
    } else if (name == updateui::command::CLOSE) {
        postCommand(hwnd, Command::Close, 0);
    }
}

struct ReaderContext {
    HWND hwnd = nullptr;
    HANDLE pipe = INVALID_HANDLE_VALUE;
};

DWORD WINAPI readerThread(LPVOID parameter)
{
    ReaderContext* context = static_cast<ReaderContext*>(parameter);

    std::string buffer;
    char chunk[512] = { 0 };

    for (;;) {
        DWORD read = 0;
        if (!::ReadFile(context->pipe, chunk, sizeof(chunk), &read, nullptr) || read == 0) {
            break;
        }

        buffer.append(chunk, read);

        for (size_t end = buffer.find('\n'); end != std::string::npos; end = buffer.find('\n')) {
            std::string line = buffer.substr(0, end);
            buffer.erase(0, end + 1);

            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            if (!line.empty()) {
                handleLine(context->hwnd, line);
            }
        }
    }

    //! NOTE: The end of the pipe means the privileged side is gone, whether it
    //! finished or died; either way there is nothing left to report.
    postCommand(context->hwnd, Command::Close, 0);

    delete context;
    return 0;
}
}

namespace updateui {
int run(const std::wstring& pipeHandleValue)
{
    HANDLE pipe = INVALID_HANDLE_VALUE;
    try {
        pipe = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(std::stoull(pipeHandleValue)));
    } catch (...) {
        return 1;
    }

    makeProcessDpiAware();

    const HINSTANCE instance = ::GetModuleHandleW(nullptr);

    WNDCLASSEXW windowClass = { };
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = windowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    windowClass.lpszClassName = WINDOW_CLASS_NAME;

    if (::RegisterClassExW(&windowClass) == 0) {
        return 1;
    }

    Window window;

    window.handle = ::CreateWindowExW(WS_EX_TOPMOST | WS_EX_APPWINDOW, WINDOW_CLASS_NAME, L"",
                                      WS_POPUP, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
                                      nullptr, nullptr, instance, &window);
    if (!window.handle) {
        return 1;
    }

    window.dpi = windowDpi(window.handle);
    window.font = createFont(window.dpi);
    centerOnPrimaryScreen(window);

    ::SetTimer(window.handle, MARQUEE_TIMER_ID, MARQUEE_INTERVAL_MS, nullptr);

    ReaderContext* context = new ReaderContext();
    context->hwnd = window.handle;
    context->pipe = pipe;

    const HANDLE thread = ::CreateThread(nullptr, 0, readerThread, context, 0, nullptr);
    if (!thread) {
        delete context;
        ::DestroyWindow(window.handle);
        return 1;
    }

    MSG message = { };
    while (::GetMessageW(&message, nullptr, 0, 0) > 0) {
        ::TranslateMessage(&message);
        ::DispatchMessageW(&message);
    }

    //! NOTE: Normally the reader has already finished - it is what asked the
    //! window to close. It is still sitting in ReadFile if the window was
    //! closed by hand, and cancelling that is what lets it end.
    ::CancelSynchronousIo(thread);
    ::CloseHandle(pipe);
    ::WaitForSingleObject(thread, 2000);
    ::CloseHandle(thread);

    if (window.font) {
        ::DeleteObject(window.font);
    }

    return 0;
}
}
