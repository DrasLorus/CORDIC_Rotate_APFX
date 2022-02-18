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

#ifndef C_CORDIC_ROTATE_ROM_TEMPLATE
#define C_CORDIC_ROTATE_ROM_TEMPLATE

enum rom_types {
    mc,
    cst
};

template <unsigned TIn_I, rom_types type, unsigned TIn_W, unsigned Tnb_stages, unsigned Tq>
class CCordicRotateRom {
    static_assert(TIn_W > 0, "Inputs can't be on zero bits.");
    static_assert(Tnb_stages < 8, "7 stages of CORDIC is the maximum supported.");
    static_assert(Tnb_stages > 1, "2 stages of CORDIC is the minimum.");
};

#endif // C_CORDIC_ROTATE_ROM_TEMPLATE