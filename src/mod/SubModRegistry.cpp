#include "mod/SubModRegistry.h"
#include "mod/Config.h"
#include "submods/DiagnosticsSubMod.h"

#include <algorithm>

namespace mod_container {

SubModRegistry& SubModRegistry::instance() {
    static SubModRegistry reg;
    return reg;
}

void SubModRegistry::registerBuiltins() {
    if (mBuiltinsRegistered) {
        return;
    }
    mBuiltinsRegistered = true;
    registerSubMod(std::make_shared<DiagnosticsSubMod>());
}

void SubModRegistry::registerSubMod(std::shared_ptr<ISubMod> subMod) {
    if (!subMod) {
        return;
    }
    if (findById(subMod->id())) {
        return;
    }
    mSubMods.push_back(std::move(subMod));
}

void SubModRegistry::clear() {
    mSubMods.clear();
    mBuiltinsRegistered = false;
}

std::shared_ptr<ISubMod> SubModRegistry::findById(std::string_view id) const {
    for (const auto& mod : mSubMods) {
        if (id == mod->id()) {
            return mod;
        }
    }
    return nullptr;
}

void SubModRegistry::syncFromConfig(ModConfig& config) {
    for (const auto& mod : mSubMods) {
        auto it = std::find_if(config.subMods.begin(), config.subMods.end(),
                               [&](const SubModConfigEntry& entry) {
                                   return entry.id == mod->id();
                               });
        if (it != config.subMods.end()) {
            mod->setEnabled(it->enabled);
        } else {
            config.subMods.push_back(SubModConfigEntry{
                .id = mod->id(),
                .displayName = mod->displayName(),
                .enabled = mod->isEnabled(),
            });
        }
    }
}

void SubModRegistry::syncToConfig(ModConfig& config) const {
    for (const auto& mod : mSubMods) {
        auto it = std::find_if(config.subMods.begin(), config.subMods.end(),
                               [&](const SubModConfigEntry& entry) {
                                   return entry.id == mod->id();
                               });
        if (it != config.subMods.end()) {
            it->displayName = mod->displayName();
            it->enabled = mod->isEnabled();
        } else {
            config.subMods.push_back(SubModConfigEntry{
                .id = mod->id(),
                .displayName = mod->displayName(),
                .enabled = mod->isEnabled(),
            });
        }
    }
}

void SubModRegistry::dispatchLoad(ll::mod::NativeMod& host) {
    for (const auto& mod : mSubMods) {
        if (mod->isEnabled()) {
            mod->onLoad(host);
        }
    }
}

void SubModRegistry::dispatchEnable(ll::mod::NativeMod& host) {
    for (const auto& mod : mSubMods) {
        if (mod->isEnabled()) {
            mod->onEnable(host);
        }
    }
}

void SubModRegistry::dispatchGameAttached(ll::mod::NativeMod& host, void* gameHandle) {
    for (const auto& mod : mSubMods) {
        if (mod->isEnabled()) {
            mod->onGameAttached(host, gameHandle);
        }
    }
}

void SubModRegistry::dispatchDisable(ll::mod::NativeMod& host) {
    for (auto it = mSubMods.rbegin(); it != mSubMods.rend(); ++it) {
        (*it)->onDisable(host);
    }
}

void SubModRegistry::dispatchUnload(ll::mod::NativeMod& host) {
    for (auto it = mSubMods.rbegin(); it != mSubMods.rend(); ++it) {
        (*it)->onUnload(host);
    }
}

} // namespace mod_container