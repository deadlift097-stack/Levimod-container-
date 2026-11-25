#!/usr/bin/env bash
set -euo pipefail

ABI="${1:-arm64-v8a}"
ANDROID_PLATFORM="${ANDROID_PLATFORM:-android-28}"
NDK_DIR="${ANDROID_NDK_HOME:-}"

if [[ -z "$NDK_DIR" && -n "${ANDROID_HOME:-}" ]]; then
    if [[ -d "$ANDROID_HOME/ndk/28.2.13676358" ]]; then
        NDK_DIR="$ANDROID_HOME/ndk/28.2.13676358"
    elif [[ -d "$ANDROID_HOME/ndk" ]]; then
        NDK_DIR="$(find "$ANDROID_HOME/ndk" -mindepth 1 -maxdepth 1 -type d | sort -r | head -n1)"
    fi
fi

if [[ -z "$NDK_DIR" || ! -d "$NDK_DIR" ]]; then
    echo "Android NDK not found. Set ANDROID_NDK_HOME or ANDROID_HOME." >&2
    exit 1
fi

cmake -S . -B build-config -G Ninja
cmake --build build-config --target levi_generate_config

cmake -S . -B "build-${ABI}" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="${NDK_DIR}/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI="${ABI}" \
    -DANDROID_PLATFORM="${ANDROID_PLATFORM}" \
    -DANDROID_STL="c++_shared" \
    -DLEVI_PACKAGE_CONFIG_DIR="$(pwd)/build-config/generated-config"

cmake --build "build-${ABI}" --target levi_package