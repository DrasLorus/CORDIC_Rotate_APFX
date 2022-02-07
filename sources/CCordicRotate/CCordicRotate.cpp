#include "CCordicRotate.hpp"


#define uint2int(sz, in) ((in & (1U << sz)) == (1U << sz)   \
                              ? static_cast<short>(~in + 1) \
                              : static_cast<short>(in))

template <>
void CCordicRotate<8, 14, 4, 17, 5, 19, 7, 12>::process(
    const ap_fixed<14, 4> & fx_angle,
    const ap_fixed<17, 5> &  fx_re_in,
    const ap_fixed<17, 5> &  fx_im_in,
    ap_fixed<19, 7> &       fx_re_out,
    ap_fixed<19, 7> &       fx_im_out) {

    constexpr uint64_t sign_mask_14 = 0x2000;  // 0bxxx xx10 0000 0000 0000
    constexpr uint64_t sign_mask_17 = 0x10000; // 0bxx1 0000 0000 0000 0000
    constexpr uint64_t sign_mask_19 = 0x10000; // 0b100 0000 0000 0000 0000

    const uint16_t angle_bits = fx_angle.bits_to_uint64();

    const short angle = uint2int(14, angle_bits);

    const uint32_t re_in_bits = fx_re_in.bits_to_uint64();
    const uint32_t im_in_bits = fx_im_in.bits_to_uint64();

    const std::complex<int32_t> value(uint2int(17, re_in_bits), uint2int(17, im_in_bits));

    int   b_yn;
    int   xn;
    int   xtmp;
    int   ytmp;
    short z;
    bool  negate;
    if (angle > 1608) {
        if (((angle - 3217) & 8192) != 0) {
            z = static_cast<short>((angle - 3217) | -8192);
        } else {
            z = static_cast<short>((angle - 3217) & 8191);
        }
        if (z <= 1608) {
            negate = true;
        } else {
            if (((angle - 6434) & 8192) != 0) {
                z = static_cast<short>((angle - 6434) | -8192);
            } else {
                z = static_cast<short>((angle - 6434) & 8191);
            }
            negate = false;
        }
    } else if (angle < -1608) {
        if (((angle + 3217) & 8192) != 0) {
            z = static_cast<short>((angle + 3217) | -8192);
        } else {
            z = static_cast<short>((angle + 3217) & 8191);
        }
        if (z >= -1608) {
            negate = true;
        } else {
            if (((angle + 6434) & 8192) != 0) {
                z = static_cast<short>((angle + 6434) | -8192);
            } else {
                z = static_cast<short>((angle + 6434) & 8191);
            }
            negate = false;
        }
    } else {
        z      = angle;
        negate = false;
    }
    z = static_cast<short>(z << 2);
    if ((z & 8192) != 0) {
        z = static_cast<short>(z | -8192);
    } else {
        z = static_cast<short>(z & 8191);
    }
    xn   = value.real();
    b_yn = value.imag();
    xtmp = value.real();
    ytmp = value.imag();
    for (int idx = 0; idx < 8; idx++) {
        if (z < 0) {
            z = static_cast<short>(z + atanLUT.table[idx]);
            if ((z & 8192) != 0) {
                z = static_cast<short>(z | -8192);
            } else {
                z = static_cast<short>(z & 8191);
            }
            ytmp += xn;
            if ((ytmp & 262144) != 0) {
                xn = ytmp | -262144;
            } else {
                xn = ytmp & 262143;
            }
            ytmp = b_yn - xtmp;
            if ((ytmp & 262144) != 0) {
                b_yn = ytmp | -262144;
            } else {
                b_yn = ytmp & 262143;
            }
        } else {
            z = static_cast<short>(z - atanLUT.table[idx]);
            if ((z & 8192) != 0) {
                z = static_cast<short>(z | -8192);
            } else {
                z = static_cast<short>(z & 8191);
            }
            ytmp = xn - ytmp;
            if ((ytmp & 262144) != 0) {
                xn = ytmp | -262144;
            } else {
                xn = ytmp & 262143;
            }
            ytmp = b_yn + xtmp;
            if ((ytmp & 262144) != 0) {
                b_yn = ytmp | -262144;
            } else {
                b_yn = ytmp & 262143;
            }
        }
        ytmp = xn >> (idx + 1);
        if ((ytmp & 262144) != 0) {
            xtmp = ytmp | -262144;
        } else {
            xtmp = ytmp & 262143;
        }
        ytmp = b_yn >> (idx + 1);
        if ((ytmp & 262144) != 0) {
            ytmp |= -262144;
        } else {
            ytmp &= 262143;
        }
    }
    if (negate) {
        if ((-xn & 262144) != 0) {
            xn = -xn | -262144;
        } else {
            xn = -xn & 262143;
        }
        if ((-b_yn & 262144) != 0) {
            b_yn = -b_yn | -262144;
        } else {
            b_yn = -b_yn & 262143;
        }
    }

    const int64_t i_xn   = (xn * 39797L) >> 16;
    const int64_t i_b_yn = (b_yn * 39797L) >> 16;

    ap_fixed<19, 7> re, im;
    fx_re_out.V = *reinterpret_cast<const uint64_t *>(&i_xn);
    fx_im_out.V = *reinterpret_cast<const uint64_t *>(&i_b_yn);
}
