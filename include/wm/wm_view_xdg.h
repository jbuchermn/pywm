#ifndef WM_VIEW_XDG_H
#define WM_VIEW_XDG_H

#include <wayland-server.h>
#include <wlr/types/wlr_xdg_shell.h>

#include "wm/wm_view.h"

struct wm_view_xdg;

struct wm_xdg_subsurface {
    struct wm_view_xdg* toplevel;

    struct wlr_subsurface* wlr_subsurface;

    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener destroy;
    struct wl_listener new_subsurface;
    struct wl_listener surface_commit;
};

void wm_xdg_subsurface_init(struct wm_xdg_subsurface* subsurface, struct wm_view_xdg* toplevel, struct wlr_subsurface* wlr_subsurface);
void wm_xdg_subsurface_destroy(struct wm_xdg_subsurface* subsurface);


struct wm_popup_xdg {
    struct wm_view_xdg* toplevel;

    struct wlr_xdg_popup* wlr_xdg_popup;

    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener destroy;
    struct wl_listener new_popup;
    struct wl_listener new_subsurface;
    struct wl_listener surface_commit;
};

void wm_popup_xdg_init(struct wm_popup_xdg* popup, struct wm_view_xdg* toplevel, struct wlr_xdg_popup* wlr_xdg_popup);
void wm_popup_xdg_destroy(struct wm_popup_xdg* popup);

struct wm_view_xdg {
    struct wm_view super;

    struct wlr_xdg_surface* wlr_xdg_surface;

    bool floating;
    bool constrain_popups_to_toplevel; /* false = constrain to output */

    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener destroy;
    struct wl_listener new_popup;
    struct wl_listener new_subsurface;

    struct wl_listener surface_commit;

    struct wl_listener request_fullscreen;
    struct wl_listener request_move;
    struct wl_listener request_resize;
    struct wl_listener request_maximize;
    struct wl_listener request_minimize;
    struct wl_listener request_show_window_menu;
};

void wm_view_xdg_init(struct wm_view_xdg* view, struct wm_server* server, struct wlr_xdg_surface* surface);


#endif
