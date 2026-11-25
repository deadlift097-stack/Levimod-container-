#include "container/ExternalSubMod.h"

#include <algorithm>
#include <cstring>
#include <span>
#include <stdexcept>

namespace modcontainer {

namespace {

PluginState *findState(ModConfig &cfg, const std::string &id) {
    for (auto &state : cfg.plugins)
        if (state.id == id)
            return &state;
    return nullptr;
}

const PluginState *findState(const ModConfig &cfg, const std::string &id) {
    for (const auto &state : cfg.plugins)
        if (state.id == id)
            return &state;
    return nullptr;
}

PluginState &stateFor(ModConfig &cfg, const std::string &id) {
    if (auto *existing = findState(cfg, id))
        return *existing;
    cfg.plugins.push_back(PluginState{.id = id});
    return cfg.plugins.back();
}

std::string orEmpty(const char *text) { return text ? std::string(text) : std::string(); }

} // namespace

ExternalSubMod::ExternalSubMod(const McPlugin *plugin, std::filesystem::path file)
    : mPlugin(plugin), mFile(std::move(file)), mId(orEmpty(plugin->id)),
      mDisplayName(orEmpty(plugin->display_name)), mDescription(orEmpty(plugin->description)) {
    for (size_t i = 0; i < plugin->setting_count; ++i) {
        const McSetting &src = plugin->settings[i];
        mSettings.push_back(SettingDesc{
            .key = orEmpty(src.key),
            .title = orEmpty(src.title),
            .type = src.type,
            .defaultValue = orEmpty(src.default_value),
            .minValue = orEmpty(src.min_value),
            .maxValue = orEmpty(src.max_value),
        });
    }
}

const ExternalSubMod::SettingDesc *ExternalSubMod::findSetting(std::string_view key) const {
    for (const auto &desc : mSettings)
        if (desc.key == key)
            return &desc;
    return nullptr;
}

std::string ExternalSubMod::currentValue(const ModConfig &cfg, const SettingDesc &desc) const {
    if (const auto *state = findState(cfg, mId))
        for (const auto &setting : state->settings)
            if (setting.key == desc.key)
                return setting.value;
    return desc.defaultValue;
}

bool ExternalSubMod::enabledInConfig(const ModConfig &cfg) const {
    const auto *state = findState(cfg, mId);
    return state ? state->enabled : false;
}

void ExternalSubMod::setEnabledInConfig(ModConfig &cfg, bool enabled) const {
    stateFor(cfg, mId).enabled = enabled;
}

void ExternalSubMod::describe(const ModConfig &cfg, pl::modmenu::ModuleBuilder &builder) const {
    using pl::modmenu::ConfigType;
    for (const auto &desc : mSettings) {
        ConfigType type = ConfigType::Text;
        switch (desc.type) {
        case MC_SETTING_TOGGLE: type = ConfigType::Toggle; break;
        case MC_SETTING_SLIDER_INT: type = ConfigType::SliderInt; break;
        case MC_SETTING_SLIDER_FLOAT: type = ConfigType::SliderFloat; break;
        default: break;
        }
        builder.config(desc.key, desc.title, type, currentValue(cfg, desc), desc.minValue,
                       desc.maxValue);
    }
}

void ExternalSubMod::start(SubModHost &host) {
    mHost = McHost{};
    mHost.abi_version = MC_PLUGIN_ABI_VERSION;
    mHost.struct_size = sizeof(McHost);
    mHost.ctx = this;
    mHost.log = &ExternalSubMod::hostLog;
    mHost.get_setting = &ExternalSubMod::hostGetSetting;
    mHost.set_setting = &ExternalSubMod::hostSetSetting;
    mHost.surface_size = &ExternalSubMod::hostSurfaceSize;
    mHost.draw_rects = &ExternalSubMod::hostDrawRects;

    mActiveHost = &host;
    const int rc = mPlugin->start(&mHost);
    if (rc != 0) {
        mActiveHost = nullptr;
        throw std::runtime_error("plugin start() returned " + std::to_string(rc));
    }
}

void ExternalSubMod::stop(SubModHost &host) {
    (void)host;
    mPlugin->stop();
    mActiveHost = nullptr;
}

void ExternalSubMod::configChanged(SubModHost &host, std::string_view key, std::string_view value) {
    const auto *desc = findSetting(key);
    if (!desc)
        return;

    auto &state = stateFor(host.config(), mId);
    auto it = std::find_if(state.settings.begin(), state.settings.end(),
                           [&](const PluginSetting &s) { return s.key == desc->key; });
    if (it == state.settings.end())
        state.settings.push_back(PluginSetting{desc->key, std::string(value)});
    else
        it->value = std::string(value);
    host.saveConfig();

    if (mActiveHost && mPlugin->setting_changed) {
        const std::string keyCopy(key);
        const std::string valueCopy(value);
        mPlugin->setting_changed(keyCopy.c_str(), valueCopy.c_str());
    }
}

// ---- host functions handed to the plugin -------------------------------------------------

void ExternalSubMod::hostLog(void *ctx, int level, const char *message) {
    auto *self = static_cast<ExternalSubMod *>(ctx);
    if (!self->mActiveHost || !message)
        return;
    auto &log = self->mActiveHost->logger();
    switch (level) {
    case MC_LOG_DEBUG: log.debug("[{}] {}", self->mId, message); break;
    case MC_LOG_WARN: log.warn("[{}] {}", self->mId, message); break;
    case MC_LOG_ERROR: log.error("[{}] {}", self->mId, message); break;
    default: log.info("[{}] {}", self->mId, message); break;
    }
}

size_t ExternalSubMod::hostGetSetting(void *ctx, const char *key, char *buffer, size_t capacity) {
    auto *self = static_cast<ExternalSubMod *>(ctx);
    if (!self->mActiveHost || !key)
        return 0;
    const auto *desc = self->findSetting(key);
    if (!desc)
        return 0;

    const std::string value = self->currentValue(self->mActiveHost->config(), *desc);
    if (buffer && capacity > 0) {
        const size_t n = std::min(value.size(), capacity - 1);
        std::memcpy(buffer, value.data(), n);
        buffer[n] = '\0';
    }
    return value.size();
}

int ExternalSubMod::hostSetSetting(void *ctx, const char *key, const char *value) {
    auto *self = static_cast<ExternalSubMod *>(ctx);
    if (!self->mActiveHost || !key || !value || !self->findSetting(key))
        return -1;

    auto &state = stateFor(self->mActiveHost->config(), self->mId);
    auto it = std::find_if(state.settings.begin(), state.settings.end(),
                           [&](const PluginSetting &s) { return s.key == key; });
    if (it == state.settings.end())
        state.settings.push_back(PluginSetting{key, value});
    else
        it->value = value;
    self->mActiveHost->saveConfig();
    return 0;
}

int ExternalSubMod::hostSurfaceSize(void *ctx, float *width, float *height) {
    (void)ctx;
    if (!width || !height)
        return -1;
    const auto size = pl::modmenu::getHudSurfaceSize();
    if (size.width <= 0.0f || size.height <= 0.0f)
        return -1;
    *width = size.width;
    *height = size.height;
    return 0;
}

int ExternalSubMod::hostDrawRects(void *ctx, const McRect *rects, size_t count) {
    auto *self = static_cast<ExternalSubMod *>(ctx);
    if (!self->mActiveHost || (count > 0 && !rects) || count > 512)
        return -1;

    std::vector<pl::modmenu::DrawCommand> commands;
    commands.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        pl::modmenu::DrawCommand cmd;
        cmd.type = pl::modmenu::DrawCommandType::RectFilled;
        cmd.x = rects[i].x;
        cmd.y = rects[i].y;
        cmd.w = rects[i].w;
        cmd.h = rects[i].h;
        cmd.color = rects[i].color;
        commands.push_back(std::move(cmd));
    }
    pl::modmenu::submitDrawCommands(
        self->mActiveHost->moduleId(self->mId),
        std::span<const pl::modmenu::DrawCommand>(commands.data(), commands.size()));
    return 0;
}

} // namespace modcontainer
