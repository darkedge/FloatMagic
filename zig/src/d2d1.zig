usingnamespace @import("win32.zig");

pub fn translation(x: f32, y: f32) D2D1_MATRIX_3X2_F {
    return D2D1_MATRIX_3X2_F{
        ._11 = 1.0,
        ._12 = 0.0,
        ._21 = 0.0,
        ._22 = 1.0,
        ._31 = x,
        ._32 = y,
    };
}

pub fn mul(a: D2D1_MATRIX_3X2_F, b: D2D1_MATRIX_3X2_F) D2D1_MATRIX_3X2_F {
    return D2D1_MATRIX_3X2_F{
        ._11 = a._11 * b._11 + a._12 * b._21,
        ._12 = a._11 * b._12 + a._12 * b._22,
        ._21 = a._21 * b._11 + a._22 * b._21,
        ._22 = a._21 * b._12 + a._22 * b._22,
        ._31 = a._31 * b._11 + a._32 * b._21 + b._31,
        ._32 = a._31 * b._12 + a._32 * b._22 + b._32,
    };
}