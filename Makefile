CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
LUAJIT = luajit

TARGET_ARM64 = bf_interpreter
TARGET_X64 = bf_interpreter_x64
DYNASM_DIR = luajit/dynasm
DYNASM_DASC = bf_interpreter.dasc
GENERATED_C_ARM64 = bf_interpreter.c
GENERATED_C_X64 = bf_interpreter_x64.c

all: $(TARGET_ARM64)

arm64: $(TARGET_ARM64)
x64: $(TARGET_X64)
both: $(TARGET_ARM64) $(TARGET_X64)

$(DYNASM_DIR):
	@if [ ! -d "luajit" ]; then \
		git clone https://github.com/LuaJIT/LuaJIT.git luajit; \
	fi

$(GENERATED_C_ARM64): $(DYNASM_DASC) $(DYNASM_DIR)
	$(LUAJIT) $(DYNASM_DIR)/dynasm.lua -o $(GENERATED_C_ARM64) $(DYNASM_DASC)

$(GENERATED_C_X64): $(DYNASM_DASC) $(DYNASM_DIR)
	$(LUAJIT) $(DYNASM_DIR)/dynasm.lua -D X64_BUILD -o $(GENERATED_C_X64) $(DYNASM_DASC)

$(TARGET_ARM64): $(GENERATED_C_ARM64)
	$(CC) $(CFLAGS) -I$(DYNASM_DIR) -o $(TARGET_ARM64) $(GENERATED_C_ARM64)

$(TARGET_X64): $(GENERATED_C_X64)
	$(CC) $(CFLAGS) -I$(DYNASM_DIR) -DX64_BUILD -no-pie -o $(TARGET_X64) $(GENERATED_C_X64)

clean:
	rm -f $(TARGET_ARM64) $(TARGET_X64) $(GENERATED_C_ARM64) $(GENERATED_C_X64)

test: $(TARGET_ARM64)
	@echo "Testing ARM64 version..."
	./$(TARGET_ARM64) examples/hello.bf

test-x64: $(TARGET_X64)
	@echo "Testing x64 version..."
	./$(TARGET_X64) examples/hello.bf

test-both: test test-x64

test-debug: $(TARGET_ARM64)
	@echo "Testing with debug mode..."
	./$(TARGET_ARM64) -d examples/hello.bf

.PHONY: all clean test test-x64 test-both test-debug arm64 x64 both