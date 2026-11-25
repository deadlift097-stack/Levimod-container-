#include "mod/GameWatcher.h"

#include <chrono>
#include <dlfcn.h>

namespace mod_container {

namespace {

constexpr const char* kGameLibrary = "libminecraftpe.so";

void* probeGameLibrary() noexcept {
    void* handle = dlopen(kGameLibrary, RTLD_NOW | RTLD_NOLOAD);
    if (handle) {
        // Balance the reference count added by RTLD_NOLOAD.
        dlclose(handle);
    }
    return handle;
}

} // namespace

GameWatcher::~GameWatcher() { stop(); }

void GameWatcher::start(int settleDelayMs, OnReadyCallback onReady) {
    stop();

    mStop.store(false);
    mAttached.store(false);

    const int clampedSettleMs = settleDelayMs < 0 ? 0 : settleDelayMs;

    mWorker = std::thread([this, clampedSettleMs, cb = std::move(onReady)]() {
        void* handle = nullptr;
        // Up to ~5 minutes bounded poll (fast at first, then 50ms backoff).
        for (int i = 0; i < 6500; ++i) {
            if (mStop.load()) {
                return;
            }
            handle = probeGameLibrary();
            if (handle) {
                break;
            }
            const auto sleepMs = (i < 1000) ? 5 : 50;
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
        }

        if (!handle || mStop.load()) {
            return;
        }

        // Wait for relocations & early initializers to settle before hooking.
        for (int waited = 0; waited < clampedSettleMs; waited += 50) {
            if (mStop.load()) {
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        if (mStop.load()) {
            return;
        }

        void* settledHandle = probeGameLibrary();
        if (!settledHandle) {
            return;
        }

        std::lock_guard<std::mutex> lock(mCallbackMutex);
        if (mStop.load() || mAttached.exchange(true)) {
            return;
        }
        if (cb) {
            cb(settledHandle);
        }
    });
}

void GameWatcher::stop() {
    mStop.store(true);
    if (mWorker.joinable()) {
        mWorker.join();
    }
    mAttached.store(false);
}

} // namespace mod_container