#ifndef C_CORDIC_ROTATE_ROM_HALF_PI_HPP
#define C_CORDIC_ROTATE_ROM_HALF_PI_HPP

#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdlib>

#include <complex>

#include <ap_fixed.h>
#include <ap_int.h>

#include "RomGeneratorConst/RomGeneratorConst.hpp"

// ``` GNU Octave
// kn_values(X) = prod(1 ./ abs(1 + 1j * 2.^ (-(0:X))))
// ```
static constexpr double kn_values[7] = {0.70710678118655, 0.632455532033680, 0.613571991077900, 0.608833912517750, 0.607648256256170, 0.607351770141300, 0.607277644093530};

template <unsigned TIn_W, unsigned TIn_I, unsigned TNStages, unsigned Tq>
class CCordicRotateRomHalfPi {
    static_assert(TIn_W > 0, "Inputs can't be on zero bits.");
    static_assert(TNStages < 8, "7 stages of CORDIC is the maximum supported.");
    static_assert(TNStages > 1, "2 stages of CORDIC is the minimum.");

public:
    static constexpr const CRomGeneratorConst<TIn_W, TNStages, Tq> & rom_cordic{};

    static constexpr unsigned In_W    = TIn_W;
    static constexpr unsigned In_I    = TIn_I;
    static constexpr unsigned Out_W   = In_W + 2;
    static constexpr unsigned Out_I   = In_I + 2;
    static constexpr unsigned NStages = TNStages;

    static constexpr uint64_t kn_i             = uint64_t(kn_values[NStages - 1] * double(1U << 3)); // 3 bits are enough
    static constexpr uint64_t in_scale_factor  = uint64_t(1U << (In_W - In_I));
    static constexpr uint64_t out_scale_factor = uint64_t(1U << (Out_W - Out_I));

    static constexpr int64_t scale_cordic(int64_t in) {
        return in * kn_i / 8U;
    }

    static constexpr std::complex<int64_t> cordic(std::complex<int64_t> x_in,
                                           uint8_t               counter) {

        int64_t A = x_in.real();
        int64_t B = x_in.imag();

        const uint8_t R    = rom_cordic.rom[counter];
        uint8_t       mask = 0x80;
        if ((R & mask) == mask) {
            A = -A;
            B = -B;
        }

        for (uint8_t u = 1; u < NStages + 1; u++) {
            mask = mask >> 1;

            const int64_t Ri = (R & mask) == mask ? 1 : -1;

            const int64_t I = A + Ri * (B / int64_t(1U << (u - 1)));
            B               = B - Ri * (A / int64_t(1U << (u - 1)));
            A               = I;
        }

        return {(A), (B)};
    }

#ifndef __SYNTHESIS__
    static constexpr double scale_cordic(double in) {
        return in * kn_values[NStages - 1];
    }

    static constexpr std::complex<double> cordic(std::complex<double> x_in,
                                          uint8_t              counter) {
        const std::complex<int64_t> fx_x_in(int64_t(x_in.real() * double(in_scale_factor)),
                                            int64_t(x_in.imag() * double(in_scale_factor)));

        const std::complex<int64_t> fx_out = cordic(fx_x_in, counter);
        return {scale_cordic(double(fx_out.real())) / double(out_scale_factor), scale_cordic(double(fx_out.imag())) / double(out_scale_factor)};
    }

#endif

    template <unsigned ap_W>
    static ap_int<ap_W> scale_cordic(const ap_int<ap_W> & in) {
        const ap_int<ap_W + 3> tmp = in * ap_uint<3>(kn_i);
        return ap_int<ap_W>(tmp >> 3);
    }

    static void cordic(const ap_int<In_W> & re_in, const ap_int<In_W> & im_in,
                const ap_uint<8> & counter,
                ap_int<Out_W> & re_out, ap_int<Out_W> & im_out) {

        const ap_uint<6 + 1> R = (rom_cordic.rom[counter] >> (7 - NStages));

        ap_int<Out_W> A = bool(R[NStages]) ? ap_int<In_W>(-re_in) : re_in;
        ap_int<Out_W> B = bool(R[NStages]) ? ap_int<In_W>(-im_in) : im_in;

        for (uint8_t u = 1; u < 6 + 1; u++) { // 6 stages

            const bool Ri = bool(R[NStages - u]);

            // Results in (X / 2^(u - 1)), meaning only the
            // Out_W - u LSBs are meaninfull in shifted_X
            // Can't use range access since 11111111 (-1) would become 00001111 (15).
            // Would be possible if the loop is manually unrolled, to predict bitsize,
            // thus directly put 1111 into 4 bits (so still -1).
            const ap_int<Out_W> shifted_A = A >> (u - 1); // A(Out_W - 1, u - 1);
            const ap_int<Out_W> shifted_B = B >> (u - 1); // B(Out_W - 1, u - 1);

            const ap_int<Out_W> arc_step_A
                = Ri
                    ? ap_int<Out_W>(-shifted_A)
                    : shifted_A;
            const ap_int<Out_W> arc_step_B
                = Ri
                    ? shifted_B
                    : ap_int<Out_W>(-shifted_B);

            const ap_int<Out_W + 1> I = A + arc_step_B;
            B                         = B + arc_step_A;
            A                         = I;
        }

        re_out = A;
        im_out = B;
    }
};

#if 0
template <>
inline void CCordicRotateRomHalfPi<16, 4, 6, 64>::cordic(
    const ap_int<16> & re_in, const ap_int<16> & im_in,
    const ap_uint<8> & counter,
    ap_int<Out_W> & re_out, ap_int<Out_W> & im_out) const {

    const ap_uint<6 + 1> R = (rom_cordic.rom[counter.to_uint()] >> (7 - 6));

    ap_int<Out_W> A = bool(R[6]) ? ap_int<16>(-re_in) : re_in;
    ap_int<Out_W> B = bool(R[6]) ? ap_int<16>(-im_in) : im_in;

    for (uint8_t u = 1; u < 6 + 1; u++) { // 6 stages

        const bool Ri = bool(R[6 - u]);

        // Results in (X / 2^(u - 1)), meaning only the
        // Out_W - u LSBs are meaninfull in shifted_X
        // Can't use range access since 11111111 (-1) would become 00001111 (15).
        // Would be possible if the loop is manually unrolled, to predict bitsize,
        // thus directly put 1111 into 4 bits (so still -1).
        const ap_int<Out_W> shifted_A = A >> (u - 1); // A(Out_W - 1, u - 1);
        const ap_int<Out_W> shifted_B = B >> (u - 1); // B(Out_W - 1, u - 1);

        const ap_int<Out_W> arc_step_A
            = Ri
                ? ap_int<Out_W>(-shifted_A)
                : shifted_A;
        const ap_int<Out_W> arc_step_B
            = Ri
                ? shifted_B
                : ap_int<Out_W>(-shifted_B);

        const auto I = A + arc_step_B;
        B            = B + arc_step_A;
        A            = I;
    }

    re_out = A;
    im_out = B;
}
#endif

#endif // C_CORDIC_ROTATE_ROM_HALF_PI_HPP