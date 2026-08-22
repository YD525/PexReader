# PEX parser fixtures

These fixtures are original, synthetic test data created for this repository. They contain no Bethesda game assets or
third-party mod content and are distributed under the repository's GNU Lesser General Public License version 3.

The payloads use whitespace-separated hexadecimal bytes so every format field remains reviewable. The test suite
materializes them as temporary `.pex` files before invoking the parser.

- `skyrim-3.1.pex.hex` covers the supported Skyrim Papyrus 3.1 header with debug information, properties, all variable
  data types, and fixed and variable-argument instructions.
- `skyrim-se-3.2-unicode.pex.hex` covers the supported Skyrim Special Edition Papyrus 3.2 header and a UTF-8
  string-table entry while retaining the same complete model structure.
- `malformed-truncated.pex.hex` is the 3.2 fixture with its final byte removed and must be rejected.

New parser and ownership bug fixes must add the smallest synthetic fixture or boundary mutation that reproduces the
defect. Crash and fuzzing inputs must be minimized and converted to this reviewable hexadecimal form before commit.
The scheduled sanitizer run derives each case from these small seeds with exactly one changed byte. Before invoking
the direct parser and exported C entry point, it records the current hexadecimal input, seed name, byte offset, and
replacement value so a crash leaves a minimized, reproducible mutation in the failure diagnostics.
