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

#ifndef _ROM_GENERATOR_ML
#define _ROM_GENERATOR_ML

#include <climits>
#include <cmath>
#include <complex>
#include <cstdint>

#include "RomRotateCommon/definitions.hpp"

namespace rcr = rom_cordic_rotate;

template <unsigned In_W, unsigned NStages, unsigned Tq, unsigned divider = 2>
class CRomGeneratorML {
    static_assert(In_W > 0, "Inputs can't be on zero bits.");
    static_assert(NStages < 8, "7 stages of CORDIC is the maximum supported.");
    static_assert(NStages > 1, "2 stages of CORDIC is the minimum.");
    static_assert(NStages > 1, "2 stages of CORDIC is the minimum.");
    static_assert(rcr::is_pow_2<divider>(), "divider must be a power of 2.");

public:
    static constexpr double rotation = rcr::pi / divider;
    static constexpr double q        = Tq;

    static constexpr unsigned max_length   = 2 * divider * Tq; // 2pi / (pi / divider) * q
    static constexpr unsigned addr_length  = rcr::needed_bits<max_length - 1>();
    static constexpr int64_t  scale_factor = int64_t(1U << (In_W - 1));

private:
    constexpr std::complex<int64_t> cordic_ML(const std::complex<int64_t> & x_in,
                                              uint8_t                       counter) {

        int64_t A = x_in.real();
        int64_t B = x_in.imag();

        const uint8_t R    = counter;
        uint8_t       mask = 0x01;
        if ((R & mask) == mask) {
            A = -A;
            B = -B;
        }

        for (uint16_t u = 1; u < NStages + 1; u++) {
            mask = mask << 1;

            const int64_t Ri = (R & mask) == mask ? 1 : -1;

            const int64_t I = A + Ri * (B / int64_t(1U << (u - 1)));
            B               = B - Ri * (A / int64_t(1U << (u - 1)));
            A               = I;
        }

        return {A, B};
    }

public:
    uint8_t rom[max_length];

    CRomGeneratorML() {
        for (unsigned n = 0; n < max_length; n++) {
            const double re_x = floor(double(scale_factor - 1) * cos(-rotation / double(q) * double(n)));
            const double im_x = floor(double(scale_factor - 1) * sin(-rotation / double(q) * double(n)));

            const std::complex<int64_t> x {int64_t(re_x), int64_t(im_x)};

            double  error = 1000.;
            uint8_t rom_v = 0x0;

            std::complex<double> res;
            for (uint32_t v = 0; v < max_length; v++) {
                const std::complex<int64_t> res_int = cordic_ML(x, v);

                const std::complex<double> res_dbl(double(res_int.real()) / double(scale_factor - 1),
                                                   double(res_int.imag()) / double(scale_factor - 1));

                const double curr_error = std::abs(std::arg(res_dbl));
                if (curr_error < error) {
                    error = curr_error;
                    rom_v = uint8_t(v);
                    res   = res_dbl;
                }
            }

            rom[n] = rom_v;
        }
    }
};

template <unsigned In_W, unsigned NStages, unsigned Tq, unsigned divider = 2>
void generate_rom_header_ml(const char * filename) {
    const CRomGeneratorML<In_W, NStages, Tq, divider> rom;

    FILE * rom_file = fopen(filename, "w");
    if (!bool(rom_file)) {
        perror("Can't open the rom file for writing.");
        exit(EXIT_FAILURE);
    }

    char upper_file_def[64];
    snprintf(upper_file_def, 64, "CORDIC_ROMS_ML_%u_%u_%u_%u", In_W, NStages, Tq, divider);

    char rom_name[64];
    snprintf(rom_name, 64, "ml_%u_%u_%u_%u", In_W, NStages, Tq, divider);

    fprintf(rom_file, "/** @file %s\n * THIS FILE IS GENERATED AUTOMATICALY, DO NOT EDIT IT!\n */\n", filename);

    fprintf(rom_file, "#ifndef %s\n#define %s\n\n", upper_file_def, upper_file_def);
    fprintf(rom_file, "#include <cstdint>\n\n");
    fprintf(rom_file, "namespace cordic_roms {\n\n");

    fprintf(rom_file, "constexpr uint64_t %s_size = %d;\n\n", rom_name, rom.max_length);

    fprintf(rom_file, "constexpr uint8_t  %s[%d] = {\n  ", rom_name, rom.max_length);
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
void generate_rom_header_ml_raw(const char * filename) {
    const CRomGeneratorML<In_W, NStages, Tq, divider> rom;

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

#endif // _ROM_GENERATOR_ML