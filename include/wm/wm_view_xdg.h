#ifndef WM_VIEW_XDG_H
#define WM_VIEW_XDG_H

#include <wayland-server.h>
#include <wlr/types/wlr_xdg_shell.h>

#include "wm/wm_view.h"

struct wm_view_xdg;

struct wm_view_xdg {
    struct wm_view super;

    struct wlr_xdg_surface* wlr_xdg_surface;

    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener destroy;
};

void wm_view_xdg_init(struct wm_view_xdg* view, struct wm_server* server, struct wlr_xdg_surface* surface);

struct wm_view_vtable wm_view_xdg_vtable;

#endif
