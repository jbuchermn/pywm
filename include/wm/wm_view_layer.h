#ifndef WM_VIEW_LAYER_H
#define WM_VIEW_LAYER_H

#include <wayland-server.h>
#include <wlr/types/wlr_layer_shell_v1.h>

#include "wm/wm_view.h"

struct wm_view_layer;

struct wm_layer_subsurface {
    struct wl_list link; //wm_view_layer::subsurfaces / wm_popup_layer::subsurfaces / wm_layer_subsurface::subsurfaces

    struct wm_view_layer* root;

    struct wlr_subsurface* wlr_subsurface;

    struct wl_list subsurfaces;

    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener destroy;
    struct wl_listener new_subsurface;
    struct wl_listener surface_commit;
};

void wm_layer_subsurface_init(struct wm_layer_subsurface* subsurface, struct wm_view_layer* root, struct wlr_subsurface* wlr_subsurface);
void wm_layer_subsurface_destroy(struct wm_layer_subsurface* subsurface);


struct wm_popup_layer {
    struct wl_list link; //wm_view_layer::popups / wm_popup_layer::popups

    struct wm_view_layer* root;

    struct wlr_xdg_popup* wlr_xdg_popup;

    struct wl_list popups;
    struct wl_list subsurfaces;

    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener destroy;
    struct wl_listener new_popup;
    struct wl_listener new_subsurface;
    struct wl_listener surface_commit;
};

void wm_popup_layer_init(struct wm_popup_layer* popup, struct wm_view_layer* root, struct wlr_xdg_popup* wlr_xdg_popup);
void wm_popup_layer_destroy(struct wm_popup_layer* popup);

struct wm_view_layer {
    struct wm_view super;

    struct wlr_layer_surface_v1* wlr_layer_surface;

    int width;
    int height;

    struct wl_list popups;
    struct wl_list subsurfaces;

    int size_constraints[9];

    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener destroy;
    struct wl_listener new_popup;
    struct wl_listener new_subsurface;

    struct wl_listener surface_commit;
};

void wm_view_layer_init(struct wm_view_layer* view, struct wm_server* server, struct wlr_layer_surface_v1* surface);


#endif //
