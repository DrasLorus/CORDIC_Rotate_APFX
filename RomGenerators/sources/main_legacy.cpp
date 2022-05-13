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

#include "RomGeneratorConst/RomGeneratorConst.hpp"
#include "RomGeneratorML/RomGeneratorML.hpp"

#include <fstream>
#include <iostream>
#include <string>

using namespace std;

#if __cplusplus >= 201402L || XILINX_MAJOR > 2019
#define OWN_CONSTEXPR constexpr
#else
#define OWN_CONSTEXPR
#endif

template <unsigned NStages>
OWN_CONSTEXPR complex<int64_t> cordic(
    complex<int64_t> x_in,
    uint8_t          counter,
    const uint8_t *  rom_cordic) {

    int64_t A = x_in.real();
    int64_t B = x_in.imag();

    const uint8_t R    = rom_cordic[counter];
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

    return {A, B};
}

#if __cplusplus >= 201402L || XILINX_MAJOR > 2019

template <unsigned NStages>
void checkConst() {
    constexpr CRomGeneratorConst<16, NStages, 64> rom;

    string   fn = "result_const_W16_S" + to_string(NStages) + "_Q64.dat";
    ofstream res_file(fn);

    for (unsigned u = 0; u < rom.max_length; u++) {
        auto res = cordic<NStages>(4096, u, rom.rom);
        res_file << double(res.real() * 155) / 1048576. << ", " << double(res.imag() * 155) / 1048576. << endl;
    }
}

#else

template <unsigned,unsigned,unsigned>
void generate_rom_header_cst_raw(const string &) {}

template <unsigned,unsigned,unsigned>
void generate_rom_header_cst(const string &) {}

template <unsigned>
void checkConst() {}

#endif


template <unsigned NStages>
void checkMC() {
    const CRomGeneratorML<16, NStages, 64, 2> rom;

    string   fn = "result_MC_W16_S" + to_string(NStages) + "_Q64.dat";
    ofstream res_file(fn);

    for (unsigned u = 0; u < rom.max_length; u++) {
        complex<int64_t> res = cordic<NStages>(4096, u, rom.rom);
        res_file << double(res.real() * 155) / 1048576. << ", " << double(res.imag() * 155) / 1048576. << endl;
    }
}


int main(int argc, char * argv[]) {

    uint8_t stages;
    bool    use_ml;
    bool    use_txt;

    switch (argc) {
        case 1:
            stages  = 6;
            use_ml  = false;
            use_txt = false;
            break;
        case 2:
            stages  = stoi(string(argv[1]));
            use_ml  = false;
            use_txt = false;
            break;
        case 3:
            stages  = stoi(string(argv[1]));
            use_ml  = stoi(string(argv[2])) > 0;
            use_txt = false;
            break;
        case 4:
            stages  = stoi(string(argv[1]));
            use_ml  = stoi(string(argv[2])) > 0;
            use_txt = stoi(string(argv[3])) > 0;
            break;
        default:
            cerr << "Error, too many arguments. Expected at most 3." << endl;
            exit(EXIT_FAILURE);
    }
    if (use_txt) {
        if (use_ml) {
            switch (stages) {
                case 2:
                    generate_rom_header_ml_raw<16, 2, 64>("rom_cordic_ml_W16_S2_Q64.txt");
                    checkMC<2>();
                    break;
                case 3:
                    generate_rom_header_ml_raw<16, 3, 64>("rom_cordic_ml_W16_S3_Q64.txt");
                    checkMC<3>();
                    break;
                case 4:
                    generate_rom_header_ml_raw<16, 4, 64>("rom_cordic_ml_W16_S4_Q64.txt");
                    checkMC<4>();
                    break;
                case 5:
                    generate_rom_header_ml_raw<16, 5, 64>("rom_cordic_ml_W16_S5_Q64.txt");
                    checkMC<5>();
                    break;
                case 6:
                    generate_rom_header_ml_raw<16, 6, 64>("rom_cordic_ml_W16_S6_Q64.txt");
                    checkMC<6>();
                    break;
                case 7:
                    generate_rom_header_ml_raw<16, 7, 64>("rom_cordic_ml_W16_S7_Q64.txt");
                    checkMC<7>();
                    break;
                default:
                    cerr << "Error, no less than 2 stages, no more than 7." << endl;
                    exit(EXIT_FAILURE);
            }
        } else {
            switch (stages) {
                case 2:
                    generate_rom_header_cst_raw<16, 2, 64>("rom_cordic_const_W16_S2_Q64.txt");
                    checkConst<2>();
                    break;
                case 3:
                    generate_rom_header_cst_raw<16, 3, 64>("rom_cordic_const_W16_S3_Q64.txt");
                    checkConst<3>();
                    break;
                case 4:
                    generate_rom_header_cst_raw<16, 4, 64>("rom_cordic_const_W16_S4_Q64.txt");
                    checkConst<4>();
                    break;
                case 5:
                    generate_rom_header_cst_raw<16, 5, 64>("rom_cordic_const_W16_S5_Q64.txt");
                    checkConst<5>();
                    break;
                case 6:
                    generate_rom_header_cst_raw<16, 6, 64>("rom_cordic_const_W16_S6_Q64.txt");
                    checkConst<6>();
                    break;
                case 7:
                    generate_rom_header_cst_raw<16, 7, 64>("rom_cordic_const_W16_S7_Q64.txt");
                    checkConst<7>();
                    break;
                default:
                    cerr << "Error, no less than 2 stages, no more than 7." << endl;
                    exit(EXIT_FAILURE);
            }
        }
    } else {

        if (use_ml) {
            switch (stages) {
                case 2:
                    generate_rom_header_ml<16, 2, 64>("rom_cordic_ml_W16_S2_Q64.hpp");
                    checkMC<2>();
                    break;
                case 3:
                    generate_rom_header_ml<16, 3, 64>("rom_cordic_ml_W16_S3_Q64.hpp");
                    checkMC<3>();
                    break;
                case 4:
                    generate_rom_header_ml<16, 4, 64>("rom_cordic_ml_W16_S4_Q64.hpp");
                    checkMC<4>();
                    break;
                case 5:
                    generate_rom_header_ml<16, 5, 64>("rom_cordic_ml_W16_S5_Q64.hpp");
                    checkMC<5>();
                    break;
                case 6:
                    generate_rom_header_ml<16, 6, 64>("rom_cordic_ml_W16_S6_Q64.hpp");
                    checkMC<6>();
                    break;
                case 7:
                    generate_rom_header_ml<16, 7, 64>("rom_cordic_ml_W16_S7_Q64.hpp");
                    checkMC<7>();
                    break;
                default:
                    cerr << "Error, no less than 2 stages, no more than 7." << endl;
                    exit(EXIT_FAILURE);
            }
        } else {
            switch (stages) {
                case 2:
                    generate_rom_header_cst<16, 2, 64>("rom_cordic_const_W16_S2_Q64.hpp");
                    checkConst<2>();
                    break;
                case 3:
                    generate_rom_header_cst<16, 3, 64>("rom_cordic_const_W16_S3_Q64.hpp");
                    checkConst<3>();
                    break;
                case 4:
                    generate_rom_header_cst<16, 4, 64>("rom_cordic_const_W16_S4_Q64.hpp");
                    checkConst<4>();
                    break;
                case 5:
                    generate_rom_header_cst<16, 5, 64>("rom_cordic_const_W16_S5_Q64.hpp");
                    checkConst<5>();
                    break;
                case 6:
                    generate_rom_header_cst<16, 6, 64>("rom_cordic_const_W16_S6_Q64.hpp");
                    checkConst<6>();
                    break;
                case 7:
                    generate_rom_header_cst<16, 7, 64>("rom_cordic_const_W16_S7_Q64.hpp");
                    checkConst<7>();
                    break;
                default:
                    cerr << "Error, no less than 2 stages, no more than 7." << endl;
                    exit(EXIT_FAILURE);
            }
        }
    }

    return EXIT_SUCCESS;
}