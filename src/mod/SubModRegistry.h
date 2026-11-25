#pragma once

#include <memory>
#include <string>
#include <vector>

#include <pl/Mod.hpp>

namespace mod_container {

struct ModConfig;

class ISubMod {
public:
    virtual ~ISubMod() = default;

    [[nodiscard]] virtual const char* id() const noexcept = 0;
    [[nodiscard]] virtual const char* displayName() const noexcept = 0;
    [[nodiscard]] virtual const char* description() const noexcept = 0;

    virtual void onLoad(ll::mod::NativeMod& host) = 0;
    virtual void onEnable(ll::mod::NativeMod& host) = 0;
    virtual void onGameAttached(ll::mod::NativeMod& host, void* gameHandle) {
        (void)host;
        (void)gameHandle;
    }
    virtual void onDisable(ll::mod::NativeMod& host) = 0;
    virtual void onUnload(ll::mod::NativeMod& host) = 0;

    [[nodiscard]] bool isEnabled() const noexcept { return mEnabled; }
    void setEnabled(bool enabled) noexcept { mEnabled = enabled; }

private:
    bool mEnabled = true;
};

class SubModRegistry {
public:
    static SubModRegistry& instance();

    void registerBuiltins();
    void registerSubMod(std::shared_ptr<ISubMod> subMod);
    void clear();

    void syncFromConfig(ModConfig& config);
    void syncToConfig(ModConfig& config) const;

    void dispatchLoad(ll::mod::NativeMod& host);
    void dispatchEnable(ll::mod::NativeMod& host);
    void dispatchGameAttached(ll::mod::NativeMod& host, void* gameHandle);
    void dispatchDisable(ll::mod::NativeMod& host);
    void dispatchUnload(ll::mod::NativeMod& host);

    [[nodiscard]] const std::vector<std::shared_ptr<ISubMod>>& all() const noexcept {
        return mSubMods;
    }

    [[nodiscard]] std::shared_ptr<ISubMod> findById(std::string_view id) const;

private:
    SubModRegistry() = default;
    std::vector<std::shared_ptr<ISubMod>> mSubMods;
    bool mBuiltinsRegistered = false;
};

} // namespace mod_container