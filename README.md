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

### Build Commands

```bash
# Build ARM64 version (default)
make arm64

# Build x64 version
make x64

# Build both versions
make both

# Clean build files
make clean
```

## Usage

```bash
# Run Brainfuck program
./bf_interpreter examples/hello.bf

# Run with debug mode (dumps machine code)
./bf_interpreter -d examples/hello.bf

# x64 version (if available)
./bf_interpreter_x64 examples/hello.bf
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

### Local Testing (ARM64)
```bash
# Test ARM64 version
make test

# Test with debug output
make test-debug
```

## Docker Multi-Platform Support

The project includes a multi-platform Dockerfile that automatically detects the target architecture and builds the appropriate version.

### Quick Start with Docker

```bash
# Build and run for your current platform
docker build -t dynasm-bf .
docker run --rm dynasm-bf

# Expected output: "Hello World!"
```

### Multi-Platform Docker Builds

```bash
# Build specifically for x64 (AMD64)
docker build --platform=linux/amd64 -t dynasm-bf:x64 .
docker run --rm --platform=linux/amd64 dynasm-bf:x64

# Build specifically for ARM64
docker build --platform=linux/arm64 -t dynasm-bf:arm64 .
docker run --rm --platform=linux/arm64 dynasm-bf:arm64

# Build both platforms (requires Docker Buildx)
docker buildx build --platform=linux/amd64,linux/arm64 -t dynasm-bf:multi .
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
- Downloads and builds LuaJIT from source for reliability
- Compiles the appropriate version with correct flags:
  - **x64**: Uses `-DX64_BUILD` and `-no-pie` flags
  - **ARM64**: Uses standard compilation
- Tests the built interpreter with the Hello World example

### Cross-Platform Testing

```bash
# Test x64 version (with emulation if on ARM64 host)
docker build --platform=linux/amd64 -t dynasm-bf:x64 .
docker run --platform=linux/amd64 dynasm-bf:x64

# Test ARM64 version (with emulation if on x64 host)  
docker build --platform=linux/arm64 -t dynasm-bf:arm64 .
docker run --platform=linux/arm64 dynasm-bf:arm64

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
- Conditional compilation for different architectures
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

## File Structure

```
dynasm-brainfuck/
├── bf_interpreter.dasc     # DynASM source template
├── bf_interpreter.c        # Generated ARM64 C code
├── bf_interpreter_x64.c    # Generated x64 C code  
├── bf_interpreter          # ARM64 executable
├── bf_interpreter_x64      # x64 executable
├── Makefile               # Multi-architecture build configuration
├── Dockerfile             # Multi-platform Docker build
├── .dockerignore          # Docker build context exclusions
├── examples/              # Brainfuck test programs
│   └── hello.bf           # Hello World example
├── luajit/               # LuaJIT source (git submodule/auto-cloned)
│   ├── src/luajit        # LuaJIT executable
│   └── dynasm/           # DynASM preprocessor
└── README.md             # This documentation
```

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
make arm64  # This will clone LuaJIT automatically
```

**DynASM preprocessing errors:**
```bash
# Clean and rebuild
make clean
make arm64
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Test on both ARM64 and x64 (using Docker)
4. Submit a pull request

## License

MIT License - see LICENSE file for details