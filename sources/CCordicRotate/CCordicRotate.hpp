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

#ifndef C_CORDIC_ROTATE_HPP
#define C_CORDIC_ROTATE_HPP

#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdlib>

#include <complex>

#include <ap_fixed.h>
#include <ap_int.h>

template <uint8_t N_STAGES,
          class T,
          uint8_t ATAN_I>
struct CAtanLUT {
    static constexpr double atanDbl[28] {
        0.78539816339745, 0.46364760900081, 0.24497866312686, 0.12435499454676,
        0.06241880999596, 0.03123983343027, 0.01562372862048, 0.00781234106010,
        0.00390623013197, 0.00195312251648, 0.00097656218956, 0.00048828121119,
        0.00024414062015, 0.00012207031189, 0.00006103515617, 0.00003051757812,
        0.00001525878906, 0.00000762939453, 0.00000381469727, 0.00000190734863,
        0.00000095367432, 0.00000047683716, 0.00000023841858, 0.00000011920929,
        0.00000005960464, 0.00000002980232, 0.00000001490116, 0.00000000745058};

    static_assert(N_STAGES < 28, "Not enough arctan available.");
    static_assert(N_STAGES <= ATAN_I, "ATAN_I can't be less than N_STAGES.");
    static_assert(std::is_integral<T>(), "Must be a standard C++ integer type.");
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
public:
    static constexpr const CAtanLUT<N_STAGES, uint64_t, ATAN_I> & atanLUT = CAtanLUT<N_STAGES, uint64_t, ATAN_I>();

    static void process(
        const ap_fixed<TH_W, TH_I> & fx_angle,
        const ap_fixed<IN_W, IN_I> & fx_re_in,
        const ap_fixed<IN_W, IN_I> & fx_im_in,
        ap_fixed<OUT_W, OUT_I> &     fx_re_out,
        ap_fixed<OUT_W, OUT_I> &     fx_im_out);

    CCordicRotate() {}
    virtual ~CCordicRotate() {};
};

#endif
