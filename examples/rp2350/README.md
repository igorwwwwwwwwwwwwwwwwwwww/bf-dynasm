# RP2350 (RISC-V JIT)

This target runs a real on-device JIT on RP2350:
- embeds Brainfuck source at build time
- parses + optimizes AST on device
- DynASM-compiles RISC-V machine code in RAM
- executes the generated code

Notes:
- Final generated JIT code is allocated from SRAM (`malloc`).
- When enabled and available, DynASM internal growth buffers use PSRAM.
- On some setups, physical reset after flashing is more reliable than soft reboot for clean USB CDC output start.
- RISC-V DynASM backend support is vendored in this repo (`dasm_riscv*.lua`, `dasm_riscv.h`) and used by `bf_riscv*.dasc`.
- Source reference: https://github.com/plctlab/LuaJIT
- Upstream MR reference: https://github.com/LuaJIT/LuaJIT/pull/1267

## Setup

```sh
(cd examples/rp2350 && ./fetch-deps.sh)
source examples/rp2350/deps/env.sh
```

## Build + flash (default `examples/hello.b`)

```sh
cmake -S examples/rp2350 -B examples/rp2350/build-riscv -DPICO_BOARD=pico2 -DPICO_PLATFORM=rp2350-riscv
cmake --build examples/rp2350/build-riscv -j
cmake --build examples/rp2350/build-riscv --target flash
```

One-liner:
```sh
source examples/rp2350/deps/env.sh && cmake -S examples/rp2350 -B examples/rp2350/build-riscv -DPICO_BOARD=pico2 -DPICO_PLATFORM=rp2350-riscv -DBF_SOURCE=$(pwd)/examples/mandelbrot.b -DBF_ENABLE_TIMING=1 -DRP2350_DEBUG_OUTPUT=0 && cmake --build examples/rp2350/build-riscv -j && picotool load -f examples/rp2350/build-riscv/dynasm_brainfuck_rp2350.elf && picotool reboot -f
```

## Useful options

```sh
# Enable runtime timing line:
-DBF_ENABLE_TIMING=1

# Verbose JIT/compile logs:
-DRP2350_DEBUG_OUTPUT=1

# Disable AST optimize passes:
-DRP2350_JIT_OPTIMIZE=0

# Enable idle heartbeat log:
-DRP2350_IDLE_HEARTBEAT=1

# Enable PSRAM-backed DynASM buffers and set CS pin:
-DRP2350_USE_PSRAM_DASM=1 -DRP2350_PSRAM_CS_PIN=8
```

## Build a different BF program

```sh
cmake -S examples/rp2350 -B examples/rp2350/build-riscv -DPICO_BOARD=pico2 -DPICO_PLATFORM=rp2350-riscv -DBF_SOURCE=$(pwd)/examples/cat.b
cmake --build examples/rp2350/build-riscv -j
cmake --build examples/rp2350/build-riscv --target flash
```

## USB serial

```sh
ls /dev/cu.usbmodem*
picocom -b 115200 --imap lfcrlf --omap crcrlf /dev/cu.usbmodem1101
```

If `flash`/`picotool reboot -f` leaves USB in a bad state, press hardware reset and reconnect serial.

## Debug (OpenOCD + GDB)

OpenOCD:
```sh
./src/openocd -s tcl -f interface/cmsis-dap.cfg -f target/rp2350-riscv.cfg
```

GDB:
```sh
examples/rp2350/deps/riscv-toolchain/bin/riscv32-unknown-elf-gdb -q \
    -ex "file /Users/igor/code/dynasm-brainfuck/examples/rp2350/build-riscv/dynasm_brainfuck_rp2350.elf" \
    -ex "target extended-remote :3333"
```
