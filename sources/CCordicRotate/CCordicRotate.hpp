#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdlib>

#include <complex>

#include <ap_fixed.h>
#include <ap_int.h>

static constexpr double atanDbl[28] {
    0.78539816339745, 0.46364760900081, 0.24497866312686, 0.12435499454676,
    0.06241880999596, 0.03123983343027, 0.01562372862048, 0.00781234106010,
    0.00390623013197, 0.00195312251648, 0.00097656218956, 0.00048828121119,
    0.00024414062015, 0.00012207031189, 0.00006103515617, 0.00003051757812,
    0.00001525878906, 0.00000762939453, 0.00000381469727, 0.00000190734863,
    0.00000095367432, 0.00000047683716, 0.00000023841858, 0.00000011920929,
    0.00000005960464, 0.00000002980232, 0.00000001490116, 0.00000000745058};

template <uint8_t N_STAGES,
          class T,
          uint8_t ATAN_I>
struct CAtanLUT {
    static_assert(N_STAGES < 28, "Not enough arctan available.");
    static_assert(N_STAGES <= ATAN_I, "ATAN_I can't be less than N_STAGES.");
    static_assert(std::__is_standard_integer<T>(), "Must be a standard C++ integer type.");
    constexpr CAtanLUT() : table() {
        for (uint8_t i = 0; i < N_STAGES; ++i) {
            const double scaled = atanDbl[i] * static_cast<double>(1 << ATAN_I) + 0.5;
            table[i]            = static_cast<T>(scaled);
        }
    }
    T table[N_STAGES];
};

template <uint8_t N_STAGES,
          uint8_t TH_W,
          uint8_t TH_I,
          uint8_t IN_W,
          uint8_t IN_I,
          uint8_t OUT_W,
          uint8_t OUT_I,
          uint8_t ATAN_I>
class CCordicRotate {
private:
    static constexpr auto atanLUT = CAtanLUT<N_STAGES, uint64_t, ATAN_I>();

public:
    static void process(
        const ap_fixed<TH_W, TH_I> & fx_angle,
        const ap_fixed<IN_W, IN_I> & fx_re_in,
        const ap_fixed<IN_W, IN_I> & fx_im_in,
        ap_fixed<OUT_W, OUT_I> &     fx_re_out,
        ap_fixed<OUT_W, OUT_I> &     fx_im_out);

    CCordicRotate() {}
    virtual ~CCordicRotate() {};
};

#define uint2int(sz, in) ((in & (1U << sz)) == (1U << sz)   \
                              ? static_cast<short>(~in + 1) \
                              : static_cast<short>(in))

template <>
void CCordicRotate<8, 14, 10, 17, 5, 19, 12, 12>::process(
    const ap_fixed<14, 10> & fx_angle,
    const ap_fixed<17, 5> &  fx_re_in,
    const ap_fixed<17, 5> &  fx_im_in,
    ap_fixed<19, 12> &       fx_re_out,
    ap_fixed<19, 12> &       fx_im_out) {

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

    ap_fixed<19, 7> re, im;
    re.V = static_cast<uint32_t>((xn * 39797L) >> 16);
    im.V = static_cast<uint32_t>((b_yn * 39797L) >> 16);

    ap_fx_cpx<19, 7> iterative_factor;
    iterative_factor.real(re);
    iterative_factor.imag(im);

    return iterative_factor;
}