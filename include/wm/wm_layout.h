#ifndef WM_LAYOUT_H
#define WM_LAYOUT_H

#include <wayland-server.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>

struct wm_server;
struct wm_view;

struct wm_layout {
    struct wm_server* wm_server;

    struct wlr_output_layout* wlr_output_layout;
    struct wl_list wm_outputs; // wm_output::link

    /* First registered output for now - multiple screens are not
     * properly supported anyway - used to notify clients about 
     * the output they are on */
    struct wm_output* default_output;

    struct wl_listener change;
};

void wm_layout_init(struct wm_layout* layout, struct wm_server* server);
void wm_layout_destroy(struct wm_layout* layout);

void wm_layout_add_output(struct wm_layout* layout, struct wlr_output* output);

void wm_layout_damage_from(struct wm_layout* layout, struct wm_view* content);
void wm_layout_damage_whole(struct wm_layout* layout);

#endif
