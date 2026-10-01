#!/usr/bin/env bash
# Installs the Ubuntu dependencies required by ./scripts/run.sh -t linux.
set -euo pipefail

readonly SLANG_VERSION="2026.19"
readonly SLANG_INSTALL_DIR="/opt/core-deps/slang/${SLANG_VERSION}"

if [[ "$(uname -s)" != "Linux" ]] || ! command -v apt-get >/dev/null; then
  echo "This script supports Ubuntu and other apt-based Linux distributions only." >&2
  exit 1
fi

case "$(uname -m)" in
  x86_64)
    slang_asset="slang-${SLANG_VERSION}-linux-x86_64-glibc-2.27.tar.gz"
    slang_sha256="5899bd40c3d1ee60eadd1d1b37acc4e88ae65cc25ac61651534d55b71a85fe75"
    ;;
  aarch64|arm64)
    slang_asset="slang-${SLANG_VERSION}-linux-aarch64-glibc-2.28.tar.gz"
    slang_sha256="f49229eb9606b47b122e1d8789be46a7bcd6bb2ffa94b06fae6d544a4e6df637"
    ;;
  *)
    echo "Unsupported Linux architecture: $(uname -m)" >&2
    exit 1
    ;;
esac

sudo -v
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  ca-certificates \
  clinfo \
  cmake \
  curl \
  git \
  glslc \
  libgl1-mesa-dev \
  libvulkan-dev \
  libx11-dev \
  libxcursor-dev \
  libxext-dev \
  libxi-dev \
  libxinerama-dev \
  libxrandr-dev \
  make \
  mesa-vulkan-drivers \
  ocl-icd-opencl-dev \
  pkg-config \
  pocl-opencl-icd \
  vulkan-tools \
  xxd

if ! command -v slangc >/dev/null || ! slangc -version 2>&1 | grep -q "${SLANG_VERSION}"; then
  download_dir="$(mktemp -d)"
  trap 'rm -rf "${download_dir}"' EXIT
  archive_path="${download_dir}/${slang_asset}"
  curl --fail --location --retry 3 \
    "https://github.com/shader-slang/slang/releases/download/v${SLANG_VERSION}/${slang_asset}" \
    --output "${archive_path}"
  echo "${slang_sha256}  ${archive_path}" | sha256sum --check --status

  sudo rm -rf "${SLANG_INSTALL_DIR}"
  sudo mkdir -p "${SLANG_INSTALL_DIR}"
  sudo tar -xzf "${archive_path}" -C "${SLANG_INSTALL_DIR}" --strip-components=1
  sudo ln -sf "${SLANG_INSTALL_DIR}/bin/slangc" /usr/local/bin/slangc
fi

git submodule sync --recursive
git submodule update --init --recursive

command -v cmake make glslc slangc xxd
cmake --version
glslc --version
slangc -version
vulkaninfo --summary || echo "Warning: Vulkan runtime verification failed; check the installed GPU driver."
clinfo >/dev/null || echo "Warning: OpenCL runtime verification failed; check the installed OpenCL ICD."

echo "Dependencies installed. Build with: ./scripts/run.sh -t linux"
