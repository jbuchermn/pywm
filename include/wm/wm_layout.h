#ifndef WM_LAYOUT_H
#define WM_LAYOUT_H

#include <stdio.h>
#include <wayland-server.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>

struct wm_server;
struct wm_view;
struct wm_content;
struct wm_output;

struct wm_layout {
    struct wm_server* wm_server;

    struct wlr_output_layout* wlr_output_layout;
    struct wl_list wm_outputs; // wm_output::link

    struct wl_listener change;

    int fastest_output_mHz;
};

void wm_layout_init(struct wm_layout* layout, struct wm_server* server);
void wm_layout_destroy(struct wm_layout* layout);

void wm_layout_add_output(struct wm_layout* layout, struct wlr_output* output);
void wm_layout_remove_output(struct wm_layout* layout, struct wm_output* output);

/* Damage whole output layout */
void wm_layout_damage_whole(struct wm_layout* layout);

void wm_layout_damage_from(struct wm_layout* layout, struct wm_content* content, struct wlr_surface* origin);

void wm_layout_update_content_outputs(struct wm_layout* layout, struct wm_content* content);

void wm_layout_printf(FILE* file, struct wm_layout* layout);

#endif
