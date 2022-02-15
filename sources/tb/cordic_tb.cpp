#include "CCordicRotate/CCordicRotate.hpp"
#include "CCordicRotateHalfPiRom/CCordicRotateHalfPiRom.hpp"
#include <fstream>
#include <iostream>

#include <catch2/catch.hpp>

using namespace std;

using Catch::Matchers::Floating::WithinAbsMatcher;

typedef CCordicRotate<8, 14, 4, 17, 5, 19, 7, 12> cordic_legacy;

TEST_CASE("Adaptive CORDIC work as intended", "[!hide][WIP]") {

    string input_fn  = "../data/input.dat";  // _8_14_4_17_5_19_7_12
    string output_fn = "../data/output.dat"; // _8_14_4_17_5_19_7_12

    constexpr unsigned n_lines = 100000;
    ap_fixed<17, 5>    values_re_in[n_lines];
    ap_fixed<17, 5>    values_im_in[n_lines];
    ap_fixed<14, 4>    angles_in[n_lines];
    ap_fixed<19, 7>    values_re_out[n_lines];
    ap_fixed<19, 7>    values_im_out[n_lines];

    double exp_re_out[n_lines];
    double exp_im_out[n_lines];

    ofstream FILE;

    ifstream INPUT(input_fn);
    ifstream RESULTS(output_fn);
    // Init test vector
    for (unsigned i = 0; i < n_lines; i++) {

        double a, b, c;
        INPUT >> a >> b >> c;
        values_re_in[i] = a;
        values_im_in[i] = b;
        angles_in[i]    = c;

        RESULTS >> a >> b;
        exp_re_out[i] = a;
        exp_im_out[i] = b;
    }
    INPUT.close();
    RESULTS.close();

    // Save the results to a file
    FILE.open("results.dat");

    // Executing the encoder
    for (unsigned iter = 0; iter < n_lines; iter++) {
        // Execute

        cordic_legacy::process(angles_in[iter],
                               values_re_in[iter], values_im_in[iter],
                               values_re_out[iter], values_im_out[iter]);

        // Display the results
        // cout << "Series " << iter;
        // cout << " Outcome: ";

        FILE << values_re_out[iter].to_float() << ", " << values_re_out[iter].to_float() << endl;

        REQUIRE_THAT(values_re_out[iter].to_float(), WithinAbsMatcher(exp_re_out[iter], 0.079997558593750));
        REQUIRE_THAT(values_im_out[iter].to_float(), WithinAbsMatcher(exp_im_out[iter], 0.079997558593750));
    }
    FILE.close();

    // Compare the results file with the golden results
    // int retval = 0;
    // Return 0 if the test passed
}

typedef CCordicRotateRomHalfPi<16, 4, 6, 64> cordic_rom;

TEST_CASE("ROM-based Cordic works with C-Types", "[CORDIC]") {
    SECTION("W:16 - I:4 - Stages:6 - q:64") {
        static constexpr cordic_rom cordic {};

        string input_fn  = "../data/input.dat";  // _8_14_4_17_5_19_7_12
        string output_fn = "../data/output.dat"; // _8_14_4_17_5_19_7_12

        constexpr unsigned n_lines = 100000;

        complex<double> values_in[n_lines];
        complex<double> values_out[n_lines];

        complex<double> results[n_lines];

        ofstream FILE;

        ifstream INPUT(input_fn);

        // Init test vector
        for (unsigned i = 0; i < n_lines; i++) {
            double a, b, r;
            INPUT >> a >> b >> r;

            const complex<double> c {a, b};
            values_in[i] = c;

            constexpr double rotation = cordic_rom::rom_cordic.rotation;
            constexpr double q        = cordic_rom::rom_cordic.q;

            const complex<double> e = exp(complex<double>(0., rotation / q * (i & 255)));
            results[i]              = c * e;
        }

        INPUT.close();

        // Save the results to a file
        FILE.open("results.dat");

        constexpr double abs_margin = double(1 << cordic.Out_I) * 2. / 100.;

        // Executing the encoder
        for (unsigned iter = 0; iter < n_lines; iter++) {
            // Execute

            values_out[iter] = cordic_rom::cordic(values_in[iter], (iter & 255));

            // Display the results
            // cout << "Series " << iter;
            // cout << " Outcome: ";

            FILE << values_out[iter].real() << " " << values_out[iter].imag() << " " << results[iter].real() << " " << results[iter].imag() << endl;

            REQUIRE_THAT(values_out[iter].real(), WithinAbsMatcher(results[iter].real(), abs_margin));
            REQUIRE_THAT(values_out[iter].imag(), WithinAbsMatcher(results[iter].imag(), abs_margin));
        }
        FILE.close();

        // Compare the results file with the golden results
        // int retval = 0;
        // Return 0 if the test passed
    }
}

TEST_CASE("ROM-based Cordic works with AP-Types", "[CORDIC]") {
    constexpr unsigned n_lines = 100000;

    SECTION("W:16 - I:4 - Stages:6 - q:64") {

        static constexpr cordic_rom cordic {};

        string input_fn = "../data/input.dat";

        constexpr double   rotation = cordic_rom::rom_cordic.rotation;
        constexpr double   q        = cordic_rom::rom_cordic.q;
        constexpr uint64_t cnt_mask = 0xFF; // Value dependant of the way the ROM is initialized

        constexpr unsigned Out_W = cordic_rom::Out_W;
        constexpr unsigned In_W  = cordic_rom::In_W;

        ap_int<In_W>  values_re_in[n_lines];
        ap_int<In_W>  values_im_in[n_lines];
        ap_int<Out_W> values_re_out[n_lines];
        ap_int<Out_W> values_im_out[n_lines];

        double results_re[n_lines];
        double results_im[n_lines];

        ofstream out_stream;

        ifstream INPUT(input_fn);

        // Init test vector
        for (unsigned i = 0; i < n_lines; i++) {
            double a, b, r;
            INPUT >> a >> b >> r;

            const complex<double> c {a, b};
            values_re_in[i] = int64_t(a * double(cordic_rom::in_scale_factor));
            values_im_in[i] = int64_t(b * double(cordic_rom::in_scale_factor));

            const complex<double> e = c * exp(complex<double>(0., rotation / q * (i & cnt_mask)));
            results_re[i]           = e.real();
            results_im[i]           = e.imag();
        }

        INPUT.close();

        // Save the results to a file
        out_stream.open("results_ap.dat");
        // FILE * romf = fopen("rom.dat", "w");

        constexpr double abs_margin = double(1 << cordic.Out_I) * 2. / 100.;

        // Executing the encoder
        for (unsigned iter = 0; iter < n_lines; iter++) {
            // Execute
            const uint8_t counter = uint8_t(iter & cnt_mask);

            // if (iter < cnt_mask + 1)
            //     fprintf(romf, "%03d\n", (uint16_t) cordic.rom_cordic.rom[counter]);

            cordic_rom::cordic(
                values_re_in[iter], values_im_in[iter],
                counter,
                values_re_out[iter], values_im_out[iter]);

            // Display the results
            // cout << "Series " << iter;
            // cout << " Outcome: ";

            out_stream << values_re_out[iter].to_int64() << " " << values_im_out[iter].to_int64() << " " << results_re[iter] << " " << results_im[iter] << endl;

            REQUIRE_THAT(values_re_out[iter].to_double() * 5. / 8. / cordic_rom::out_scale_factor, WithinAbsMatcher(results_re[iter], abs_margin));
            REQUIRE_THAT(values_im_out[iter].to_double() * 5. / 8. / cordic_rom::out_scale_factor, WithinAbsMatcher(results_im[iter], abs_margin));
        }
        out_stream.close();
        // fclose(romf);

        // Compare the results file with the golden results
        // int retval = 0;
        // Return 0 if the test passed
    }

    SECTION("W:16 - I:4 - Stages:6 - q:64 - internal scaling") {
        // typedef CCordicRotateRomHalfPi<16, 4, 6, 64> cordic_rom;
        static constexpr cordic_rom cordic {};

        string input_fn = "../data/input.dat";

        constexpr double   rotation = cordic_rom::rom_cordic.rotation;
        constexpr double   q        = cordic_rom::rom_cordic.q;
        constexpr uint64_t cnt_mask = 0xFF; // Value dependant of the way the ROM is initialized

        constexpr unsigned Out_W = cordic_rom::Out_W;
        constexpr unsigned In_W  = cordic_rom::In_W;

        ap_int<In_W>  values_re_in[n_lines];
        ap_int<In_W>  values_im_in[n_lines];
        ap_int<Out_W> values_re_out[n_lines];
        ap_int<Out_W> values_im_out[n_lines];

        double results_re[n_lines];
        double results_im[n_lines];

        ofstream out_stream;

        ifstream INPUT(input_fn);

        // Init test vector
        for (unsigned i = 0; i < n_lines; i++) {
            double a, b, r;
            INPUT >> a >> b >> r;

            const complex<double> c {a, b};
            values_re_in[i] = int64_t(a * double(cordic_rom::in_scale_factor));
            values_im_in[i] = int64_t(b * double(cordic_rom::in_scale_factor));

            const complex<double> e = c * exp(complex<double>(0., rotation / q * (i & cnt_mask)));
            results_re[i]           = e.real();
            results_im[i]           = e.imag();
        }

        INPUT.close();

        // Save the results to a file
        out_stream.open("results_ap.dat");
        // FILE * romf = fopen("rom.dat", "w");

        constexpr double abs_margin = double(1 << cordic.Out_I) * 2. / 100.;

        // Executing the encoder
        for (unsigned iter = 0; iter < n_lines; iter++) {
            // Execute
            const uint8_t counter = uint8_t(iter & cnt_mask);

            // if (iter < cnt_mask + 1)
            //     fprintf(romf, "%03d\n", (uint16_t) cordic.rom_cordic.rom[counter]);

            cordic_rom::cordic(
                values_re_in[iter], values_im_in[iter],
                counter,
                values_re_out[iter], values_im_out[iter]);

            // Display the results
            // cout << "Series " << iter;
            // cout << " Outcome: ";

            out_stream << values_re_out[iter].to_int64() << " " << values_im_out[iter].to_int64() << " " << results_re[iter] << " " << results_im[iter] << endl;

            REQUIRE_THAT(cordic_rom::scale_cordic<Out_W>(values_re_out[iter]).to_double() / cordic_rom::out_scale_factor,
                         WithinAbsMatcher(results_re[iter],
                                          abs_margin));
            REQUIRE_THAT(cordic_rom::scale_cordic<Out_W>(values_im_out[iter]).to_double() / cordic_rom::out_scale_factor,
                         WithinAbsMatcher(results_im[iter],
                                          abs_margin));
        }
        out_stream.close();
        // fclose(romf);

        // Compare the results file with the golden results
        // int retval = 0;
        // Return 0 if the test passed
    }
}

TEST_CASE("ROM-based Cordic constexpr are evaluated during compilation.", "[CORDIC]") {
    SECTION("W:16 - I:4 - Stages:6 - q:64 - C-Types") {

        constexpr complex<int64_t> value_in = (1U << 12) * 97;
        constexpr uint8_t          angle    = 169;

        constexpr complex<int64_t> res1 = cordic_rom::cordic(value_in, angle);
        constexpr complex<int64_t> res2 = cordic_rom::cordic(value_in, angle);
        static_assert(res1 == res2, "Test");
        REQUIRE_FALSE(res1 == cordic_rom::cordic(complex<int64_t>(1, 0), angle));
        REQUIRE(res1 == cordic_rom::cordic(value_in, angle));
    }
}
