usingnamespace @import("win32.zig");
const d2d1 = @import("d2d1.zig");

pub fn Control() type {
    return struct {
        const Self = @This();

        /// <summary>
        /// X position relative to parent control.
        /// Note: Use zero if you want to specify the left edge inside the Paint() method.
        /// </summary>
        xParent: i16,

        /// <summary>
        /// Y position relative to parent control.
        /// Note: Use zero if you want to specify the top edge inside the Paint() method.
        /// </summary>
        yParent: i16,
        width: i16,
        height: i16,

        // Pure virtual
        initFn: fn (self: *Self, pAllocator: *AllocatorBase) void,
        resetFn: fn (self: *Self, pRenderTarget: *ID2D1RenderTarget) void,
        destroyFn: fn (self: *Self) void,

        // Virtual
        onMouseMoveFn: fn (self: *Self, x: i16, y: i16) void,
        onDoubleClickFn: fn (self: *Self, x: i16, y: i16, mkMask: u16) void,
        onMouseWheelFn: fn (self: *Self, x: i16, y: i16, mkMask: u16, zDelta: i16) void,
        onContextMenuFn: fn (self: *Self, clientX: i16, clientY: i16, screenX: i16, screenY: i16) void,
        onSizeFn: fn (self: *Self, clientX: i16, clientY: i16, screenX: i16, screenY: i16) void,
        onLeftButtonDownFn: fn (self: *Self, x: i16, y: i16) bool,
        onLeftButtonUpFn: fn (self: *Self, x: i16, y: i16) bool,

        pub fn OnPaint(pRenderTarget: *ID2D1RenderTarget) void {
            // ZoneScoped;
            var transform: D2D1_MATRIX_3X2_F = undefined;
            ID2D1RenderTarget_GetTransform(pRenderTarget, &transform);
            ID2D1RenderTarget_SetTransform(d2d1.mul(d2d1.translation(self.xParent, self.yParent), transform));
            defer ID2D1RenderTarget_SetTransform(pRenderTarget, &transform);

            ID2D1RenderTarget_PushAxisAlignedClip(pRenderTarget, D2D1_RECT_F{
                .left = 0.0,
                .top = 0.0,
                .right = self.width,
                .bottom = self.height,
            }, D2D1_ANTIALIAS_MODE_ALIASED);
            defer ID2D1RenderTarget_PopAxisAlignedClip();

            const antialiasMode = ID2D1RenderTarget_GetAntialiasMode();
            ID2D1RenderTarget_SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
            defer ID2D1RenderTarget_SetAntialiasMode(antialiasMode);

            self.Paint(pRenderTarget);
        }

        pub fn init(self: *Self, pAllocator: *AllocatorBase) void {
            return self.nextFn(self, pAllocator);
        }

        pub fn reset(self: *Self, pRenderTarget: *ID2D1RenderTarget) void {
            return self.nextFn(self, pRenderTarget);
        }

        pub fn destroy(self: *Self) void {
            return self.nextFn(self);
        }
    };
}
