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

find_deb_candidate() {
  find "${SCRIPT_DIR}" -maxdepth 2 -type f -name '*.deb' | head -n1
}

install_deb_if_requested() {
  local default_deb_path=""
  local deb_input=""

  default_deb_path="$(find_deb_candidate || true)"

  if [[ -t 0 ]]; then
    echo
    echo "Optional: install a .deb package now."
    if [[ -n "${default_deb_path}" ]]; then
      read -r -p "Install detected package '${default_deb_path}'? [y/N]: " deb_input
      if [[ "${deb_input}" =~ ^[Yy]$ ]]; then
        ${SUDO} apt-get install -y "${default_deb_path}"
        return
      fi
    fi

    read -r -p "Enter path to a .deb file to install (or press Enter to skip): " deb_input
    if [[ -n "${deb_input}" ]]; then
      ${SUDO} apt-get install -y "${deb_input}"
    else
      echo "Skipping .deb installation."
    fi
    return
  fi

  if [[ "${INSTALL_DEB:-0}" == "1" ]]; then
    if [[ -n "${DEB_FILE:-}" ]]; then
      ${SUDO} apt-get install -y "${DEB_FILE}"
    elif [[ -n "${default_deb_path}" ]]; then
      ${SUDO} apt-get install -y "${default_deb_path}"
    else
      echo "INSTALL_DEB=1 was set, but no .deb file was found. Set DEB_FILE=/path/to/package.deb."
      exit 1
    fi
  fi
}

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

echo "[4/5] Installing binary to ${INSTALL_PREFIX}..."
${SUDO} cmake --install "${BUILD_DIR}" --prefix "${INSTALL_PREFIX}"

echo "[5/5] Optional .deb install..."
install_deb_if_requested

echo "Done. Run: dnsbenchmark --help"
