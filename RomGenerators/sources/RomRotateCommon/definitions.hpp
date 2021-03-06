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

#ifndef _ROMCORDIC_DEFINITIONS_HPP_
#define _ROMCORDIC_DEFINITIONS_HPP_

#include <cstddef>
#include <cstdint>

namespace rom_cordic_rotate {

#ifdef M_PI
constexpr double pi = M_PI;
#else
constexpr double pi = 3.14159265358979323846;
#endif

constexpr double half_pi = pi / 2;
constexpr double inv_pi  = 1 / pi;
constexpr double two_pi  = 2 * pi;
constexpr double inv_2pi = 0.5 * inv_pi;

#if __cplusplus >= 201402L || XILINX_MAJOR > 2019  

constexpr uint32_t needed_bits(uint32_t value) {
    uint32_t result = 0;
    while (value > 0) {
        result++;
        value >>= 1;
    }
    return result;
}

constexpr bool is_pow_2(uint32_t value) {
    return (1U << (needed_bits(value) - 1)) == value;
}

#endif

template <uint32_t value>
constexpr uint32_t needed_bits() { return needed_bits<(value >> 1)>() + 1; }

template <>
constexpr uint32_t needed_bits<0>() { return 0; }

template <>
constexpr uint32_t needed_bits<1>() { return 1; }

template <>
constexpr uint32_t needed_bits<2>() { return 2; }

template <uint32_t value>
constexpr bool is_pow_2() {
    return (1U << (needed_bits<value>() - 1)) == value;
}

template <>
constexpr bool is_pow_2<0>() {
    return false;
}

} // namespace rom_cordic_rotate

#endif // _ROMCORDIC_DEFINITIONS_HPP_
