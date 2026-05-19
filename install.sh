#!/usr/bin/env bash
set -euo pipefail

REPO_URL="${DNSBENCHMARK_REPO_URL:-https://github.com/lorz/dns-benchmark.git}"
REPO_REF="${DNSBENCHMARK_REF:-main}"
INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"

if [[ "${EUID}" -eq 0 ]]; then
  SUDO=""
else
  SUDO="sudo"
fi

TEMP_DIR=""
cleanup() {
  if [[ -n "${TEMP_DIR}" && -d "${TEMP_DIR}" ]]; then
    rm -rf "${TEMP_DIR}"
  fi
}
trap cleanup EXIT

if [[ -f "./CMakeLists.txt" && -d "./src" ]]; then
  SOURCE_DIR="$(pwd)"
  echo "Using current directory as source: ${SOURCE_DIR}"
else
  TEMP_DIR="$(mktemp -d)"
  SOURCE_DIR="${TEMP_DIR}/repo"
  echo "Downloading source from ${REPO_URL} (${REPO_REF})..."
  git clone --depth 1 --branch "${REPO_REF}" "${REPO_URL}" "${SOURCE_DIR}"
fi

BUILD_DIR="${SOURCE_DIR}/build"

echo "[1/4] Installing Debian/Ubuntu dependencies..."
${SUDO} apt-get update
${SUDO} apt-get install -y --no-install-recommends \
  build-essential \
  cmake \
  git \
  ninja-build

echo "[2/4] Configuring project..."
cmake -S "${SOURCE_DIR}" -B "${BUILD_DIR}" -G Ninja -DCMAKE_BUILD_TYPE=Release

echo "[3/4] Building dnsbenchmark..."
cmake --build "${BUILD_DIR}" --config Release

echo "[4/4] Installing to ${INSTALL_PREFIX}..."
${SUDO} cmake --install "${BUILD_DIR}" --prefix "${INSTALL_PREFIX}"

echo "Done. Try: dnsbenchmark --help"
