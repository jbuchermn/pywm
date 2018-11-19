#ifndef WM_VIEW_XWAYLAND_H
#define WM_VIEW_XWAYLAND_H

#include <stdlib.h>
#include <wayland-server.h>
#include <wlr/xwayland.h>

#include "wm/wm_view.h"

struct wm_view_xwayland {
    struct wm_view super;

    struct wlr_xwayland_surface* wlr_xwayland_surface;

    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener destroy;
};

void wm_view_xwayland_init(struct wm_view_xwayland* view, struct wm_server* server, struct wlr_xwayland_surface* surface);

struct wm_view_vtable wm_view_xwayland_vtable;

#endif
