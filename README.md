# DynASM Brainfuck JIT Compiler

A high-performance Brainfuck interpreter with Just-In-Time compilation using DynASM for both ARM64 and x64 architectures.

## Features

- **JIT Compilation**: Compiles Brainfuck source to native machine code at runtime
- **AST Optimizations**: Advanced optimizations including multiplication loops, offset operations, and constant propagation
- **Multi-Architecture**: Supports both ARM64 and x64 architectures
- **Profiling Support**: Built-in profiler with flame graph compatibility and PC-to-AST mapping
- **Debug Mode**: Dumps AST and compiled machine code for analysis
- **High Performance**: Direct native code execution with minimal overhead
- **Modern Build System**: Bazel build system with multi-platform and sanitizer support

## Building

### Prerequisites

- GCC or Clang
- Bazel (build system)
- Bison (parser generator)
- Flex (lexical analyzer generator)

Note: LuaJIT is built from source by the build system

### Build Commands

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
$ hyperfine --warmup 1 './bf examples/mandelbrot.b' './bf_amd64_darwin examples/mandelbrot.b' '~/code/BF-JIT/target/debug/fucker examples/mandelbrot.b' '~/code/naegleria/mandelbrot-llvm' '~/code/naegleria/mandelbrot-llvm-opt' '~/code/naegleria/mandelbrot-llvm-opt3' 'php ~/code/naegleria/mandelbrot.php' 'php -d opcache.enable_cli=On -d opcache.jit=tracing ~/code/naegleria/mandelbrot.php' 'wasmtime ~/code/naegleria/mandelbrot.wasm' 'iwasm ~/code/naegleria/mandelbrot.wasm' 'wasmer ~/code/naegleria/mandelbrot.wasm' 'wasmedge --enable-jit ~/code/naegleria/mandelbrot.wasm' 'wazero run ~/code/naegleria/mandelbrot.wasm'

Benchmark 1: ./bf examples/mandelbrot.b
  Time (mean ± σ):     467.3 ms ±   1.4 ms    [User: 463.7 ms, System: 2.3 ms]
  Range (min … max):   466.0 ms … 470.5 ms    10 runs

Benchmark 2: ./bf_amd64_darwin examples/mandelbrot.b
  Time (mean ± σ):      1.251 s ±  0.008 s    [User: 1.239 s, System: 0.008 s]
  Range (min … max):    1.243 s …  1.266 s    10 runs

Benchmark 3: ~/code/BF-JIT/target/debug/fucker examples/mandelbrot.b
  Time (mean ± σ):     518.2 ms ±  14.3 ms    [User: 510.1 ms, System: 3.9 ms]
  Range (min … max):   509.4 ms … 558.3 ms    10 runs

Benchmark 4: ~/code/naegleria/mandelbrot-llvm
  Time (mean ± σ):     18.904 s ±  0.116 s    [User: 18.827 s, System: 0.051 s]
  Range (min … max):   18.679 s … 19.029 s    10 runs

Benchmark 5: ~/code/naegleria/mandelbrot-llvm-opt
  Time (mean ± σ):     869.5 ms ±   1.6 ms    [User: 863.9 ms, System: 3.9 ms]
  Range (min … max):   867.8 ms … 873.1 ms    10 runs

Benchmark 6: ~/code/naegleria/mandelbrot-llvm-opt3
  Time (mean ± σ):     889.7 ms ±  34.7 ms    [User: 869.6 ms, System: 5.0 ms]
  Range (min … max):   868.2 ms … 964.4 ms    10 runs

Benchmark 7: php ~/code/naegleria/mandelbrot.php
  Time (mean ± σ):     45.349 s ±  0.703 s    [User: 44.922 s, System: 0.147 s]
  Range (min … max):   44.649 s … 46.508 s    10 runs

Benchmark 8: php -d opcache.enable_cli=On -d opcache.jit=tracing ~/code/naegleria/mandelbrot.php
  Time (mean ± σ):     17.727 s ±  0.133 s    [User: 17.556 s, System: 0.088 s]
  Range (min … max):   17.580 s … 17.979 s    10 runs

Benchmark 9: wasmtime ~/code/naegleria/mandelbrot.wasm
  Time (mean ± σ):     749.5 ms ±   3.6 ms    [User: 740.4 ms, System: 7.0 ms]
  Range (min … max):   744.9 ms … 756.2 ms    10 runs

Benchmark 10: iwasm ~/code/naegleria/mandelbrot.wasm
  Time (mean ± σ):     24.601 s ±  0.044 s    [User: 24.431 s, System: 0.076 s]
  Range (min … max):   24.539 s … 24.691 s    10 runs

Benchmark 11: wasmer ~/code/naegleria/mandelbrot.wasm
  Time (mean ± σ):     825.0 ms ±   4.0 ms    [User: 781.1 ms, System: 39.7 ms]
  Range (min … max):   819.3 ms … 830.3 ms    10 runs

Benchmark 12: wasmedge --enable-jit ~/code/naegleria/mandelbrot.wasm
  Time (mean ± σ):      1.561 s ±  0.025 s    [User: 1.532 s, System: 0.020 s]
  Range (min … max):    1.538 s …  1.616 s    10 runs

Benchmark 13: wazero run ~/code/naegleria/mandelbrot.wasm
  Time (mean ± σ):      3.617 s ±  0.013 s    [User: 3.588 s, System: 0.021 s]
  Range (min … max):    3.599 s …  3.636 s    10 runs

Summary
  ./bf examples/mandelbrot.b ran
    1.11 ± 0.03 times faster than ~/code/BF-JIT/target/debug/fucker examples/mandelbrot.b
    1.60 ± 0.01 times faster than wasmtime ~/code/naegleria/mandelbrot.wasm
    1.77 ± 0.01 times faster than wasmer ~/code/naegleria/mandelbrot.wasm
    1.86 ± 0.01 times faster than ~/code/naegleria/mandelbrot-llvm-opt
    1.90 ± 0.07 times faster than ~/code/naegleria/mandelbrot-llvm-opt3
    2.68 ± 0.02 times faster than ./bf_amd64_darwin examples/mandelbrot.b
    3.34 ± 0.05 times faster than wasmedge --enable-jit ~/code/naegleria/mandelbrot.wasm
    7.74 ± 0.04 times faster than wazero run ~/code/naegleria/mandelbrot.wasm
   37.93 ± 0.30 times faster than php -d opcache.enable_cli=On -d opcache.jit=tracing ~/code/naegleria/mandelbrot.php
   40.45 ± 0.27 times faster than ~/code/naegleria/mandelbrot-llvm
   52.65 ± 0.18 times faster than iwasm ~/code/naegleria/mandelbrot.wasm
   97.04 ± 1.53 times faster than php ~/code/naegleria/mandelbrot.php
```

## License

MIT
