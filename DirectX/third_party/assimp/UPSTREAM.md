# Assimp dependency

- Upstream: https://github.com/assimp/assimp.git
- Version: v6.0.5
- Commit: `392a658f9c271be965271f45e7521a1b80ea4392`
- License: modified 3-clause BSD (`LICENSE`)

The public headers and license in this directory come from the pinned upstream
revision above. The bundled x64 Debug and Release import libraries and DLLs were
built from the same revision with Visual Studio 2022 (v143).

Only the x64 configurations use Assimp. Win32 keeps the existing OBJ renderer.
