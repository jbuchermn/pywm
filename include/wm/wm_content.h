#ifndef WM_CONTENT_H
#define WM_CONTENT_H

#include <stdio.h>
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

    /* Fix to one output - only for comparison; don't dereference */
    struct wm_output* fixed_output;

    /* Only consider the parts of content inside this box valid (disabled if width or height < 0) */
    double workspace_x;
    double workspace_y;
    double workspace_width;
    double workspace_height;

    /* Only applied to root surface(s) */
    double mask_x;
    double mask_y;
    double mask_w;
    double mask_h;
    double corner_radius;

    int z_index;
    double opacity;

    /* Accepts input and is displayed clearly during lock - careful */
    bool lock_enabled;
};

void wm_content_init(struct wm_content* content, struct wm_server* server);
void wm_content_base_destroy(struct wm_content* content);

void wm_content_set_output(struct wm_content* content, char* name);
void wm_content_set_workspace(struct wm_content* content, double x, double y, double width, double height);
void wm_content_get_workspace(struct wm_content* content, double* workspace_x, double* workspace_y, double* workspace_width, double* workspace_height);
bool wm_content_has_workspace(struct wm_content* content);

void wm_content_set_box(struct wm_content* content, double x, double y, double width, double height);
void wm_content_get_box(struct wm_content* content, double* display_x, double* display_y, double* display_width, double* display_height);

void wm_content_set_mask(struct wm_content* content, double mask_x, double mask_y, double mask_w, double mask_h);
void wm_content_get_mask(struct wm_content* content, double* mask_x, double* mask_y, double* mask_w, double* mask_h);
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

    void (*printf)(FILE* file, struct wm_content* content);
};

static inline void wm_content_destroy(struct wm_content* content){
    (*content->vtable->destroy)(content);
}

void wm_content_render(struct wm_content* content, struct wm_output* output, pixman_region32_t* output_damage, struct timespec now);

static inline void wm_content_damage_output(struct wm_content* content, struct wm_output* output, struct wlr_surface* origin){
    (*content->vtable->damage_output)(content, output, origin);
}
static inline void wm_content_printf(FILE* file, struct wm_content* content){
    (*content->vtable->printf)(file, content);
}

#endif
