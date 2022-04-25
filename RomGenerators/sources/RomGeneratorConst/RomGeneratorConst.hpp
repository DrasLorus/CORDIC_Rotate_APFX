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

#ifndef _ROM_GENERATOR_CONST_
#define _ROM_GENERATOR_CONST_

#include <array>
#include <climits>
#include <cmath>
#include <complex>
#include <cstdint>

#include <cassert>

#include "RomRotateCommon/definitions.hpp"

namespace rcr = rom_cordic_rotate;

template <unsigned In_W, unsigned NStages, unsigned Tq, unsigned divider = 2>
class CRomGeneratorConst {
    static_assert(In_W > 0, "Inputs can't be on zero bits.");
    static_assert(NStages < 8, "7 stages of CORDIC is the maximum supported.");
    static_assert(NStages > 1, "2 stages of CORDIC is the minimum.");
    static_assert(rcr::is_pow_2<divider>(), "divider must be a power of 2.");

public:
    static constexpr double rotation = rcr::pi / divider;
    static constexpr double q        = Tq;

    static constexpr unsigned max_length   = 2 * divider * Tq; // 2pi / (pi / divider) * q
    static constexpr unsigned addr_length  = rcr::needed_bits<max_length - 1>();
    static constexpr int64_t  scale_factor = int64_t(1U << (In_W - 1));

    static constexpr double atanDbl[28] {
        0.78539816339745, 0.46364760900081, 0.24497866312686, 0.12435499454676,
        0.06241880999596, 0.03123983343027, 0.01562372862048, 0.00781234106010,
        0.00390623013197, 0.00195312251648, 0.00097656218956, 0.00048828121119,
        0.00024414062015, 0.00012207031189, 0.00006103515617, 0.00003051757812,
        0.00001525878906, 0.00000762939453, 0.00000381469727, 0.00000190734863,
        0.00000095367432, 0.00000047683716, 0.00000023841858, 0.00000011920929,
        0.00000005960464, 0.00000002980232, 0.00000001490116, 0.00000000745058};

private:
    constexpr uint8_t cordic_rom_gen(double rot_in) const {

        double A = scale_factor - 1;
        double B = 0;

        uint8_t       R        = 0;
        const uint8_t sig_mask = 0x01;

        double beta = rot_in;

#if 0
        printf("Step 0 - %03u : %02x : %8lf\n", R, R, beta);
#endif
        if ((beta < -rcr::two_pi) || (rcr::two_pi <= beta)) {
            fprintf(stderr, "rotation must be inside ] -2*pi; 2*pi ]");
            exit(EXIT_FAILURE);
        }

        if ((beta <= -rcr::pi) || (beta > rcr::pi)) {
            beta = beta < 0. ? beta + rcr::two_pi : beta - rcr::two_pi;
        }

        if ((beta < -rcr::half_pi) || (beta > rcr::half_pi)) {
            R    = R | sig_mask;
            beta = beta < 0 ? beta + rcr::pi : beta - rcr::pi;
            // A    = -A;
            // B    = -B;
        } else {
            R = R & (~sig_mask);
        }

        for (uint8_t u = 1; u < NStages + 1; u++) {
#if 0
            printf("Step %d - %03u : %02x : %8lf\n", u, R, R, beta);
#endif
            const uint8_t mask  = (1U << u);
            const uint8_t nmask = ~mask;

            assert((mask & nmask) == 0x00);
            assert((mask | nmask) == 0xFF);

            const double sigma = beta < 0 ? -1. : 1;

            R = beta < 0 ? R | mask : R & nmask;

            const double factor = sigma / double(1U << (u - 1));

            const double I = A + B * factor;
            B              = B - A * factor;
            A              = I;

            beta = beta - sigma * atanDbl[u - 1];
        }
#if 0
        printf("\nFINAL - %03u : %02x : %8lf\n\n", R, R, beta);
#endif
        return R;
    }

public:
    uint8_t rom[max_length];

    constexpr CRomGeneratorConst() : rom() {
        for (unsigned n = 0; n < max_length; n++) {
            const double chip_rotation = rotation / double(q) * double(n);
            rom[n]                     = cordic_rom_gen(chip_rotation);
        }
    }
};

template <unsigned In_W, unsigned NStages, unsigned Tq, unsigned divider = 2>
void generate_rom_header_cst(const char * filename) {
    constexpr CRomGeneratorConst<In_W, NStages, Tq> rom {};

    FILE * rom_file = fopen(filename, "w");
    if (!bool(rom_file)) {
        perror("Can't open the rom file for writing.");
        exit(EXIT_FAILURE);
    }

    char upper_file_def[64];
    snprintf(upper_file_def, 64, "CORDIC_ROMS_CST_%u_%u_%u_%u", In_W, NStages, Tq, divider);

    char rom_name[64];
    snprintf(rom_name, 64, "cst_%u_%u_%u_%u", In_W, NStages, Tq, divider);

    fprintf(rom_file, "/** @file %s\n * THIS FILE IS GENERATED AUTOMATICALY, DO NOT EDIT IT!\n */\n", filename);

    fprintf(rom_file, "#ifndef %s\n#define %s\n\n", upper_file_def, upper_file_def);
    fprintf(rom_file, "#include <cstdint>\n\n");
    fprintf(rom_file, "namespace cordic_roms {\n\n");

    fprintf(rom_file, "constexpr uint64_t %s_size = %d;\n\n", rom_name, rom.max_length);

    fprintf(rom_file, "constexpr uint8_t %s[%d] = {\n  ", rom_name, rom.max_length);
    for (uint16_t u = 0; u < rom.max_length - 1; u++) {
        if (((u & 7) == 0) && u != 0) {
            fprintf(rom_file, "\n  ");
        }
        fprintf(rom_file, "%3d, ", uint16_t(rom.rom[u]));
    }
    fprintf(rom_file, "%3d};\n", uint16_t(rom.rom[rom.max_length - 1]));

    fprintf(rom_file, "\n} // namespace cordic_roms\n\n");
    fprintf(rom_file, "#endif // %s\n\n", upper_file_def);
}

template <unsigned In_W, unsigned NStages, unsigned Tq, unsigned divider = 2>
void generate_rom_header_cst_raw(const char * filename = "rom_cordic.txt") {
    constexpr CRomGeneratorConst<In_W, NStages, Tq, divider> rom {};

    FILE * rom_file = fopen(filename, "w");
    if (!bool(rom_file)) {
        perror("Can't open the rom file for writing.");
        exit(EXIT_FAILURE);
    }

    for (uint16_t u = 0; u < rom.max_length - 1; u++) {
        fprintf(rom_file, "%03d\n", uint16_t(rom.rom[u]));
    }
    fprintf(rom_file, "%03d\n\n", uint16_t(rom.rom[rom.max_length - 1]));
}

#endif // _ROM_GENERATOR_CONST_