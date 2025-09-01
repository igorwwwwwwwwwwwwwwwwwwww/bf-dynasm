CC = gcc
CC_X64_MACOS = clang
CFLAGS = -Wall -Wextra -O2 -g -std=c99
CFLAGS_X64_MACOS = -target x86_64-apple-macos10.12 -Wall -Wextra -O2 -g -std=c99
CFLAGS_X64_ASAN = -target x86_64-apple-macos10.12 -Wall -Wextra -O2 -g -fsanitize=address -std=c99
CFLAGS_ASAN = -Wall -Wextra -O2 -g -fsanitize=address -std=c99
LUAJIT = luajit
LUA_DOCKER = lua5.1

# Parser tools - use newer versions when available (Homebrew on macOS, system on Linux)
BREW_BISON = $(shell command -v brew >/dev/null 2>&1 && echo "$(shell brew --prefix bison)/bin/bison" || echo "bison")
BREW_FLEX = $(shell command -v brew >/dev/null 2>&1 && echo "$(shell brew --prefix flex)/bin/flex" || echo "flex")
LEX = $(BREW_FLEX)
YACC = $(BREW_BISON)

TARGET = bf
TARGET_AMD64_DARWIN = bf_amd64_darwin
TARGET_AMD64_DARWIN_ASAN = bf_amd64_darwin_asan
TARGET_ASAN = bf_asan
DYNASM_DIR = luajit/dynasm
MAIN_C = bf.c
DYNASM_DASC_ARM64 = bf_arm64.dasc
DYNASM_DASC_AMD64 = bf_amd64.dasc
ARCH_C_ARM64 = bf_arm64.c
ARCH_C_AMD64 = bf_amd64.c

# Parser files
PARSER_Y = bf_parser.y
PARSER_C = bf_parser.c
PARSER_H = bf_parser.h
LEXER_L = bf_lexer.l
LEXER_C = bf_lexer.c
LEXER_H = bf_lexer.h
AST_C = bf_ast.c
AST_H = bf_ast.h
PROF_C = bf_prof.c
PROF_H = bf_prof.h
DEBUG_C = bf_debug.c
DEBUG_H = bf_debug.h

all: $(TARGET)
asan: $(TARGET_ASAN)
amd64-darwin: $(TARGET_AMD64_DARWIN)
amd64-darwin-asan: $(TARGET_AMD64_DARWIN_ASAN)

$(DYNASM_DIR):
	@if [ ! -d "luajit" ]; then \
		git clone https://github.com/LuaJIT/LuaJIT.git luajit; \
	fi

# Parser generation
$(PARSER_C) $(PARSER_H): $(PARSER_Y)
	$(YACC) -d -o $(PARSER_C) $(PARSER_Y)

$(LEXER_C) $(LEXER_H): $(LEXER_L) $(PARSER_H)
	$(LEX) -o $(LEXER_C) $(LEXER_L)

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
$(TARGET): $(ARCH_C_AMD64) $(MAIN_C) $(PARSER_C) $(LEXER_C) $(AST_C) $(PROF_C) $(DEBUG_C)
	$(CC) $(CFLAGS) -I$(DYNASM_DIR) -o $(TARGET) $(MAIN_C) $(PARSER_C) $(LEXER_C) $(AST_C) $(PROF_C) $(DEBUG_C)

$(TARGET_ASAN): $(ARCH_C_AMD64) $(MAIN_C) $(PARSER_C) $(LEXER_C) $(AST_C) $(PROF_C) $(DEBUG_C)
	$(CC) $(CFLAGS_ASAN) -I$(DYNASM_DIR) -o $(TARGET_ASAN) $(MAIN_C) $(PARSER_C) $(LEXER_C) $(AST_C) $(PROF_C) $(DEBUG_C)
else
$(TARGET): $(ARCH_C_ARM64) $(MAIN_C) $(PARSER_C) $(LEXER_C) $(AST_C) $(PROF_C) $(DEBUG_C)
	$(CC) $(CFLAGS) -I$(DYNASM_DIR) -o $(TARGET) $(MAIN_C) $(PARSER_C) $(LEXER_C) $(AST_C) $(PROF_C) $(DEBUG_C)

$(TARGET_ASAN): $(ARCH_C_ARM64) $(MAIN_C) $(PARSER_C) $(LEXER_C) $(AST_C) $(PROF_C) $(DEBUG_C)
	$(CC) $(CFLAGS_ASAN) -I$(DYNASM_DIR) -o $(TARGET_ASAN) $(MAIN_C) $(PARSER_C) $(LEXER_C) $(AST_C) $(PROF_C) $(DEBUG_C)
endif

$(TARGET_AMD64_DARWIN): $(ARCH_C_AMD64) $(MAIN_C) $(PARSER_C) $(LEXER_C) $(AST_C) $(PROF_C) $(DEBUG_C)
	$(CC_X64_MACOS) $(CFLAGS_X64_MACOS) -I$(DYNASM_DIR) -o $(TARGET_AMD64_DARWIN) $(MAIN_C) $(PARSER_C) $(LEXER_C) $(AST_C) $(PROF_C) $(DEBUG_C)

$(TARGET_AMD64_DARWIN_ASAN): $(ARCH_C_AMD64) $(MAIN_C) $(PARSER_C) $(LEXER_C) $(AST_C) $(PROF_C) $(DEBUG_C)
	$(CC_X64_MACOS) $(CFLAGS_X64_ASAN) -I$(DYNASM_DIR) -o $(TARGET_AMD64_DARWIN_ASAN) $(MAIN_C) $(PARSER_C) $(LEXER_C) $(AST_C) $(PROF_C) $(DEBUG_C)

clean:
	rm -f $(TARGET) $(TARGET_AMD64_DARWIN) $(ARCH_C_ARM64) $(ARCH_C_AMD64) $(PARSER_C) $(PARSER_H) $(LEXER_C)

.PHONY: all clean amd64-darwin amd64-darwin-asan asan
