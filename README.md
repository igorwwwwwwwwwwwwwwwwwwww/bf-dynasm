# DynASM Brainfuck JIT Compiler

A high-performance Brainfuck interpreter with Just-In-Time compilation using DynASM for both ARM64 and x64 architectures.

## Features

- **JIT Compilation**: Compiles Brainfuck source to native machine code at runtime
- **AST Optimizations**: Advanced optimizations including multiplication loops, offset operations, and constant propagation
- **Multi-Architecture**: Supports both ARM64 and x64 architectures
- **Debug Mode**: Dumps AST and compiled machine code for analysis
- **High Performance**: Direct native code execution with minimal overhead

## Building

### Prerequisites

- GCC or Clang
- LuaJIT (for DynASM preprocessing)
- Make
- Bison (parser generator)
- Flex (lexical analyzer generator)

### Build Commands

```bash
# Install dependencies on macOS
brew install luajit flex bison

# Build native version (default - ARM64 on ARM64, AMD64 on AMD64)
make

# Build AMD64 macOS cross-compiled version  
make amd64-darwin

# Build AddressSanitizer debug versions
make asan                    # Native ASan version
make amd64-darwin-asan      # AMD64 ASan version (via Rosetta)

# Clean build files
make clean
```

## Usage

```bash
# Run Brainfuck program
./bf examples/hello.b

# Run with debug mode (dumps AST and machine code)
./bf --debug examples/fizzbuzz.b

# Run without optimizations
./bf --no-optimize examples/hello.b

# Show help
./bf --help
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
./bf_amd64_darwin examples/hello.b
```

## Testing

### Local Testing
```bash
# Test native version
./bf examples/hello.b

# Test AMD64 Darwin version (via Rosetta)
./bf_amd64_darwin examples/hello.b

# Test AddressSanitizer versions for debugging
./bf_asan examples/hello.b                    # Native ASan
./bf_amd64_darwin_asan examples/hello.b      # AMD64 ASan (via Rosetta)
```

## Docker Multi-Platform Support

The project includes a multi-platform Dockerfile that automatically detects the target architecture and builds the appropriate version.

### Quick Start with Docker

```bash
# Build multi-platform image (works on both x64 and ARM64)
docker buildx build --platform=linux/amd64,linux/arm64 -t dynasm-bf .

# Run on any platform (automatically selects correct architecture)
docker run --rm dynasm-bf

# Expected output: "Hello World!"
```

### Testing Different Brainfuck Programs

```bash
# Test with custom Brainfuck code
docker run --rm dynasm-bf sh -c 'echo "++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++." > test.b && ./bf test.b'

# Run with debug mode to see compiled machine code
docker run --rm dynasm-bf ./bf --debug examples/hello.b
```

### Cross-Platform Testing

```bash
# Build once, test on both architectures
docker buildx build --platform=linux/amd64,linux/arm64 -t dynasm-bf .

# Test x64 version (with emulation if on ARM64 host)
docker run --platform=linux/amd64 dynasm-bf

# Test ARM64 version (with emulation if on x64 host)
docker run --platform=linux/arm64 dynasm-bf

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


## Performance

This JIT compiler provides significant performance improvements over traditional Brainfuck interpreters:

- **Native Speed**: Compiled code runs at native processor speed
- **No Interpretation Overhead**: Direct machine code execution
- **AST Optimizations**: Advanced optimizations reduce instruction count significantly
- **Optimized Loops**: Native conditional branches with specialized loop patterns
- **Efficient I/O**: Direct function calls with proper ABI compliance

## License

MIT
