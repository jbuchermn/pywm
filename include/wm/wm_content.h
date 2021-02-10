#ifndef WM_CONTENT_H
#define WM_CONTENT_H

#include <wayland-server.h>

struct wm_content_vtable;

struct wm_content {
    struct wl_list link;  // wm_server::wm_views
    struct wm_server* wm_server;

    struct wm_content_vtable* vtable;

    double display_x;
    double display_y;
    double display_width;
    double display_height;
    int z_index;
};

void wm_content_init(struct wm_content* content, struct wm_server* server);
void wm_content_base_destroy(struct wm_content* content);

void wm_content_set_box(struct wm_content* content, double x, double y, double width, double height);
void wm_content_get_box(struct wm_content* content, double* display_x, double* display_y, double* display_width, double* display_height);

struct wm_content_vtable {
    void (*destroy)(struct wm_content* view);
};

static inline void wm_content_destroy(struct wm_content* content){
    (*content->vtable->destroy)(content);
}

#endif
