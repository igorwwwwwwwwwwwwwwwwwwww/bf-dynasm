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
- 30,000 byte memory array (standard Brainfuck memory size)
- Zero-initialized memory
- Bounds checking through runtime error handling

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

## License

MIT
