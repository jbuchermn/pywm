#ifndef WM_COMPOSITE_H
#define WM_COMPOSITE_H

#include <stdbool.h>
#include <wayland-server.h>

#include "wm_content.h"

struct wm_server;

enum wm_composite_type {
    WM_COMPOSITE_BLUR
};

struct wm_composite {
    struct wm_content super;
    enum wm_composite_type type;
};

void wm_composite_init(struct wm_composite* comp, struct wm_server* server);
void wm_composite_set_type(struct wm_composite* comp, enum wm_composite_type type);

void wm_composite_on_damage_below(struct wm_composite* comp, struct wm_output* output, struct wm_content* from, pixman_region32_t* damage);

bool wm_content_is_composite(struct wm_content* content);


#endif
