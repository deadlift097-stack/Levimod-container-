#pragma once

#include <atomic>
#include <chrono>
#include <dlfcn.h>
#include <link.h>
#include <mutex>
#include <string_view>
#include <thread>

namespace my_mod {

// Safe deferred attach utility for native Bedrock hooks.
// Do NOT hook dlopen() in the Java launcher process: preloader enables mods
// before libminecraftpe.so is mapped/relocated. Instead, poll via dl_iterate_phdr
// or dlopen(RTLD_NOLOAD), wait a short settle delay, then install verified hooks.
class HookWatcher {
  public:
    static bool isGameLibraryMapped(std::string_view libraryName = "libminecraftpe.so") {
        struct Probe {
            std::string_view target;
            bool found = false;
        } probe{libraryName, false};

        dl_iterate_phdr(
            [](struct dl_phdr_info *info, size_t, void *data) -> int {
                auto *state = static_cast<Probe *>(data);
                if (info && info->dlpi_name &&
                    std::string_view(info->dlpi_name).find(state->target) != std::string_view::npos) {
                    state->found = true;
                    return 1;
                }
                return 0;
            },
            &probe);

        return probe.found;
    }

    template <typename Fn> void start(Fn &&onReady, int settleDelayMs = 5000) {
        std::lock_guard<std::mutex> lock(mMutex);
        if (mRunning.load() || mInstalled.load()) {
            return;
        }
        mStopRequested.store(false);
        mRunning.store(true);

        std::thread([this, callback = std::forward<Fn>(onReady), settleDelayMs]() mutable {
            constexpr int kMaxPolls = 12000;
            bool mapped = false;
            for (int i = 0; i < kMaxPolls; ++i) {
                if (mStopRequested.load()) {
                    mRunning.store(false);
                    return;
                }
                if (isGameLibraryMapped()) {
                    mapped = true;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(i < 6000 ? 1 : 50));
            }

            if (!mapped) {
                mRunning.store(false);
                return;
            }

            for (int waited = 0; waited < settleDelayMs; waited += 100) {
                if (mStopRequested.load()) {
                    mRunning.store(false);
                    return;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            std::lock_guard<std::mutex> installLock(mMutex);
            if (!mStopRequested.load() && !mInstalled.load()) {
                callback();
                mInstalled.store(true);
            }
            mRunning.store(false);
        }).detach();
    }

    void stop() {
        mStopRequested.store(true);
    }

  private:
    std::mutex mMutex;
    std::atomic<bool> mRunning{false};
    std::atomic<bool> mInstalled{false};
    std::atomic<bool> mStopRequested{false};
};

} // namespace my_mod