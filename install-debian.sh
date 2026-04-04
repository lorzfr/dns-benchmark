#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"

if [[ "${EUID}" -eq 0 ]]; then
  SUDO=""
else
  SUDO="sudo"
fi

echo "[1/4] Installing Debian build dependencies..."
${SUDO} apt-get update
${SUDO} apt-get install -y --no-install-recommends \
  build-essential \
  cmake \
  ninja-build

echo "[2/4] Configuring project..."
cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" -G Ninja -DCMAKE_BUILD_TYPE=Release

echo "[3/4] Building dnsbenchmark..."
cmake --build "${BUILD_DIR}" --config Release

echo "[4/4] Installing binary to ${INSTALL_PREFIX}..."
${SUDO} cmake --install "${BUILD_DIR}" --prefix "${INSTALL_PREFIX}"

echo "Done. Run: dnsbenchmark --help"
