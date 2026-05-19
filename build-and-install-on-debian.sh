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

echo "[1/5] Installing Debian build dependencies..."
${SUDO} apt-get update
${SUDO} apt-get install -y --no-install-recommends \
  build-essential \
  cmake \
  ninja-build

if [[ -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
  cached_source_dir="$(sed -n 's#^CMAKE_HOME_DIRECTORY:INTERNAL=##p' "${BUILD_DIR}/CMakeCache.txt" | head -n1)"
  if [[ -n "${cached_source_dir}" && "${cached_source_dir}" != "${SCRIPT_DIR}" ]]; then
    echo "Detected existing CMake cache for a different source directory:"
    echo "  cached: ${cached_source_dir}"
    echo "  current: ${SCRIPT_DIR}"
    echo "Removing stale build directory at ${BUILD_DIR}..."
    rm -rf "${BUILD_DIR}"
  fi
fi

echo "[2/5] Configuring project..."
cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" -G Ninja -DCMAKE_BUILD_TYPE=Release

echo "[3/5] Building dnsbenchmark..."
cmake --build "${BUILD_DIR}" --config Release

echo "[4/5] Building .deb package..."
(
  cd "${BUILD_DIR}"
  cpack -G DEB --config "${BUILD_DIR}/CPackConfig.cmake"
)

echo "[5/5] Installing binary to ${INSTALL_PREFIX}..."
${SUDO} cmake --install "${BUILD_DIR}" --prefix "${INSTALL_PREFIX}"

echo "Done. Run: dnsbenchmark --help"
