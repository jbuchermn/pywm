#ifndef WM_OUTPUT_H
#define WM_OUTPUT_H

#include <wayland-server.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_damage.h>

struct wm_layout;

struct wm_output {
    struct wm_server* wm_server;
    struct wm_layout* wm_layout;
    struct wl_list link; // wm_layout::wm_outputs

    struct wlr_output* wlr_output;
    struct wlr_output_damage* wlr_output_damage;

    double scale;

    struct wl_listener destroy;
    struct wl_listener commit;
    struct wl_listener mode;
    struct wl_listener present;
    struct wl_listener damage_frame;
    struct wl_listener damage_destroy;
};

void wm_output_init(struct wm_output* output, struct wm_server* server, struct wm_layout* layout, struct wlr_output* out);
void wm_output_destroy(struct wm_output* output);


/*
 * Override name of next output to be initialised
 * Necessary, as wlroots provides no way of naming
 * headless outputs
 */
void wm_output_override_name(const char* name);


#endif
