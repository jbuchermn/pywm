#ifndef WM_VIEW_XWAYLAND_H
#define WM_VIEW_XWAYLAND_H

#include <stdlib.h>
#include <wayland-server.h>
#include <wlr/xwayland.h>

#include "wm/wm_view.h"

struct wm_view_xwayland;

struct wm_view_xwayland_child {
    struct wl_list link;  // wm_view_xwayland::children
    struct wm_view_xwayland* parent;

    struct wlr_xwayland_surface* wlr_xwayland_surface;

    bool mapped;

    struct wl_listener map;
    struct wl_listener request_configure;
    struct wl_listener unmap;
    struct wl_listener destroy;
};

void wm_view_xwayland_child_init(struct wm_view_xwayland_child* child, struct wm_view_xwayland* parent, struct wlr_xwayland_surface* surface);
void wm_view_xwayland_child_destroy(struct wm_view_xwayland_child* child);


struct wm_view_xwayland {
    struct wm_view super;

    struct wlr_xwayland_surface* wlr_xwayland_surface;

    struct wl_list children;

    /* To e used in case the view becomes a child upon mapping */
    struct {
        int x;
        int y;
    } configure_upon_child;
    bool configure_upon_child_set;

    struct wl_listener map;
    struct wl_listener request_configure;
    struct wl_listener unmap;
    struct wl_listener destroy;
};

void wm_view_xwayland_init(struct wm_view_xwayland* view, struct wm_server* server, struct wlr_xwayland_surface* surface);

struct wm_view_vtable wm_view_xwayland_vtable;

#endif
