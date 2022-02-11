#ifndef _ROM_GENERATOR_MC_HALF_PI
#define _ROM_GENERATOR_MC_HALF_PI

#include <climits>
#include <cmath>
#include <complex>
#include <cstdint>

template <unsigned In_W, unsigned NStages, unsigned Tq>
class CRomGeneratorMCHalfPi {
    static_assert(In_W > 0, "Inputs can't be on zero bits.");
    static_assert(NStages < 8, "7 stages of CORDIC is the maximum supported.");
    static_assert(NStages > 1, "2 stages of CORDIC is the minimum.");
    static_assert(NStages > 1, "2 stages of CORDIC is the minimum.");

public:
    static constexpr double   rotation     = M_PI_2;
    static constexpr double   q            = Tq;
    static constexpr uint32_t max_length   = 4 * Tq;                    // 2pi / (pi / 2) * q
    static constexpr int64_t  scale_factor = int64_t(1U << (In_W - 1)); // 2pi / (pi / 2) * q

private:
    constexpr std::complex<int64_t> cordic_MC(const std::complex<int64_t> & x_in,
                                              uint8_t                       counter) {

        int64_t A = x_in.real();
        int64_t B = x_in.imag();

        const uint8_t R    = counter;
        uint8_t       mask = 0x80;
        if ((R & mask) == mask) {
            A = -A;
            B = -B;
        }

        for (uint16_t u = 1; u < NStages + 1; u++) {
            mask = mask >> 1;

            const int64_t Ri = (R & mask) == mask ? 1 : -1;

            const int64_t I = A + Ri * (B / int64_t(1U << (u - 1)));
            B               = B - Ri * (A / int64_t(1U << (u - 1)));
            A               = I;
        }

        return {A, B};
    }

public:
    uint8_t rom[max_length];

    CRomGeneratorMCHalfPi() {
        for (unsigned n = 0; n < max_length; n++) {
            const double re_x = floor(double(scale_factor - 1) * cos(-rotation / double(q) * double(n)));
            const double im_x = floor(double(scale_factor - 1) * sin(-rotation / double(q) * double(n)));

            const std::complex<int64_t> x {re_x, im_x};

            double  error = 1000.;
            uint8_t rom_v = 0x0;

            std::complex<double> res;
            for (uint32_t v = 0; v < max_length; v++) {
                const std::complex<int64_t> res_int = cordic_MC(x, v);

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

template <unsigned In_W, unsigned NStages, unsigned Tq>
void generate_rom_header_mc(const char * filename = "rom_cordic.h") {
    const CRomGeneratorMCHalfPi<In_W, NStages, Tq> rom;

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
void generate_rom_header_mc_raw(const char * filename = "rom_cordic.txt") {
    const CRomGeneratorMCHalfPi<In_W, NStages, Tq> rom;

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

#endif // _ROM_GENERATOR_MC_HALF_PI