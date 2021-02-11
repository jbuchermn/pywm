#ifndef WM_RENDERER_H
#define WM_RENDERER_H

#include <wlr/render/wlr_renderer.h>

struct wm_output;

struct wm_renderer {
    struct wm_server* wm_server;
    struct wlr_renderer* wlr_renderer;

    struct wm_output* current;
};

void wm_renderer_init(struct wm_renderer* renderer, struct wm_server* server);
void wm_renderer_destroy(struct wm_renderer* renderer);

void wm_renderer_begin(struct wm_renderer* renderer, struct wm_output* output);
void wm_renderer_end(struct wm_renderer* renderer, struct wm_output* output);
void wm_renderer_render_texture_at(struct wm_renderer* renderer, struct wlr_texture* texture, struct wlr_box* box);

#endif
