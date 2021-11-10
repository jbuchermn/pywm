#ifndef WM_CONFIG_H
#define WM_CONFIG_H

#include <stdbool.h>

struct wm_config {
    double output_scale;
    bool enable_output_manager;
    bool enable_xwayland;

    int callback_frequency;

    const char *xkb_model;
    const char *xkb_layout;
    const char *xkb_options;

    const char *output_name;
    int output_width;
    int output_height;
    int output_mHz;

    const char* xcursor_theme;
    int xcursor_size;

    bool tap_to_click;
    bool natural_scroll;

    bool focus_follows_mouse;
    bool constrain_popups_to_toplevel;

    bool encourage_csd;

    bool debug_f1;
};

void wm_config_init_default(struct wm_config* config);

#endif
