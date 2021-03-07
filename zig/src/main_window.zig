const std = @import("std");
usingnamespace std.os.windows;
usingnamespace std.os.windows.user32;

const expect = std.testing.expect;

usingnamespace @import("win32.zig");
usingnamespace @import("control.zig");

pub const MainWindow = struct {


    pub fn run() void {
        const hInstance = @intToPtr(HINSTANCE, std.process.getBaseAddress());
        const className = std.unicode.utf8ToUtf16LeStringLiteral("Class Name");
        const wc = WNDCLASSEXW{
            .style = 0,
            .lpfnWndProc = WindowProc,
            .cbClsExtra = 0,
            .cbWndExtra = 0,
            .hInstance = hInstance,
            .hIcon = null,
            .hCursor = null,
            .hbrBackground = null,
            .lpszMenuName = null,
            .lpszClassName = className,
            .hIconSm = null,
        };
        expect(RegisterClassExW(&wc) != 0);

        const hWnd = CreateWindowExW(
            0,
            className,
            std.unicode.utf8ToUtf16LeStringLiteral("Window Title"),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            null,
            null,
            hInstance,
            null, // TODO: Add application pointer
        ).?;

        _ = ShowWindow(hWnd, SW_SHOW);

        var msg: MSG = undefined;
        while (GetMessageW(&msg, null, 0, 0) != 0) {
            _ = TranslateMessage(&msg);
            _ = DispatchMessageW(&msg);
        }
    }
};

fn WindowProc(hwnd: HWND, message: UINT, wParam: WPARAM, lParam: LPARAM) callconv(.Stdcall) LRESULT {
    var pMainWindow = @intToPtr(*MainWindow, GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (message) {
        WM_CREATE => std.log.info("All your codebase are belong to us.", .{}),
        WM_DESTROY => {
            PostQuitMessage(0);
            return null;
        },
        else => {},
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}
