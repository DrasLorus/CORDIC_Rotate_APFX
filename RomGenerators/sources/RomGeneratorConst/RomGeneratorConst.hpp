#ifndef _ROM_GENERATOR_CONST_
#define _ROM_GENERATOR_CONST_

#include <array>
#include <climits>
#include <cmath>
#include <complex>
#include <cstdint>

#include <cassert>

template <unsigned In_W, unsigned NStages, unsigned Tq>
class CRomGeneratorConst {
    static_assert(In_W > 0, "Inputs can't be on zero bits.");
    static_assert(NStages < 8, "7 stages of CORDIC is the maximum supported.");
    static_assert(NStages > 1, "2 stages of CORDIC is the minimum.");

public:
    static constexpr double   pi           = 3.14159265358979323846;
    static constexpr double   two_pi       = 2 * pi;
    static constexpr double   half_pi      = pi * 0.5;
    static constexpr double   rotation     = half_pi;
    static constexpr double   q            = Tq;
    static constexpr uint32_t max_length   = 4 * Tq;                    // 2pi / (pi / 2) * q
    static constexpr int64_t  scale_factor = int64_t(1U << (In_W - 1)); // 2pi / (pi / 2) * q

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

        uint8_t R    = 0;
        const uint8_t sig_mask = 0x80;

        double beta = rot_in;

#if 0
        printf("Step 0 - %03u : %02x : %8lf\n", R, R, beta);
#endif
        if ((beta < -two_pi) || (two_pi <= beta)) {
            fprintf(stderr, "rotation must be inside ] -2*pi; 2*pi ]");
            exit(EXIT_FAILURE);
        }

        if ((beta <= -pi) || (beta > pi)) {
            beta = beta < 0. ? beta + two_pi : beta - two_pi;
        }

        if ((beta < -half_pi) || (beta > half_pi)) {
            R    = R | sig_mask;
            beta = beta < 0 ? beta + pi : beta - pi;
            // A    = -A;
            // B    = -B;
        } else {
            R = R & (~sig_mask);
        }

        for (uint8_t u = 1; u < NStages + 1; u++) {
#if 0
            printf("Step %d - %03u : %02x : %8lf\n", u, R, R, beta);
#endif
            const uint8_t mask  = (1U << (7 - u));
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

template <unsigned In_W, unsigned NStages, unsigned Tq>
void generate_rom_header_const(const char * filename = "rom_cordic.h") {
    constexpr CRomGeneratorConst<In_W, NStages, Tq> rom{};

    FILE * rom_file = fopen(filename, "w");
    if (!bool(rom_file)) {
        perror("Can't open the rom file for writing.");
        exit(EXIT_FAILURE);
    }

    // fprintf(rom_file, "#ifndef ROM_CORDIC\n#define ROM_CORDIC\n\n");

    fprintf(rom_file, "constexpr uint8_t rom_cordic[%d] = {\n  ", rom.max_length);
    for (uint16_t u = 0; u < rom.max_length - 1; u++) {
        if (((u & 7) == 0) && u != 0) {
            fprintf(rom_file, "\n  ");
        }
        fprintf(rom_file, "%3d, ", uint16_t(rom.rom[u]));
    }
    fprintf(rom_file, "%3d};\n", uint16_t(rom.rom[rom.max_length - 1]));

    // fprintf(rom_file, "#endif // ROM_CORDIC");
}

template <unsigned In_W, unsigned NStages, unsigned Tq>
void generate_rom_header_const_raw(const char * filename = "rom_cordic.txt") {
    constexpr CRomGeneratorConst<In_W, NStages, Tq> rom{};

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