#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>

namespace mod_container {

// Crash-safe attachment watcher:
// - Never hooks dlopen (hooking dlopen in a Java host process recurses/crashes).
// - Polls with RTLD_NOLOAD, then waits settleDelayMs so ELF relocations finish
//   before any sub-mod resolves signatures or installs hooks.
class GameWatcher {
public:
    using OnReadyCallback = std::function<void(void* gameHandle)>;

    GameWatcher() = default;
    ~GameWatcher();

    GameWatcher(const GameWatcher&) = delete;
    GameWatcher& operator=(const GameWatcher&) = delete;

    void start(int settleDelayMs, OnReadyCallback onReady);
    void stop();

    [[nodiscard]] bool isAttached() const noexcept { return mAttached.load(); }

private:
    std::atomic<bool> mStop{false};
    std::atomic<bool> mAttached{false};
    std::mutex mCallbackMutex;
    std::thread mWorker;
};

} // namespace mod_container