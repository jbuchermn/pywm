#ifndef WM_CONFIG_H
#define WM_CONFIG_H

#include <stdbool.h>
#include <wayland-server.h>

#define WM_CONFIG_POS_MIN -1000000
#define WM_CONFIG_STRLEN 100

struct wm_server;

struct wm_config_output {
    struct wl_list link; // wm_config::outputs

    char name[WM_CONFIG_STRLEN];
    double scale;

    /* Unit: pixels */
    int width;
    int height;

    /* Refresh rate in mHz */
    int mHz;

    /* Unit: Logical pixels, > WM_CONFIG_POS_MIN */
    int pos_x;
    int pos_y;
};

struct wm_config {
    /* Excluded from runtime update */
    bool enable_output_manager;
    bool enable_xwayland;

    int callback_frequency;

    /* TODO: Update - Requires updating config on wlr_keyboard */
    char xkb_model[WM_CONFIG_STRLEN];
    char xkb_layout[WM_CONFIG_STRLEN];
    char xkb_options[WM_CONFIG_STRLEN];

    struct wl_list outputs;

    /* TODO: Update - Requires updating / reallocating wlr_xcursor_managers */
    const char *xcursor_theme;
    int xcursor_size;

    int focus_follows_mouse;

    /* TODO: Update - Requires updating all wm_view_xdgs */
    int constrain_popups_to_toplevel;

    /* TODO: Update - Requires updating wlr_server_decoration manager */
    int encourage_csd;

    int debug_f1;
};

void wm_config_init_default(struct wm_config *config);
void wm_config_reset_default(struct wm_config* config);
void wm_config_reconfigure(struct wm_config* config, struct wm_server* server);
void wm_config_add_output(struct wm_config *config, const char *name,
                          double scale, int width, int height, int mHz,
                          int pos_x, int pos_y);
struct wm_config_output *wm_config_find_output(struct wm_config *config,
                                               const char *name);
void wm_config_destroy(struct wm_config *config);

#endif
