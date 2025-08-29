CC = gcc
CC_X64_MACOS = clang
CFLAGS = -Wall -Wextra -O2 -std=c99
CFLAGS_X64_MACOS = -target x86_64-apple-macos10.12 -Wall -Wextra -O2 -std=c99
LUAJIT = luajit
LUA_DOCKER = lua5.1

TARGET = bf
TARGET_AMD64_DARWIN = bf_amd64_darwin
DYNASM_DIR = luajit/dynasm
MAIN_C = bf.c
DYNASM_DASC_ARM64 = bf_arm64.dasc
DYNASM_DASC_AMD64 = bf_amd64.dasc
ARCH_C_ARM64 = bf_arm64.c
ARCH_C_AMD64 = bf_amd64.c

all: $(TARGET)

amd64-darwin: $(TARGET_AMD64_DARWIN)

$(DYNASM_DIR):
	@if [ ! -d "luajit" ]; then \
		git clone https://github.com/LuaJIT/LuaJIT.git luajit; \
	fi

$(ARCH_C_ARM64): $(DYNASM_DASC_ARM64) $(DYNASM_DIR)
	@if command -v $(LUAJIT) >/dev/null 2>&1; then \
		$(LUAJIT) $(DYNASM_DIR)/dynasm.lua -o $(ARCH_C_ARM64) $(DYNASM_DASC_ARM64); \
	else \
		$(LUA_DOCKER) $(DYNASM_DIR)/dynasm.lua -o $(ARCH_C_ARM64) $(DYNASM_DASC_ARM64); \
	fi

$(ARCH_C_AMD64): $(DYNASM_DASC_AMD64) $(DYNASM_DIR)
	@if command -v $(LUAJIT) >/dev/null 2>&1; then \
		$(LUAJIT) $(DYNASM_DIR)/dynasm.lua -o $(ARCH_C_AMD64) $(DYNASM_DASC_AMD64); \
	else \
		$(LUA_DOCKER) $(DYNASM_DIR)/dynasm.lua -o $(ARCH_C_AMD64) $(DYNASM_DASC_AMD64); \
	fi

# Build only the architecture file needed for current platform
ifeq ($(shell uname -m),x86_64)
$(TARGET): $(ARCH_C_AMD64) $(MAIN_C)
	$(CC) $(CFLAGS) -I$(DYNASM_DIR) -o $(TARGET) $(MAIN_C)
else ifeq ($(shell uname -m),aarch64)
$(TARGET): $(ARCH_C_ARM64) $(MAIN_C)
	$(CC) $(CFLAGS) -I$(DYNASM_DIR) -o $(TARGET) $(MAIN_C)
else
$(TARGET): $(ARCH_C_ARM64) $(MAIN_C)
	$(CC) $(CFLAGS) -I$(DYNASM_DIR) -o $(TARGET) $(MAIN_C)
endif

$(TARGET_AMD64_DARWIN): $(ARCH_C_AMD64) $(MAIN_C)
	$(CC_X64_MACOS) $(CFLAGS_X64_MACOS) -I$(DYNASM_DIR) -o $(TARGET_AMD64_DARWIN) $(MAIN_C)

clean:
	rm -f $(TARGET) $(TARGET_AMD64_DARWIN) $(ARCH_C_ARM64) $(ARCH_C_AMD64)

test: $(TARGET)
	@echo "Testing native version..."
	./$(TARGET) examples/hello.bf

test-amd64-darwin: $(TARGET_AMD64_DARWIN)
	@echo "Testing AMD64 Darwin version (via Rosetta)..."
	arch -x86_64 ./$(TARGET_AMD64_DARWIN) examples/hello.bf



.PHONY: all clean test test-amd64-darwin amd64-darwin