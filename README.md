# DynASM Brainfuck JIT Compiler

A high-performance Brainfuck interpreter with Just-In-Time compilation using DynASM for both ARM64 and x64 architectures.

## Features

- **JIT Compilation**: Compiles Brainfuck source to native machine code at runtime
- **AST Optimizations**: Advanced optimizations including multiplication loops, offset operations, and constant propagation
- **Multi-Architecture**: Supports both ARM64 and x64 architectures
- **Profiling Support**: Built-in profiler with flame graph compatibility and PC-to-AST mapping
- **Debug Mode**: Dumps AST and compiled machine code for analysis
- **Debug Logging**: Interactive breakpoints with `!` symbol for execution tracing
- **High Performance**: Direct native code execution with minimal overhead
- **Dual Build Systems**: Both Make (simple) and Bazel (advanced) with multi-platform and sanitizer support

## Building

### Prerequisites

- GCC or Clang
- Bazel (build system)
- Bison (parser generator)
- Flex (lexical analyzer generator)

Note: LuaJIT is built from source by the build system

### Build Commands

#### Make (Simple)
```bash
# Install dependencies on macOS
brew install flex bison

# Build native version
make

# Build AMD64 version on ARM64 Mac
make amd64-darwin
```

#### Bazel (Advanced)
```bash
# Install dependencies on macOS
brew install flex bison bazel

# Build native version
bazel build //:bf

# Build with debug mode
bazel build --config=debug //:bf

# Build with optimizations
bazel build --config=opt //:bf

# Build AddressSanitizer version
bazel build --config=asan //:bf

# Build ThreadSanitizer version
bazel build --config=tsan //:bf

# Build MemorySanitizer version
bazel build --config=msan //:bf

# Cross-compile for AMD64 Darwin
bazel build --config=amd64-darwin //:bf

# Combine configs (e.g., AMD64 with AddressSanitizer)
bazel build --config=amd64-darwin --config=asan //:bf
```

## Usage

### Basic Usage

```bash
# Run Brainfuck program directly with Bazel
bazel run //:bf examples/hello.b

# Run with debug mode (dumps AST and machine code)
bazel run //:bf -- --debug examples/fizzbuzz.b

# Run without optimizations
bazel run //:bf -- --no-optimize examples/hello.b

# Show help
bazel run //:bf -- --help

# Run with timing information
bazel run //:bf -- --timing examples/hello.b

# Set custom memory size (in bytes)
bazel run //:bf -- --memory 32768 examples/hello.b

# Or build first, then run the binary directly
bazel build //:bf
bazel-bin/bf examples/hello.b
```

### Profiling

```bash
# Run with profiling (generates flame graph compatible output)
bazel run //:bf -- --profile profile.folded examples/fizzbuzz.b

# Or with built binary
bazel build //:bf
bazel-bin/bf --profile profile.folded examples/fizzbuzz.b
```

## Optimizations

The compiler includes several AST-level optimizations:

- **Run-length encoding**: Consecutive `+`, `-`, `>`, `<` operations are combined
- **Loop optimizations**: `[-]` becomes direct cell clearing (`SET_CONST(0)`)
- **Copy operations**: `[-<+>]` becomes optimized copy cell operations with arbitrary offsets
- **Multiplication loops**: Patterns like `++++[>+++<-]` become individual `MUL` and `COPY_CELL` operations
- **Unified MUL/COPY**: `MUL` with multiplier=1 automatically uses more efficient `COPY_CELL`
- **Offset operations**: `>+<` sequences become direct offset additions without pointer movement
- **Constant propagation**: `[-]+++` becomes direct constant assignment (`SET_CONST(3)`)
- **SET_CONST coalescing**: `SET_CONST(0) + ADD_VAL(-1)` becomes `SET_CONST(-1)` at same offset

## Architecture Support

### ARM64 (Apple Silicon, ARM64 Linux)
- Native execution with AAPCS64 ABI compliance
- Uses x19 register for memory pointer
- Register-indirect function calls with 64-bit address loading

### x64 (Intel/AMD 64-bit)
- System V AMD64 ABI compliance
- Stack-based memory pointer for alignment
- Register-indirect function calls

```bash
# AMD64 version (automatically uses Rosetta on ARM64 Macs)
bazel build --config=amd64-darwin //:bf
bazel-bin/bf examples/hello.b
```

## Testing

### Local Testing
```bash
# Test native version
bazel run //:bf examples/hello.b

# Test AMD64 Darwin version (via Rosetta)
bazel run --config=amd64-darwin //:bf examples/hello.b

# Test AddressSanitizer versions for debugging
bazel run --config=asan //:bf examples/hello.b

# Test AMD64 with AddressSanitizer (via Rosetta)
bazel run --config=amd64-darwin --config=asan //:bf examples/hello.b
```

## Docker Multi-Platform Support

The project includes a multi-platform Dockerfile that automatically detects the target architecture and builds the appropriate version.

### Quick Start with Docker

```bash
# Build multi-platform image (works on both x64 and ARM64)
docker buildx build --platform=linux/amd64,linux/arm64 -t bf-dynasm .

# Run on any platform (automatically selects correct architecture)
docker run --rm bf-dynasm

# Expected output: "Hello World!"
```

### Testing Different Brainfuck Programs

```bash
# Test with custom Brainfuck code
docker run --rm bf-dynasm sh -c 'echo "++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++." > test.b && ./bf test.b'

# Run with debug mode to see compiled machine code
docker run --rm bf-dynasm ./bf --debug hello.b
```

### Cross-Platform Testing

```bash
# Build once, test on both architectures
docker buildx build --platform=linux/amd64,linux/arm64 -t bf-dynasm .

# Test x64 version (with emulation if on ARM64 host)
docker run --platform=linux/amd64 bf-dynasm

# Test ARM64 version (with emulation if on x64 host)
docker run --platform=linux/arm64 bf-dynasm

# Both should output: "Hello World!"
```

## Implementation Details

### DynASM Integration
- Uses LuaJIT's DynASM for runtime assembly generation
- Automatic architecture detection via C preprocessor macros (`__x86_64__`, `__aarch64__`)
- Architecture-specific includes: `bf_amd64.c` for x64, `bf_arm64.c` for ARM64
- PC labels for loop management with proper nesting

### Memory Model
- 65,536 byte memory array by default (configurable via --memory option)
- Zero-initialized memory
- Bounds checking with guard pages for immediate crash detection

### Optimization Features
- Direct memory operations (no interpretation overhead)
- Efficient loop implementation with native conditional branches
- Minimal function call overhead for I/O operations

### Debug Mode
- AST dump showing optimization transformations
- Hex dump of compiled machine code
- Architecture identification
- Line:column information displayed in grey after each AST node

### Profiling System
- Signal-based profiling with PC-to-AST mapping
- Flame graph compatible output format
- Nested loop context tracking


## Performance

This JIT compiler provides significant performance improvements over traditional Brainfuck interpreters:

- **Native Speed**: Compiled code runs at native processor speed
- **No Interpretation Overhead**: Direct machine code execution
- **AST Optimizations**: Advanced optimizations reduce instruction count significantly
- **Optimized Loops**: Native conditional branches with specialized loop patterns
- **Efficient I/O**: Direct function calls with proper ABI compliance

## Benchmarks

The mandelbrot benchmark was run on an Apple M1 (2020 MacBook Air), measuring against:

- [optasmjit](https://github.com/eliben/code-for-blog/blob/main/2017/bfjit/optasmjit.cpp) by [Eli Bendersky](https://eli.thegreenplace.net/2017/adventures-in-jit-compilation-part-2-an-x64-jit/), via rosetta
- [BF-JIT](https://github.com/danthedaniel/BF-JIT) by [Daniel Angell](https://danangell.com/blog/posts/brainfuck-optimizing-jit/), via rosetta
- [bf-cranelift-jit](https://github.com/Rodrigodd/bf-compiler) by [Rodrigo Batista de Moraes](https://rodrigodd.github.io/2022/11/26/bf_compiler-part3.html), via rosetta
- [naegleria](https://github.com/igorwwwwwwwwwwwwwwwwwwww/naegleria) by me (llvm, php, wasm)
  - LLVM was compiled with clang 17.0.0
  - PHP was run against 8.4.12, with and without JIT
  - wasm was run against these runtimes
    - [wasmtime](https://github.com/bytecodealliance/wasmtime)
    - [iwasm](https://github.com/bytecodealliance/wasm-micro-runtime)
    - [wasmer](https://github.com/wasmerio/wasmer)
    - [WasmEdge](https://github.com/WasmEdge/WasmEdge)
    - [Wazero](https://github.com/tetratelabs/wazero)
- [brainfuck-php](https://github.com/igorw/brainfuck-php) was skipped
  - baseline ~50 minutes. eliminating NullLogger calls ~36 minutes. optimizing jumps ~13 minutes. PHP JIT ~10 minutes.

Results:

```
$ hyperfine --warmup 3 './bf examples/mandelbrot.b' './bf --unsafe examples/mandelbrot.b' './bf_amd64_darwin examples/mandelbrot.b' './bf_amd64_darwin --unsafe examples/mandelbrot.b' '~/code/BF-JIT/target/release/fucker examples/mandelbrot.b' '~/code/BF-JIT/target/x86_64-apple-darwin/release/fucker examples/mandelbrot.b' '~/code/eliben-code-for-blog/2017/bfjit/optinterp examples/mandelbrot.b' '~/code/eliben-code-for-blog/2017/bfjit/optinterp2 examples/mandelbrot.b' '~/code/eliben-code-for-blog/2017/bfjit/optinterp3 examples/mandelbrot.b' '~/code/eliben-code-for-blog/2017/bfjit/optasmjit examples/mandelbrot.b' '~/code/rodrigodd-bf-compiler/target/x86_64-apple-darwin/debug/bf-cranelift-jit examples/mandelbrot.b' '~/code/naegleria/mandelbrot-llvm' '~/code/naegleria/mandelbrot-llvm-opt' '~/code/naegleria/mandelbrot-llvm-opt3' 'php ~/code/naegleria/mandelbrot.php' 'php -d opcache.enable_cli=On -d opcache.jit=tracing ~/code/naegleria/mandelbrot.php' 'wasmtime ~/code/naegleria/mandelbrot.wasm' 'iwasm ~/code/naegleria/mandelbrot.wasm' 'wasmer ~/code/naegleria/mandelbrot.wasm' 'wasmedge --enable-jit ~/code/naegleria/mandelbrot.wasm' 'wazero run ~/code/naegleria/mandelbrot.wasm'

Benchmark 1: ./bf examples/mandelbrot.b
  Time (mean ± σ):     504.9 ms ±   0.4 ms    [User: 502.0 ms, System: 2.2 ms]
  Range (min … max):   504.4 ms … 505.6 ms    10 runs

Benchmark 2: ./bf --unsafe examples/mandelbrot.b
  Time (mean ± σ):     413.0 ms ±   0.3 ms    [User: 410.3 ms, System: 2.1 ms]
  Range (min … max):   412.4 ms … 413.7 ms    10 runs

Benchmark 3: ./bf_amd64_darwin examples/mandelbrot.b
  Time (mean ± σ):     554.8 ms ±   0.5 ms    [User: 547.5 ms, System: 4.8 ms]
  Range (min … max):   554.3 ms … 556.1 ms    10 runs

Benchmark 4: ./bf_amd64_darwin --unsafe examples/mandelbrot.b
  Time (mean ± σ):     420.2 ms ±   0.4 ms    [User: 413.3 ms, System: 4.4 ms]
  Range (min … max):   419.6 ms … 421.1 ms    10 runs

Benchmark 5: ~/code/BF-JIT/target/release/fucker examples/mandelbrot.b
  Time (mean ± σ):     480.4 ms ±   1.4 ms    [User: 477.1 ms, System: 2.4 ms]
  Range (min … max):   478.8 ms … 483.1 ms    10 runs

Benchmark 6: ~/code/BF-JIT/target/x86_64-apple-darwin/release/fucker examples/mandelbrot.b
  Time (mean ± σ):     490.6 ms ±   0.8 ms    [User: 482.7 ms, System: 5.4 ms]
  Range (min … max):   489.1 ms … 491.6 ms    10 runs

Benchmark 7: ~/code/eliben-code-for-blog/2017/bfjit/optinterp examples/mandelbrot.b
  Time (mean ± σ):     12.070 s ±  0.011 s    [User: 12.028 s, System: 0.040 s]
  Range (min … max):   12.050 s … 12.084 s    10 runs

Benchmark 8: ~/code/eliben-code-for-blog/2017/bfjit/optinterp2 examples/mandelbrot.b
  Time (mean ± σ):      6.398 s ±  0.002 s    [User: 6.376 s, System: 0.021 s]
  Range (min … max):    6.396 s …  6.402 s    10 runs

Benchmark 9: ~/code/eliben-code-for-blog/2017/bfjit/optinterp3 examples/mandelbrot.b
  Time (mean ± σ):      3.167 s ±  0.005 s    [User: 3.155 s, System: 0.011 s]
  Range (min … max):    3.162 s …  3.178 s    10 runs

Benchmark 10: ~/code/eliben-code-for-blog/2017/bfjit/optasmjit examples/mandelbrot.b
  Time (mean ± σ):     691.8 ms ±   1.7 ms    [User: 683.7 ms, System: 5.6 ms]
  Range (min … max):   687.2 ms … 693.2 ms    10 runs

Benchmark 11: ~/code/rodrigodd-bf-compiler/target/x86_64-apple-darwin/debug/bf-cranelift-jit examples/mandelbrot.b
  Time (mean ± σ):      2.808 s ±  0.009 s    [User: 2.784 s, System: 0.020 s]
  Range (min … max):    2.799 s …  2.830 s    10 runs

Benchmark 12: ~/code/naegleria/mandelbrot-llvm
  Time (mean ± σ):     18.266 s ±  0.004 s    [User: 18.204 s, System: 0.060 s]
  Range (min … max):   18.258 s … 18.273 s    10 runs

Benchmark 13: ~/code/naegleria/mandelbrot-llvm-opt
  Time (mean ± σ):     840.0 ms ±   0.6 ms    [User: 834.0 ms, System: 4.7 ms]
  Range (min … max):   839.0 ms … 840.8 ms    10 runs

Benchmark 14: ~/code/naegleria/mandelbrot-llvm-opt3
  Time (mean ± σ):     840.2 ms ±   0.2 ms    [User: 834.1 ms, System: 4.8 ms]
  Range (min … max):   840.0 ms … 840.6 ms    10 runs

Benchmark 15: php ~/code/naegleria/mandelbrot.php
  Time (mean ± σ):     42.717 s ±  0.055 s    [User: 42.554 s, System: 0.155 s]
  Range (min … max):   42.635 s … 42.810 s    10 runs

Benchmark 16: php -d opcache.enable_cli=On -d opcache.jit=tracing ~/code/naegleria/mandelbrot.php
  Time (mean ± σ):     17.486 s ±  0.107 s    [User: 17.387 s, System: 0.093 s]
  Range (min … max):   17.379 s … 17.629 s    10 runs

Benchmark 17: wasmtime ~/code/naegleria/mandelbrot.wasm
  Time (mean ± σ):     740.9 ms ±   0.5 ms    [User: 733.9 ms, System: 6.8 ms]
  Range (min … max):   740.4 ms … 742.0 ms    10 runs

Benchmark 18: iwasm ~/code/naegleria/mandelbrot.wasm
  Time (mean ± σ):     24.230 s ±  0.054 s    [User: 24.139 s, System: 0.088 s]
  Range (min … max):   24.161 s … 24.304 s    10 runs

Benchmark 19: wasmer ~/code/naegleria/mandelbrot.wasm
  Time (mean ± σ):     798.1 ms ±   0.9 ms    [User: 761.3 ms, System: 36.1 ms]
  Range (min … max):   797.0 ms … 799.6 ms    10 runs

Benchmark 20: wasmedge --enable-jit ~/code/naegleria/mandelbrot.wasm
  Time (mean ± σ):      1.513 s ±  0.007 s    [User: 1.492 s, System: 0.018 s]
  Range (min … max):    1.503 s …  1.528 s    10 runs

Benchmark 21: wazero run ~/code/naegleria/mandelbrot.wasm
  Time (mean ± σ):      3.525 s ±  0.003 s    [User: 3.510 s, System: 0.021 s]
  Range (min … max):    3.519 s …  3.528 s    10 runs

Summary
  ./bf --unsafe examples/mandelbrot.b ran
    1.02 ± 0.00 times faster than ./bf_amd64_darwin --unsafe examples/mandelbrot.b
    1.16 ± 0.00 times faster than ~/code/BF-JIT/target/release/fucker examples/mandelbrot.b
    1.19 ± 0.00 times faster than ~/code/BF-JIT/target/x86_64-apple-darwin/release/fucker examples/mandelbrot.b
    1.22 ± 0.00 times faster than ./bf examples/mandelbrot.b
    1.34 ± 0.00 times faster than ./bf_amd64_darwin examples/mandelbrot.b
    1.67 ± 0.00 times faster than ~/code/eliben-code-for-blog/2017/bfjit/optasmjit examples/mandelbrot.b
    1.79 ± 0.00 times faster than wasmtime ~/code/naegleria/mandelbrot.wasm
    1.93 ± 0.00 times faster than wasmer ~/code/naegleria/mandelbrot.wasm
    2.03 ± 0.00 times faster than ~/code/naegleria/mandelbrot-llvm-opt
    2.03 ± 0.00 times faster than ~/code/naegleria/mandelbrot-llvm-opt3
    3.66 ± 0.02 times faster than wasmedge --enable-jit ~/code/naegleria/mandelbrot.wasm
    6.80 ± 0.02 times faster than ~/code/rodrigodd-bf-compiler/target/x86_64-apple-darwin/debug/bf-cranelift-jit examples/mandelbrot.b
    7.67 ± 0.01 times faster than ~/code/eliben-code-for-blog/2017/bfjit/optinterp3 examples/mandelbrot.b
    8.53 ± 0.01 times faster than wazero run ~/code/naegleria/mandelbrot.wasm
   15.49 ± 0.01 times faster than ~/code/eliben-code-for-blog/2017/bfjit/optinterp2 examples/mandelbrot.b
   29.22 ± 0.04 times faster than ~/code/eliben-code-for-blog/2017/bfjit/optinterp examples/mandelbrot.b
   42.34 ± 0.26 times faster than php -d opcache.enable_cli=On -d opcache.jit=tracing ~/code/naegleria/mandelbrot.php
   44.23 ± 0.04 times faster than ~/code/naegleria/mandelbrot-llvm
   58.66 ± 0.14 times faster than iwasm ~/code/naegleria/mandelbrot.wasm
  103.42 ± 0.16 times faster than php ~/code/naegleria/mandelbrot.php
```

## License

MIT
