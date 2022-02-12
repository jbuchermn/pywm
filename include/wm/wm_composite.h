#ifndef WM_COMPOSITE_H
#define WM_COMPOSITE_H

#include <stdbool.h>
#include <wayland-server.h>
#include <pixman.h>

#include "wm_content.h"

struct wm_server;

enum wm_composite_type {
    WM_COMPOSITE_BLUR
};

struct wm_composite {
    struct wm_content super;
    enum wm_composite_type type;

    struct {
        int n_params_int;
        int* params_int;
        int n_params_float;
        float* params_float;
    } params;

};

void wm_composite_init(struct wm_composite* comp, struct wm_server* server);
void wm_composite_set_type(struct wm_composite* comp, const char* type, int n_params_int, int* params_int, int n_params_float, float* params_float);

void wm_composite_on_damage_below(struct wm_composite* comp, struct wm_output* output, struct wm_content* from, pixman_region32_t* damage);
bool wm_content_is_composite(struct wm_content* content);
void wm_composite_apply(struct wm_composite* composite, struct wm_output* output, pixman_region32_t* damage, unsigned int from_buffer, struct timespec now);

struct wm_compose_tree {
    struct wl_list link; // wm_compose_tree::children
    struct wm_compose_tree* parent;

    pixman_region32_t output;
    pixman_region32_t leaf_input;
    struct wm_composite* composite;

    struct wl_list children; 
};

struct wm_compose_tree* wm_compose_tree_from_damage(struct wm_server* server, struct wm_output* output, pixman_region32_t* damage, bool bypass);
void wm_compose_tree_destroy(struct wm_compose_tree* tree);


#endif
