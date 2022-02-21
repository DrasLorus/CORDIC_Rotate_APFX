# CORDIC ROM Headers

This directory contains build-time generated CORDIC ROM headers.
They are in the form `cordic_rom_${ROM_TYPE}_${CORDIC_W}_${CORDIC_STAGES}_${CORDIC_Q}.hpp` and contain (besides the usual double-inclusion protection) a table `constexpr uint8_t ${ROM_TYPE}_${CORDIC_W}_${CORDIC_STAGES}_${CORDIC_Q}` under the namespace `cordic_roms`. It is filled with corresponding CORDIC control signals.
This table is generated using its corresponding `rom_generator`, itself built and called by the build system.

*Note: This directory is usually empty, but will be filled automatically when needed.*
