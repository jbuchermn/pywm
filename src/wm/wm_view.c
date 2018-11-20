#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <stdlib.h>
#include <wayland-server.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <wlr/xwayland.h>

#include "wm/wm_view.h"
#include "wm/wm_seat.h"
#include "wm/wm_server.h"
#include "wm/wm.h"


void wm_view_base_init(struct wm_view* view, struct wm_server* server){
    view->wm_server = server;
    view->mapped = false;
    view->display_x = 0.;
    view->display_y = 0.;
    view->display_width = 0.;
    view->display_height = 0.;

    wl_list_insert(&view->wm_server->wm_views, &view->link);
}

void wm_view_base_destroy(struct wm_view* view){
    wl_list_remove(&view->link);
}


void wm_view_base_set_box(struct wm_view* view, double x, double y, double width, double height){
    view->display_x = x;
    view->display_y = y;
    view->display_width = width;
    view->display_height = height;
}

void wm_view_base_get_box(struct wm_view* view, double* display_x, double* display_y, double* display_width, double* display_height){
    *display_x = view->display_x;
    *display_y = view->display_y;
    *display_width = view->display_width;
    *display_height = view->display_height;
}

struct wm_view_vtable wm_view_base_vtable = {
    .destroy = wm_view_base_destroy,
    .get_info = NULL,
    .set_box = wm_view_base_set_box,
    .get_box = wm_view_base_get_box,
    .request_size = NULL,
    .get_size = NULL,
    .get_size_constraints = NULL,
    .focus = NULL,
    .surface_at = NULL,
    .for_each_surface = NULL,
    .is_floating = NULL,
    .get_parent = NULL
};
