#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEPS_DIR="${SCRIPT_DIR}/deps"
PICO_SDK_DIR="${DEPS_DIR}/pico-sdk"
PICO_SDK_REF="${PICO_SDK_REF:-master}"
PICO_SDK_TOOLS_TAG="${PICO_SDK_TOOLS_TAG:-v2.0.0-5}"
RISCV_TOOLCHAIN_DIR="${DEPS_DIR}/riscv-toolchain"

host_arch="$(uname -m)"
case "${host_arch}" in
  arm64|aarch64)
    toolchain_archive="riscv-toolchain-14-arm64-mac.zip"
    ;;
  x86_64)
    toolchain_archive="riscv-toolchain-14-x64-mac.zip"
    ;;
  *)
    echo "Unsupported host architecture: ${host_arch}" >&2
    exit 1
    ;;
esac

RISCV_TOOLCHAIN_URL="https://github.com/raspberrypi/pico-sdk-tools/releases/download/${PICO_SDK_TOOLS_TAG}/${toolchain_archive}"
RISCV_TOOLCHAIN_GCC="${RISCV_TOOLCHAIN_DIR}/bin/riscv32-unknown-elf-gcc"

mkdir -p "${DEPS_DIR}"

if [[ -d "${PICO_SDK_DIR}/.git" ]]; then
  echo "Updating pico-sdk in ${PICO_SDK_DIR}"
  git -C "${PICO_SDK_DIR}" fetch --depth=1 origin "${PICO_SDK_REF}"
  git -C "${PICO_SDK_DIR}" checkout -q FETCH_HEAD
else
  echo "Cloning pico-sdk into ${PICO_SDK_DIR}"
  git clone --depth=1 --branch "${PICO_SDK_REF}" https://github.com/raspberrypi/pico-sdk.git "${PICO_SDK_DIR}"
fi

echo "Initializing pico-sdk submodules"
git -C "${PICO_SDK_DIR}" submodule update --init --depth=1

if [[ ! -x "${RISCV_TOOLCHAIN_GCC}" || "${FORCE_TOOLCHAIN:-0}" == "1" ]]; then
  archive_path="${DEPS_DIR}/${toolchain_archive}"

  echo "Downloading ${toolchain_archive}"
  curl -fL --retry 3 -o "${archive_path}" "${RISCV_TOOLCHAIN_URL}"

  rm -rf "${RISCV_TOOLCHAIN_DIR}"
  mkdir -p "${RISCV_TOOLCHAIN_DIR}"

  echo "Extracting toolchain to ${RISCV_TOOLCHAIN_DIR}"
  unzip -q -o "${archive_path}" -d "${RISCV_TOOLCHAIN_DIR}"

  if [[ ! -x "${RISCV_TOOLCHAIN_GCC}" ]]; then
    echo "Failed to locate ${RISCV_TOOLCHAIN_GCC} after extraction" >&2
    exit 1
  fi
else
  echo "RISC-V toolchain already present at ${RISCV_TOOLCHAIN_GCC}"
fi

TOOLCHAIN_ROOT="${RISCV_TOOLCHAIN_DIR}"
ENV_FILE="${DEPS_DIR}/env.sh"

cat > "${ENV_FILE}" <<EOT
export PICO_SDK_PATH="${PICO_SDK_DIR}"
export PICO_TOOLCHAIN_PATH="${TOOLCHAIN_ROOT}"
EOT

cat <<EOT
Done.
Set:
  export PICO_SDK_PATH="${PICO_SDK_DIR}"
  export PICO_TOOLCHAIN_PATH="${TOOLCHAIN_ROOT}"
Or source:
  source "${ENV_FILE}"
EOT
