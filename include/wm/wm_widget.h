#ifndef WM_WIDGET_H
#define WM_WIDGET_H

#include <stdbool.h>
#include <wayland-server.h>
#include <wlr/render/wlr_texture.h>

#include "wm_content.h"

struct wm_server;

struct wm_widget {
    struct wm_content super;

    struct wlr_texture* wlr_texture;
};

void wm_widget_init(struct wm_widget* widget, struct wm_server* server);

void wm_widget_set_pixels(struct wm_widget* widget, uint32_t format, uint32_t stride, uint32_t width, uint32_t height, const void* data);

#endif
