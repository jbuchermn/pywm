#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <stdlib.h>
#include <wayland-server.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_matrix.h>

#include "wm/wm_server.h"
#include "wm/wm_renderer.h"
#include "wm/wm_output.h"

void wm_renderer_init(struct wm_renderer* renderer, struct wm_server* server){
    renderer->wm_server = server;

    renderer->wlr_renderer = wlr_backend_get_renderer(server->wlr_backend);
    assert(renderer->wlr_renderer);

    wlr_renderer_init_wl_display(renderer->wlr_renderer, server->wl_display);

    renderer->current = NULL;
}

void wm_renderer_destroy(struct wm_renderer* renderer){
    wlr_renderer_destroy(renderer->wlr_renderer);
}

void wm_renderer_begin(struct wm_renderer* renderer, struct wm_output* output){
	wlr_renderer_begin(renderer->wlr_renderer, output->wlr_output->width, output->wlr_output->height);

	float color[4] = { 0.3, 0.3, 0.3, 1.0 };
	wlr_renderer_clear(renderer->wlr_renderer, color);

    renderer->current = output;
}

void wm_renderer_end(struct wm_renderer* renderer, struct wm_output* output){
	wlr_renderer_end(renderer->wlr_renderer);

    renderer->current = NULL;
}



void wm_renderer_render_texture_at(struct wm_renderer* renderer, struct wlr_texture* texture, struct wlr_box* box){

    float matrix[9];
    wlr_matrix_project_box(matrix, box, WL_OUTPUT_TRANSFORM_NORMAL, 0, renderer->current->wlr_output->transform_matrix);

	struct wlr_fbox fbox = {
		.x = 0,
		.y = 0,
		.width = texture->width,
		.height = texture->height,
	};

    wlr_render_subtexture_with_matrix(
            renderer->wlr_renderer,
            texture,
            &fbox, matrix, 1.);
}
