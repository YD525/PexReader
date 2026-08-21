# PexReader

PexReader is designed to replace the Lexicon AI Translator PexReader.cs class.

> ⚠️ Note: This library is still under active development.

Most of the logic in this library is based on [sse-pex-interface](https://github.com/Cutleast/sse-pex-interface).

Encoding and identification section [SSE-Auto-Translator](https://github.com/Cutleast/SSE-Auto-Translator).

## Building from source

PexReader requires Visual Studio 2022 with the Desktop development with C++ workload.

Build the x64 Release configuration:

```powershell
msbuild .\PexReader.sln /m /p:Configuration=Release /p:Platform=x64
```

The resulting library is written to `x64\Release\PEX.Interop.dll`.

## Releases

Push a version tag matching `v*` to build the x64 library and create a GitHub Release. Each release contains
`PEX.Interop.dll` and `PEX.Interop.dll.sha256`.

# Contributors:

YD525 (https://github.com/YD525).

Cutleast (https://github.com/cutleast).
