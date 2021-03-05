#ifndef WM_CONTENT_H
#define WM_CONTENT_H

#include <wayland-server.h>
#include <pixman.h>
#include <wlr/types/wlr_surface.h>
#include <wlr/util/log.h>

struct wm_output;

struct wm_content_vtable;

struct wm_content {
    struct wl_list link;  // wm_server::wm_views
    struct wm_server* wm_server;

    struct wm_content_vtable* vtable;

    double display_x;
    double display_y;
    double display_width;
    double display_height;
    double corner_radius;

    int z_index;
    double opacity;

    /* Accepts input and is displayed clearly during lock - careful */
    bool lock_enabled;
};

void wm_content_init(struct wm_content* content, struct wm_server* server);
void wm_content_base_destroy(struct wm_content* content);

void wm_content_set_box(struct wm_content* content, double x, double y, double width, double height);
void wm_content_get_box(struct wm_content* content, double* display_x, double* display_y, double* display_width, double* display_height);

void wm_content_set_corner_radius(struct wm_content* content, double corner_radius);
double wm_content_get_corner_radius(struct wm_content* content);

void wm_content_set_z_index(struct wm_content* content, int z_index);
int wm_content_get_z_index(struct wm_content* content);

void wm_content_set_opacity(struct wm_content* content, double opacity);
double wm_content_get_opacity(struct wm_content* content);

void wm_content_set_lock_enabled(struct wm_content* content, bool lock_enabled);

struct wm_content_vtable {
    void (*destroy)(struct wm_content* content);
    void (*render)(struct wm_content* content, struct wm_output* output, pixman_region32_t* output_damage, struct timespec now);

    /* origin == NULL means damage whole */
    void (*damage_output)(struct wm_content* content, struct wm_output* output, struct wlr_surface* origin);
};

static inline void wm_content_destroy(struct wm_content* content){
    (*content->vtable->destroy)(content);
}
static inline void wm_content_render(struct wm_content* content, struct wm_output* output, pixman_region32_t* output_damage, struct timespec now){
    (*content->vtable->render)(content, output, output_damage, now);
}
static inline void wm_content_damage_output(struct wm_content* content, struct wm_output* output, struct wlr_surface* origin){
    (*content->vtable->damage_output)(content, output, origin);
}

#endif
