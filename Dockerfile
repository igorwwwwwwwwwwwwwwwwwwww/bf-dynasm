FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    build-essential \
    file \
    make \
    luajit \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

# Build using simplified make system (automatic architecture detection)
RUN make

# Test with Hello World
RUN ./bf examples/hello.b

CMD ["./bf", "examples/hello.b"]