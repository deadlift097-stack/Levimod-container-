#!/usr/bin/env bash
set -euo pipefail

ABI="${1:-arm64-v8a}"
BUILD_ROOT="${BUILD_ROOT:-build}"
ANDROID_PLATFORM="${ANDROID_PLATFORM:-android-28}"
GENERATOR="${GENERATOR:-Ninja}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

resolve_ndk() {
  if [[ -n "${ANDROID_NDK_HOME:-}" && -d "${ANDROID_NDK_HOME}" ]]; then
    echo "${ANDROID_NDK_HOME}"
    return 0
  fi
  local sdk_root="${ANDROID_HOME:-${ANDROID_SDK_ROOT:-}}"
  if [[ -n "${sdk_root}" ]]; then
    if [[ -d "${sdk_root}/ndk/28.2.13676358" ]]; then
      echo "${sdk_root}/ndk/28.2.13676358"
      return 0
    fi
    if [[ -d "${sdk_root}/ndk" ]]; then
      local latest
      latest="$(find "${sdk_root}/ndk" -mindepth 1 -maxdepth 1 -type d | sort -r | head -n 1)"
      if [[ -n "${latest}" ]]; then
        echo "${latest}"
        return 0
      fi
    fi
  fi
  echo "Android NDK not found. Set ANDROID_NDK_HOME or ANDROID_HOME." >&2
  return 1
}

generate_config() {
  local cfg_dir="${REPO_ROOT}/${BUILD_ROOT}-config"
  cmake -S "${REPO_ROOT}" -B "${cfg_dir}" -G "${GENERATOR}"
  cmake --build "${cfg_dir}" --target levi_generate_config
  echo "${cfg_dir}/generated-config"
}

build_abi() {
  local target_abi="$1"
  local ndk_home
  ndk_home="$(resolve_ndk)"
  local toolchain="${ndk_home}/build/cmake/android.toolchain.cmake"
  local build_dir="${REPO_ROOT}/${BUILD_ROOT}-${target_abi}"
  local cfg_dir
  cfg_dir="$(generate_config)"

  cmake -S "${REPO_ROOT}" -B "${build_dir}" -G "${GENERATOR}" \
    -DCMAKE_TOOLCHAIN_FILE="${toolchain}" \
    -DANDROID_ABI="${target_abi}" \
    -DANDROID_PLATFORM="${ANDROID_PLATFORM}" \
    -DANDROID_STL="c++_shared" \
    -DLEVI_PACKAGE_CONFIG_DIR="${cfg_dir}"

  cmake --build "${build_dir}" --target levi_package
}

case "${ABI}" in
  arm64-v8a|armeabi-v7a)
    build_abi "${ABI}"
    ;;
  all)
    build_abi "arm64-v8a"
    build_abi "armeabi-v7a"
    ;;
  *)
    echo "Usage: $0 [arm64-v8a|armeabi-v7a|all]" >&2
    exit 2
    ;;
esac