# CORDIC Rotate APFX

A free way to implement a CORDIC-based rotation using HLS, with bit-accurate precision.

## Goal

[CORDIC](https://en.wikipedia.org/wiki/CORDIC) (COordinate Rotation DIgital Computer) is an efficient way to implement hardware complex rotations (e.g. `z * exp(jw)`, with `z = x + jy` a complex and `w` a real angle). It is also useful for microcontrollers or microprocessors lacking floating-point units, as such multiplications can consume a noticeable amount of CPU cycles.

This implementation in C++14 (`-std=c++14`) is suitable for hardware simulation and (with the right lib's and a few tweaks) for synthesis.

This repository mainly hosts `CCordicRotateHalfPiRom`, CORDIC-based rotation units which rely on a ROM.
Currently, It ranges from 2 to 7 stages, and the word length is a template.
It depends on ROM generators, each one producing a ROM table which contains control signals for each CORDIC stage, depending on the input angle. There are generators:

- A true `constexpr` one, that is entirely processed by the compiler.
- A Monte-Carlo one, that is evaluated at runtime and can be used to produced ROM-headers, suitable for Autoconf/CMake header generations.

Only rotations of pi and pi/2 are currently supported, but support for any pi/2^k can be easily added.

`CCordicRotate` is an unfinished template that would implement a *smart* CORDIC, which can work without a ROM.

## Test suite and dependencies

The [Catch2](https://github.com/catchorg/Catch) test framework has been used in conjunction with CTest to provides unit tests.
The [GitHub mirror of the repository](https://github.com/DrasLorus/CORDIC_Rotate_APFX) also make use of GitHub Actions and Docker as a CI/CD solution.

- Has been tested successfully compiled with:
  - GNU GCC 9.4, 10.1, 10.2 and 11.2,
  - LLVM Clang 12.0 and 13.0,
- Uses Catch v2.13.7,
- Depends on Xilinx HLS arbitrary precision types, available as FOSS [here provided by Xilinx](https://github.com/Xilinx/HLS_arbitrary_Precision_Types) or [here patched by myself](https://github.com/DrasLorus/HLS_arbitrary_Precision_Types). Note: Xilinx also provides proprietary versions of those headers, suitable for synthesis and implementation, bundled with their products.

## License and copyright

Copyright 2022 Camille "DrasLorus" Monière.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU
Lesser General Public License as published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.

A copy of the license is available [here in Markdown](lgpl-3.0.md) or [here in plain text](LICENSE).
