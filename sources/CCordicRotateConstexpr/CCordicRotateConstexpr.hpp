/*
 *
 * Copyright 2022 Camille "DrasLorus" Monière.
 *
 * This file is part of CORDIC_Rotate_APFX.
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU
 * Lesser General Public License as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef C_CORDIC_ROTATE_CONSTEXPR_HPP
#define C_CORDIC_ROTATE_CONSTEXPR_HPP

#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdlib>

#include <complex>

#include <ap_fixed.h>
#include <ap_int.h>

#include "RomGeneratorConst/RomGeneratorConst.hpp"

namespace rcr = rom_cordic_rotate;

template <unsigned TIn_W, unsigned TIn_I, unsigned Tnb_stages, unsigned Tq, unsigned divider = 2>
class CCordicRotateConstexpr {
    static_assert(TIn_W > 0, "Inputs can't be on zero bits.");
    static_assert(Tnb_stages < 8, "7 stages of CORDIC is the maximum supported.");
    static_assert(Tnb_stages > 1, "2 stages of CORDIC is the minimum.");
    static_assert(rcr::is_pow_2<divider>(), "divider must be a power of 2.");

public:
    // ``` GNU Octave
    // kn_values(X) = prod(1 ./ abs(1 + 1j * 2.^ (-(0:X))))
    // ```
    static constexpr double kn_values[7] = {
        0.70710678118655, 0.632455532033680, 0.613571991077900,
        0.608833912517750, 0.607648256256170, 0.607351770141300, 0.607277644093530};
    static constexpr const CRomGeneratorConst<TIn_W, Tnb_stages, Tq, divider> & rom_cordic {};

    static constexpr unsigned In_W      = TIn_W;
    static constexpr unsigned In_I      = TIn_I;
    static constexpr unsigned Out_W     = In_W + 2;
    static constexpr unsigned Out_I     = In_I + 2;
    static constexpr unsigned nb_stages = Tnb_stages;

    static constexpr unsigned kn_i             = unsigned(kn_values[nb_stages - 1] * double(1U << 4)); // 4 bits are enough
    static constexpr unsigned in_scale_factor  = unsigned(1U << (In_W - In_I));
    static constexpr unsigned out_scale_factor = unsigned(1U << (Out_W - Out_I));

    static constexpr double   rotation    = CRomGeneratorConst<TIn_W, Tnb_stages, Tq, divider>::rotation;
    static constexpr unsigned addr_length = CRomGeneratorConst<TIn_W, Tnb_stages, Tq, divider>::addr_length;

    static constexpr int64_t scale_cordic(int64_t in) {
        return in * kn_i / 16U;
    }

    static constexpr double scale_cordic(double in) {
        return in * kn_values[nb_stages - 1];
    }

#if !defined(__SYNTHESIS__) && defined(SOFTWARE)
    static constexpr std::complex<int64_t> cordic(std::complex<int64_t> x_in,
                                                  uint64_t              counter) {

        int64_t A = x_in.real();
        int64_t B = x_in.imag();

        const uint8_t R    = rom_cordic.rom[counter];
        uint8_t       mask = 0x01;
        if ((R & mask) == mask) {
            A = -A;
            B = -B;
        }

        for (uint8_t u = 1; u < nb_stages + 1; u++) {
            mask = mask << 1;

            const int64_t Ri = (R & mask) == mask ? 1 : -1;

            const int64_t I = A + Ri * (B / int64_t(1U << (u - 1)));
            B               = B - Ri * (A / int64_t(1U << (u - 1)));
            A               = I;
        }

        return {(A), (B)};
    }

    static constexpr std::complex<double> cordic(std::complex<double> x_in,
                                                 uint64_t             counter) {
        const std::complex<int64_t> fx_x_in(int64_t(x_in.real() * double(in_scale_factor)),
                                            int64_t(x_in.imag() * double(in_scale_factor)));

        const std::complex<int64_t> fx_out = cordic(fx_x_in, counter);
        return {scale_cordic(double(fx_out.real())) / double(out_scale_factor), scale_cordic(double(fx_out.imag())) / double(out_scale_factor)};
    }

#endif

    static ap_int<Out_W> scale_cordic(const ap_int<Out_W> & in) {
        const ap_int<Out_W + 4> tmp = in * ap_uint<4>(kn_i);
        return ap_int<Out_W>(tmp >> 4);
    }

    static void cordic(const ap_int<In_W> & re_in, const ap_int<In_W> & im_in,
                       const ap_uint<addr_length> & counter,
                       ap_int<Out_W> & re_out, ap_int<Out_W> & im_out) {

        const ap_uint<nb_stages + 1> R = rom_cordic.rom[counter];

        ap_int<Out_W> A = bool(R[0]) ? ap_int<In_W>(-re_in) : re_in;
        ap_int<Out_W> B = bool(R[0]) ? ap_int<In_W>(-im_in) : im_in;

        for (uint8_t u = 1; u < nb_stages + 1; u++) { // nb_stages stages

            const bool Ri = bool(R[u]);

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

    constexpr CCordicRotateConstexpr() = default;
};

#if 0
template <>
inline void CCordicRotateConstexpr<16, 4, 6, 64>::cordic(
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

#endif // C_CORDIC_ROTATE_CONSTEXPR_HPP