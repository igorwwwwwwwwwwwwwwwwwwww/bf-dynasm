FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    file \
    make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

# Build LuaJIT locally first 
RUN cd luajit && make clean && make

# Build appropriate version based on architecture using local luajit
RUN case $(uname -m) in \
    x86_64) \
        echo "Building for x64..." && \
        ./luajit/src/luajit luajit/dynasm/dynasm.lua -D X64_BUILD -o bf_interpreter_x64.c bf_interpreter.dasc && \
        gcc -Wall -Wextra -O2 -std=c99 -Iluajit/dynasm -DX64_BUILD -no-pie -o bf_interpreter_x64 bf_interpreter_x64.c && \
        file bf_interpreter_x64 && \
        mv bf_interpreter_x64 bf_interpreter \
        ;; \
    aarch64) \
        echo "Building for ARM64..." && \
        ./luajit/src/luajit luajit/dynasm/dynasm.lua -o bf_interpreter.c bf_interpreter.dasc && \
        gcc -Wall -Wextra -O2 -std=c99 -Iluajit/dynasm -o bf_interpreter bf_interpreter.c && \
        file bf_interpreter \
        ;; \
    *) \
        echo "Unsupported architecture: $(uname -m)" && \
        exit 1 \
        ;; \
    esac

# Test the interpreter
CMD ["./bf_interpreter", "examples/hello.bf"]