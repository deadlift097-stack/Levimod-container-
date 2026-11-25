/*
 * Mod Container plugin ABI (version 1)
 *
 * A plugin is one shared library (lib<name>.so, same ABI as the container) that exports
 *
 *     MC_PLUGIN_EXPORT const McPlugin *ModContainerGetPlugin(void);
 *
 * The container loads it from <mod dir>/submods/ or <data dir>/submods/, shows it in the
 * Mod Menu (OFF by default) and calls start()/stop() when the user toggles it.
 *
 * Plain C types only, so a plugin does not have to be built with the container's headers
 * or the same C++ standard library. A plugin that wants hooks, patches or other Preloader
 * features may additionally use the Preloader SDK (<pl/...>) itself.
 *
 * Rules:
 *  - Never let a C++ exception leave a callback.
 *  - Host functions (except log) must be called from inside start/stop/setting_changed.
 *  - Strings you return in McPlugin / McSetting are copied once at load time.
 */
#ifndef MOD_CONTAINER_PLUGIN_H
#define MOD_CONTAINER_PLUGIN_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MC_PLUGIN_ABI_VERSION 1u
#define MC_PLUGIN_ENTRY_SYMBOL "ModContainerGetPlugin"

#if defined(__GNUC__) || defined(__clang__)
#define MC_PLUGIN_EXPORT __attribute__((visibility("default")))
#else
#define MC_PLUGIN_EXPORT
#endif

enum { MC_LOG_DEBUG = 0, MC_LOG_INFO = 1, MC_LOG_WARN = 2, MC_LOG_ERROR = 3 };

enum {
    MC_SETTING_TOGGLE = 0,       /* values "true" / "false" */
    MC_SETTING_SLIDER_INT = 1,   /* integer as text, uses min/max */
    MC_SETTING_SLIDER_FLOAT = 2, /* float as text, uses min/max */
    MC_SETTING_TEXT = 3          /* free text */
};

typedef struct McRect {
    float x, y, w, h; /* overlay pixels */
    uint32_t color;   /* 0xFFFFFFFF = opaque white */
} McRect;

/* Passed to start(). Valid until stop() returns. */
typedef struct McHost {
    uint32_t abi_version;
    uint32_t struct_size; /* sizeof(McHost) as built by the container */
    void *ctx;            /* pass back as the first argument of every call */

    void (*log)(void *ctx, int level, const char *message);

    /* Copies the saved value of a setting into buffer (NUL-terminated, truncated to
     * capacity). Returns the full length, or 0 if the key is unknown. */
    size_t (*get_setting)(void *ctx, const char *key, char *buffer, size_t capacity);
    /* Saves a setting. Returns 0 on success. */
    int (*set_setting)(void *ctx, const char *key, const char *value);

    /* Overlay surface size in pixels. Returns 0 on success, -1 if the HUD is not ready. */
    int (*surface_size)(void *ctx, float *width, float *height);
    /* Replaces this plugin's overlay with filled rectangles; count 0 clears it. */
    int (*draw_rects)(void *ctx, const McRect *rects, size_t count);
} McHost;

typedef struct McSetting {
    const char *key;   /* [A-Za-z0-9_], stable */
    const char *title; /* shown in the menu */
    int type;          /* MC_SETTING_* */
    const char *default_value;
    const char *min_value; /* sliders only, may be NULL */
    const char *max_value; /* sliders only, may be NULL */
} McSetting;

typedef struct McPlugin {
    uint32_t abi_version; /* MC_PLUGIN_ABI_VERSION */
    const char *id;       /* [a-z0-9_]{1,32}, stable across releases */
    const char *display_name;
    const char *description;
    const McSetting *settings;
    size_t setting_count; /* at most 32 */

    int (*start)(const McHost *host); /* return 0 on success */
    void (*stop)(void);
    void (*setting_changed)(const char *key, const char *value); /* may be NULL */
} McPlugin;

typedef const McPlugin *(*McPluginEntryFn)(void);

#ifdef __cplusplus
}
#endif

#endif /* MOD_CONTAINER_PLUGIN_H */
