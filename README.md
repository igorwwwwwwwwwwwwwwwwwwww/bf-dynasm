# DynASM Brainfuck JIT Compiler

A high-performance Brainfuck interpreter with Just-In-Time compilation using DynASM for both ARM64 and x64 architectures.

## Features

- **JIT Compilation**: Compiles Brainfuck source to native machine code at runtime
- **Multi-Architecture**: Supports both ARM64 and x64 architectures  
- **Multi-Platform Docker**: Automatic architecture detection and cross-platform builds
- **Debug Mode**: Dumps compiled machine code in hex format for analysis
- **High Performance**: Direct native code execution with optimized calling conventions
- **Cross-Platform**: Works on macOS (ARM64/Intel) and Linux (ARM64/x64)

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

# Clean build files
make clean
```

## Usage

```bash
# Run Brainfuck program (native architecture)
./bf examples/hello.bf

# Run with debug mode (dumps machine code)
./bf -d examples/hello.bf

# AMD64 version (via Rosetta on ARM64 Macs)
arch -x86_64 ./bf_amd64_darwin examples/hello.bf
```

## Architecture Support

### ARM64 (Apple Silicon, ARM64 Linux)
- Native execution on ARM64 systems
- Optimized ARM64 assembly with proper calling conventions
- Uses ARM64 registers: x19 for memory pointer, standard AAPCS64 ABI
- Function calls use register-indirect calls with 4-instruction 64-bit address loading:
  ```asm
  mov  x16, #immediate_low_16_bits
  movk x16, #immediate_next_16_bits, lsl #16
  movk x16, #immediate_next_16_bits, lsl #32  
  movk x16, #immediate_high_16_bits, lsl #48
  blr  x16
  ```

### x64 (Intel/AMD 64-bit)
- Cross-compiled x64 machine code generation
- System V AMD64 ABI compliance
- Uses register-indirect calls with 64-bit immediate loading:
  ```asm
  mov rax, function_address    ; 64-bit immediate
  call rax                     ; Register-indirect call
  ```
- Memory pointer stored on stack for proper alignment

## Testing

### Local Testing
```bash
# Test native version
make test

# Test AMD64 Darwin version (via Rosetta)
make test-amd64-darwin
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
docker run --rm dynasm-bf sh -c 'echo "++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++." > test.bf && ./bf_interpreter test.bf'

# Run with debug mode to see compiled machine code
docker run --rm dynasm-bf ./bf_interpreter -d examples/hello.bf
```

### Docker Architecture Details

The Dockerfile automatically:
- Detects the target architecture (`x86_64` or `aarch64`)
- Uses system LuaJIT package for reliable builds
- Compiles with automatic architecture detection via C preprocessor macros
- Tests the built interpreter with the Hello World example

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

## Examples

The `examples/` directory contains various Brainfuck programs:

- `hello.bf` - Hello World program
- `hello2.bf` - Alternative Hello World with nested loops  
- `hello3.bf` - Complex Hello World variant
- `count.bf` - Simple counting program
- `simple_loop.bf` - Basic loop test

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
- Hex dump of compiled machine code
- Architecture identification
- Memory layout analysis


## Performance

This JIT compiler provides significant performance improvements over traditional Brainfuck interpreters:

- **Native Speed**: Compiled code runs at native processor speed
- **No Interpretation Overhead**: Direct machine code execution
- **Optimized Loops**: Native conditional branches instead of interpretation
- **Efficient I/O**: Direct function calls with proper ABI compliance

## Troubleshooting

### Docker Issues

**Building for different architecture fails:**
```bash
# Make sure Docker Buildx is enabled
docker buildx create --use

# Check available platforms
docker buildx ls
```

**"cannot create state: not enough memory" error:**
- This was an issue with system LuaJIT packages
- The Dockerfile now builds LuaJIT from source to avoid this

**ARM64 segmentation fault in Docker:**
- Fixed in current version using proper register-indirect calls
- If you see this, make sure you're using the latest Dockerfile

### Build Issues

**"luajit: command not found":**
```bash
# The Makefile will auto-download LuaJIT
make  # This will clone LuaJIT automatically
```

**DynASM preprocessing errors:**
```bash
# Clean and rebuild
make clean
make
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Test on both ARM64 and x64 (using Docker)
4. Submit a pull request

## License

MIT License - see LICENSE file for details