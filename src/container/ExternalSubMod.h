#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "container/SubMod.h"
#include "ModContainerPlugin.h"

namespace modcontainer {

// Adapts a loaded plugin (C ABI, see sdk/ModContainerPlugin.h) to ISubMod. All metadata is
// copied at construction, so the plugin's own string storage is only needed for callbacks.
class ExternalSubMod final : public ISubMod {
  public:
    struct SettingDesc {
        std::string key;
        std::string title;
        int type = MC_SETTING_TEXT;
        std::string defaultValue;
        std::string minValue;
        std::string maxValue;
    };

    ExternalSubMod(const McPlugin *plugin, std::filesystem::path file);

    [[nodiscard]] std::string id() const override { return mId; }
    [[nodiscard]] std::string displayName() const override { return mDisplayName; }
    [[nodiscard]] std::string description() const override { return mDescription; }
    [[nodiscard]] const std::filesystem::path &file() const { return mFile; }

    [[nodiscard]] bool enabledInConfig(const ModConfig &cfg) const override;
    void setEnabledInConfig(ModConfig &cfg, bool enabled) const override;
    void describe(const ModConfig &cfg, pl::modmenu::ModuleBuilder &builder) const override;

    void start(SubModHost &host) override;
    void stop(SubModHost &host) override;
    void configChanged(SubModHost &host, std::string_view key, std::string_view value) override;

  private:
    [[nodiscard]] const SettingDesc *findSetting(std::string_view key) const;
    [[nodiscard]] std::string currentValue(const ModConfig &cfg, const SettingDesc &desc) const;

    // McHost trampolines; ctx is the ExternalSubMod.
    static void hostLog(void *ctx, int level, const char *message);
    static size_t hostGetSetting(void *ctx, const char *key, char *buffer, size_t capacity);
    static int hostSetSetting(void *ctx, const char *key, const char *value);
    static int hostSurfaceSize(void *ctx, float *width, float *height);
    static int hostDrawRects(void *ctx, const McRect *rects, size_t count);

    const McPlugin *mPlugin;
    std::filesystem::path mFile;
    std::string mId;
    std::string mDisplayName;
    std::string mDescription;
    std::vector<SettingDesc> mSettings;

    McHost mHost{};
    SubModHost *mActiveHost = nullptr; // set while the plugin is running
};

} // namespace modcontainer
