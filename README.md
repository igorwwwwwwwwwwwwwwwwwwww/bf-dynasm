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

- [BF-JIT](https://github.com/danthedaniel/BF-JIT)
- [naegleria](https://github.com/igorwwwwwwwwwwwwwwwwwwww/naegleria) (llvm, php, wasm)
  - LLVM was compiled with clang 17.0.0
  - PHP was run against 8.4.12, with and without JIT
  - wasm was run against these runtimes
    - [wasmtime](https://github.com/bytecodealliance/wasmtime)
    - [iwasm](https://github.com/bytecodealliance/wasm-micro-runtime)
    - [wasmer](https://github.com/wasmerio/wasmer)
    - [WasmEdge](https://github.com/WasmEdge/WasmEdge)
    - [Wazero](https://github.com/tetratelabs/wazero)
- [brainfuck-php](https://github.com/igorw/brainfuck-php) was skipped
  - baseline ~50 minutes. eliminating NullLogger calls ~36 minutes. optimizing jumps ~13 minutes. JIT ~10 minutes.

Results:

```
$ hyperfine --warmup 3 './bf examples/mandelbrot.b' './bf --unsafe examples/mandelbrot.b' './bf_amd64_darwin examples/mandelbrot.b' './bf_amd64_darwin --unsafe examples/mandelbrot.b' '~/code/BF-JIT/target/release/fucker examples/mandelbrot.b' '~/code/BF-JIT/target/x86_64-apple-darwin/release/fucker examples/mandelbrot.b' '~/code/naegleria/mandelbrot-llvm' '~/code/naegleria/mandelbrot-llvm-opt' '~/code/naegleria/mandelbrot-llvm-opt3' 'php ~/code/naegleria/mandelbrot.php' 'php -d opcache.enable_cli=On -d opcache.jit=tracing ~/code/naegleria/mandelbrot.php' 'wasmtime ~/code/naegleria/mandelbrot.wasm' 'iwasm ~/code/naegleria/mandelbrot.wasm' 'wasmer ~/code/naegleria/mandelbrot.wasm' 'wasmedge --enable-jit ~/code/naegleria/mandelbrot.wasm' 'wazero run ~/code/naegleria/mandelbrot.wasm'

Benchmark 1: ./bf examples/mandelbrot.b
  Time (mean ± σ):     559.3 ms ±   1.7 ms    [User: 555.4 ms, System: 3.1 ms]
  Range (min … max):   557.7 ms … 562.7 ms    10 runs

Benchmark 2: ./bf --unsafe examples/mandelbrot.b
  Time (mean ± σ):     458.4 ms ±   0.7 ms    [User: 455.6 ms, System: 2.1 ms]
  Range (min … max):   457.5 ms … 459.3 ms    10 runs

Benchmark 3: ./bf_amd64_darwin examples/mandelbrot.b
  Time (mean ± σ):     647.1 ms ±   0.7 ms    [User: 639.0 ms, System: 5.6 ms]
  Range (min … max):   646.4 ms … 649.1 ms    10 runs

Benchmark 4: ./bf_amd64_darwin --unsafe examples/mandelbrot.b
  Time (mean ± σ):     476.6 ms ±   0.5 ms    [User: 469.4 ms, System: 4.8 ms]
  Range (min … max):   475.7 ms … 477.3 ms    10 runs

Benchmark 5: ~/code/BF-JIT/target/release/fucker examples/mandelbrot.b
  Time (mean ± σ):     479.6 ms ±   0.7 ms    [User: 476.4 ms, System: 2.4 ms]
  Range (min … max):   478.7 ms … 481.3 ms    10 runs

Benchmark 6: ~/code/BF-JIT/target/x86_64-apple-darwin/release/fucker examples/mandelbrot.b
  Time (mean ± σ):     490.4 ms ±   0.4 ms    [User: 482.2 ms, System: 5.6 ms]
  Range (min … max):   489.5 ms … 491.0 ms    10 runs

Benchmark 7: ~/code/naegleria/mandelbrot-llvm
  Time (mean ± σ):     18.271 s ±  0.008 s    [User: 18.207 s, System: 0.061 s]
  Range (min … max):   18.266 s … 18.293 s    10 runs

Benchmark 8: ~/code/naegleria/mandelbrot-llvm-opt
  Time (mean ± σ):     840.0 ms ±   0.4 ms    [User: 834.1 ms, System: 4.7 ms]
  Range (min … max):   838.9 ms … 840.3 ms    10 runs

Benchmark 9: ~/code/naegleria/mandelbrot-llvm-opt3
  Time (mean ± σ):     840.1 ms ±   0.3 ms    [User: 834.1 ms, System: 4.8 ms]
  Range (min … max):   839.7 ms … 840.7 ms    10 runs

Benchmark 10: php ~/code/naegleria/mandelbrot.php
  Time (mean ± σ):     43.365 s ±  1.592 s    [User: 42.717 s, System: 0.171 s]
  Range (min … max):   42.610 s … 47.831 s    10 runs

Benchmark 11: php -d opcache.enable_cli=On -d opcache.jit=tracing ~/code/naegleria/mandelbrot.php
  Time (mean ± σ):     17.531 s ±  0.080 s    [User: 17.399 s, System: 0.097 s]
  Range (min … max):   17.431 s … 17.632 s    10 runs

Benchmark 12: wasmtime ~/code/naegleria/mandelbrot.wasm
  Time (mean ± σ):     744.1 ms ±   8.3 ms    [User: 734.8 ms, System: 6.6 ms]
  Range (min … max):   739.1 ms … 767.4 ms    10 runs

Benchmark 13: iwasm ~/code/naegleria/mandelbrot.wasm
  Time (mean ± σ):     24.195 s ±  0.043 s    [User: 24.077 s, System: 0.072 s]
  Range (min … max):   24.137 s … 24.267 s    10 runs

Benchmark 14: wasmer ~/code/naegleria/mandelbrot.wasm
  Time (mean ± σ):     802.5 ms ±   3.3 ms    [User: 761.4 ms, System: 37.3 ms]
  Range (min … max):   796.1 ms … 807.2 ms    10 runs

Benchmark 15: wasmedge --enable-jit ~/code/naegleria/mandelbrot.wasm
  Time (mean ± σ):      1.544 s ±  0.068 s    [User: 1.509 s, System: 0.019 s]
  Range (min … max):    1.497 s …  1.714 s    10 runs

Benchmark 16: wazero run ~/code/naegleria/mandelbrot.wasm
  Time (mean ± σ):      3.522 s ±  0.011 s    [User: 3.502 s, System: 0.020 s]
  Range (min … max):    3.508 s …  3.541 s    10 runs

Summary
  ./bf --unsafe examples/mandelbrot.b ran
    1.04 ± 0.00 times faster than ./bf_amd64_darwin --unsafe examples/mandelbrot.b
    1.05 ± 0.00 times faster than ~/code/BF-JIT/target/release/fucker examples/mandelbrot.b
    1.07 ± 0.00 times faster than ~/code/BF-JIT/target/x86_64-apple-darwin/release/fucker examples/mandelbrot.b
    1.22 ± 0.00 times faster than ./bf examples/mandelbrot.b
    1.41 ± 0.00 times faster than ./bf_amd64_darwin examples/mandelbrot.b
    1.62 ± 0.02 times faster than wasmtime ~/code/naegleria/mandelbrot.wasm
    1.75 ± 0.01 times faster than wasmer ~/code/naegleria/mandelbrot.wasm
    1.83 ± 0.00 times faster than ~/code/naegleria/mandelbrot-llvm-opt
    1.83 ± 0.00 times faster than ~/code/naegleria/mandelbrot-llvm-opt3
    3.37 ± 0.15 times faster than wasmedge --enable-jit ~/code/naegleria/mandelbrot.wasm
    7.68 ± 0.03 times faster than wazero run ~/code/naegleria/mandelbrot.wasm
   38.24 ± 0.18 times faster than php -d opcache.enable_cli=On -d opcache.jit=tracing ~/code/naegleria/mandelbrot.php
   39.86 ± 0.06 times faster than ~/code/naegleria/mandelbrot-llvm
   52.78 ± 0.12 times faster than iwasm ~/code/naegleria/mandelbrot.wasm
   94.60 ± 3.48 times faster than php ~/code/naegleria/mandelbrot.php
```

## License

MIT
