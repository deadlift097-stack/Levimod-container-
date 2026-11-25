// Example container plugin. Copy this folder to add your own: plugins/<name>/.
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "ModContainerPlugin.h"

namespace {

const McHost *gHost = nullptr;

void say(int level, const char *text) {
    if (gHost)
        gHost->log(gHost->ctx, level, text);
}

void drawMarker(bool on) {
    if (!gHost)
        return;
    if (!on) {
        gHost->draw_rects(gHost->ctx, nullptr, 0);
        return;
    }
    char sizeText[16] = "12";
    gHost->get_setting(gHost->ctx, "marker_size", sizeText, sizeof sizeText);
    const float size = static_cast<float>(std::atof(sizeText));
    const McRect rect{16.0f, 16.0f, size, size, 0xFFFFFFFFu};
    gHost->draw_rects(gHost->ctx, &rect, 1);
}

bool readToggle(const char *key) {
    char text[8] = "false";
    gHost->get_setting(gHost->ctx, key, text, sizeof text);
    return std::strcmp(text, "true") == 0;
}

int start(const McHost *host) {
    gHost = host;
    char message[128] = {0};
    host->get_setting(host->ctx, "message", message, sizeof message);
    std::snprintf(message + std::strlen(message), sizeof message - std::strlen(message), " (plugin)");
    say(MC_LOG_INFO, message);
    drawMarker(readToggle("show_marker"));
    return 0;
}

void stop() {
    drawMarker(false);
    say(MC_LOG_INFO, "hello_plugin stopped");
    gHost = nullptr;
}

void settingChanged(const char *key, const char *value) {
    if (!gHost)
        return;
    if (std::strcmp(key, "show_marker") == 0 || std::strcmp(key, "marker_size") == 0) {
        (void)value;
        drawMarker(readToggle("show_marker"));
    }
}

const McSetting kSettings[] = {
    {"message", "Message", MC_SETTING_TEXT, "Hello from a plugin!", nullptr, nullptr},
    {"show_marker", "Show marker", MC_SETTING_TOGGLE, "false", nullptr, nullptr},
    {"marker_size", "Marker size", MC_SETTING_SLIDER_INT, "12", "4", "64"},
};

const McPlugin kPlugin = {
    MC_PLUGIN_ABI_VERSION,
    "hello_plugin",
    "Hello Plugin",
    "Example plugin: logs a message and can draw a small marker.",
    kSettings,
    sizeof kSettings / sizeof kSettings[0],
    start,
    stop,
    settingChanged,
};

} // namespace

extern "C" MC_PLUGIN_EXPORT const McPlugin *ModContainerGetPlugin(void) { return &kPlugin; }
