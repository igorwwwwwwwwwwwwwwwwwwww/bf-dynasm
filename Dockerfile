# Build stage
FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y \
    build-essential \
    curl \
    unzip \
    python3 \
    bison \
    flex \
    && rm -rf /var/lib/apt/lists/*

# Install Bazelisk (which downloads the right Bazel version)
RUN ARCH=$(dpkg --print-architecture) && \
    if [ "$ARCH" = "amd64" ]; then BAZEL_ARCH="amd64"; else BAZEL_ARCH="arm64"; fi && \
    curl -fsSLO "https://github.com/bazelbuild/bazelisk/releases/latest/download/bazelisk-linux-${BAZEL_ARCH}" && \
    chmod +x bazelisk-linux-${BAZEL_ARCH} && \
    mv bazelisk-linux-${BAZEL_ARCH} /usr/local/bin/bazel

WORKDIR /app
COPY . .

# Build using Bazel
RUN bazel build //:bf

# Runtime stage - use Ubuntu minimal for glibc compatibility
FROM ubuntu:24.04

# Create working directory and copy files
WORKDIR /app
COPY --from=builder /app/bazel-bin/bf /app/bf
COPY --from=builder /app/examples /app/examples

ENTRYPOINT ["./bf"]
CMD ["examples/hello.b"]
