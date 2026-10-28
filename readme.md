# Warp tunnel simulation
A small graphical app written on C lang. This app renders the beautiful view of the Warp.

![Warp tunnel example](img/Warp.png)

# Dependencies
- SDL2 lib

# Build
```bash 
    mkdir build
    cd build

    cmake -G "Unix Makefiles" -S ../ -B Release -DCMAKE_BUILD_TYPE=Release

    cmake --build ./Release
```

# IR generator build

```bash
    cmake --build build/Release --target app_ir_gen
    ./build/Release/bin/app_ir_gen > IR/app_gen.ll
```
