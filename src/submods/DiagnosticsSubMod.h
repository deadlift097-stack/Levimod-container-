#pragma once

#include "mod/SubModRegistry.h"

namespace mod_container {

class DiagnosticsSubMod final : public ISubMod {
public:
    [[nodiscard]] const char* id() const noexcept override { return "core.diagnostics"; }
    [[nodiscard]] const char* displayName() const noexcept override {
        return "Runtime Diagnostics";
    }
    [[nodiscard]] const char* description() const noexcept override {
        return "Logs container lifecycle events and resolved game library base.";
    }

    void onLoad(ll::mod::NativeMod& host) override;
    void onEnable(ll::mod::NativeMod& host) override;
    void onGameAttached(ll::mod::NativeMod& host, void* gameHandle) override;
    void onDisable(ll::mod::NativeMod& host) override;
    void onUnload(ll::mod::NativeMod& host) override;
};

} // namespace mod_container