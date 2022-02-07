#include "CCordicRotate/CCordicRotate.hpp"
#include <fstream>
#include <iostream>

#include <catch2/catch.hpp>

using namespace std;

typedef CCordicRotate<8, 14, 4, 17, 5, 19, 7, 12> cordic_t;

using Catch::Matchers::Floating::WithinAbsMatcher;

TEST_CASE("_8_14_10_17_5_19_12_12") {

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

        cordic_t::process(angles_in[iter], values_re_in[iter], values_im_in[iter], values_re_out[iter], values_im_out[iter]);

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
