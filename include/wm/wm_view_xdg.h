#ifndef WM_VIEW_XDG_H
#define WM_VIEW_XDG_H

#include <wayland-server.h>
#include <wlr/types/wlr_xdg_shell.h>

#include "wm/wm_view.h"

struct wm_view_xdg;

struct wm_view_xdg_popup {
    struct wl_list link;  // wm_view_xdg::popups

    struct wlr_xdg_popup* wlr_xdg_popup;

    struct wm_view_xdg* parent;

    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener destroy;
    struct wl_listener new_popup;
};

void wm_view_xdg_popup_init(struct wm_view_xdg_popup* popup, struct wlr_xdg_popup* wlr_xdg_popup, struct wm_view_xdg* parent);
void wm_view_xdg_popup_destroy(struct wm_view_xdg_popup* popup);

struct wm_view_xdg {
    struct wm_view super;

    struct wlr_xdg_surface* wlr_xdg_surface;

    struct wl_list popups;  // wm_view_xdg_popup::link

    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener destroy;
    struct wl_listener new_popup;
};

void wm_view_xdg_init(struct wm_view_xdg* view, struct wm_server* server, struct wlr_xdg_surface* surface);

struct wm_view_vtable wm_view_xdg_vtable;

#endif
