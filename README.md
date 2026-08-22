# PexReader

PexReader is designed to replace the Lexicon AI Translator PexReader.cs class.

> ⚠️ Note: This library is still under active development.

Most of the logic in this library is based on [sse-pex-interface](https://github.com/Cutleast/sse-pex-interface).

Encoding and identification section [SSE-Auto-Translator](https://github.com/Cutleast/SSE-Auto-Translator).

## Building from source

PexReader requires Visual Studio 2022 with the Desktop development with C++ workload.

Build the x64 Release configuration:

```powershell
.\scripts\Invoke-NativeBuild.ps1 -Configuration Release -Platform x64
.\scripts\Run-Tests.ps1 -Configuration Release -Platform x64
```

The resulting library is written to `x64\Release\PEX.Interop.dll`.
The parser suite runs non-interactively through the Visual Studio C++ test runner. Its synthetic PEX fixtures and
their license status are documented in `PexReader.Tests/Fixtures/README.md`.

## Native ABI

[PexReaderApi.h](PexReader/PexReaderApi.h) is the canonical C-compatible ABI contract. It defines the ABI version,
calling convention, fixed-width types, value layout, encodings, buffer units, status values, and ownership rules.
Consumers can compare `C_GetAbiVersion()` with `PEX_READER_ABI_VERSION` before using the remaining exports.

The build script enforces warning level 4 and treats compiler and linker warnings as errors. Pull requests run the
Release x64 build, all regression tests, and MSVC native analysis on Windows Server 2022 with the Visual Studio 2022
v143 toolset. A weekly and manually dispatchable job repeats the tests under AddressSanitizer and applies
deterministic single-byte mutations to both supported fixtures through the native parser and exported C entry point.
If the process fails, the last one-byte reproducer and its mutation metadata are retained with the workflow
diagnostics.

## Releases

Push a version tag matching `v*` to build the x64 library and create a GitHub Release. Each release contains
`PEX.Interop.dll`, `PexReaderApi.h`, and a SHA-256 checksum for each file.

# Contributors:

YD525 (https://github.com/YD525).

Cutleast (https://github.com/cutleast).
