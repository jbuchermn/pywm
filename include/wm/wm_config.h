#ifndef WM_CONFIG_H
#define WM_CONFIG_H

#include <stdbool.h>
#include <wayland-server.h>

struct wm_config_output {
    struct wl_list link; // wm_config::outputs

    const char *name;
    double scale;

    /* Unit: pixels */
    int width;
    int height;

    /* Refresh rate in mHz */
    int mHz;

    /* Unit: Logical pixels, >= 0 */
    int pos_x;
    int pos_y;
};

struct wm_config {
    bool enable_output_manager;
    bool enable_xwayland;

    int callback_frequency;
    int max_callback_frequency;

    const char *xkb_model;
    const char *xkb_layout;
    const char *xkb_options;

    struct wl_list outputs;

    const char *xcursor_theme;
    int xcursor_size;

    int focus_follows_mouse;
    int constrain_popups_to_toplevel;

    int encourage_csd;

    int debug_f1;
};

void wm_config_init_default(struct wm_config *config);
void wm_config_add_output(struct wm_config *config, const char *name,
                          double scale, int width, int height, int mHz,
                          int pos_x, int pos_y);
struct wm_config_output *wm_config_find_output(struct wm_config *config,
                                               const char *name);
void wm_config_destroy(struct wm_config *config);

#endif
