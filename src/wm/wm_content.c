#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <stdlib.h>
#include <wayland-server.h>
#include <wlr/util/log.h>

#include "wm/wm_content.h"
#include "wm/wm_server.h"

struct wm_content_vtable wm_content_base_vtable;

void wm_content_init(struct wm_content* content, struct wm_server* server) {
    content->vtable = &wm_content_base_vtable;

    content->wm_server = server;
    content->display_x = 0.;
    content->display_y = 0.;
    content->display_width = 0.;
    content->display_height = 0.;
    content->z_index = 0;
    wl_list_insert(&content->wm_server->wm_contents, &content->link);
}

void wm_content_base_destroy(struct wm_content* content) {
    wl_list_remove(&content->link);
}

void wm_content_set_box(struct wm_content* content, double x, double y, double width, double height) {

    content->display_x = x;
    content->display_y = y;
    content->display_width = width;
    content->display_height = height;
}

void wm_content_get_box(struct wm_content* content, double* display_x, double* display_y, double* display_width, double* display_height){
    *display_x = content->display_x;
    *display_y = content->display_y;
    *display_width = content->display_width;
    *display_height = content->display_height;
}

struct wm_content_vtable wm_content_base_vtable = {
    .destroy = wm_content_base_destroy,
};
