```bash
cmake -S ASM2IR/IRGen -B build/asm-irgen -DCMAKE_BUILD_TYPE=Release
cmake --build build/asm-irgen
```

```bash
# 3
./build/asm-irgen/sim_jit_calls ASM2IR/app.asm /tmp/app_calls.ll

# 4
./build/asm-irgen/sim_jit_ir ASM2IR/app.asm /tmp/app_native.ll
```