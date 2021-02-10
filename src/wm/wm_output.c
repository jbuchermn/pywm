#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <time.h>
#include <wlr/util/log.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_matrix.h>
#include "wm/wm_server.h"
#include "wm/wm_output.h"
#include "wm/wm_view.h"
#include "wm/wm_layout.h"
#include "wm/wm_widget.h"
#include "wm/wm_config.h"
#include "wm/wm.h"

/*
 * Callbacks
 */
static void handle_destroy(struct wl_listener* listener, void* data){
    wlr_log(WLR_DEBUG, "Output: Destroy");
    struct wm_output* output = wl_container_of(listener, output, destroy);
    wm_output_destroy(output);
}

static void handle_mode(struct wl_listener* listener, void* data){
    wlr_log(WLR_DEBUG, "Output: Mode");
    struct wm_output* output = wl_container_of(listener, output, mode);
}

static void handle_transform(struct wl_listener* listener, void* data){
    struct wm_output* output = wl_container_of(listener, output, transform);
}

static void handle_present(struct wl_listener* listener, void* data){
    struct wm_output* output = wl_container_of(listener, output, present);
}

struct render_data {
	struct wm_output *output;
	struct timespec when;
    double x;
    double y;
    double x_scale;
    double y_scale;
};

/* static void render_decoration(struct wm_view* view, struct render_data* rdata){ */
/*     int _width, _height; */
/*     wm_view_get_size(view, &_width, &_height); */
/*     double width = _width * rdata->x_scale; */
/*     double height = _height * rdata->y_scale; */
/*  */
/*     { */
/*         double x = rdata->x - 2; */
/*         double y = rdata->y - 8; */
/*         double w = width + 4; */
/*         double h = 8; */
/*         float color[] = {0.0, 0.0, 0.0, 0.4}; */
/*  */
/*         struct wlr_box box = { */
/*             .x = round(x * rdata->output->wlr_output->scale), */
/*             .y = round(y * rdata->output->wlr_output->scale), */
/*             .width = round(w * rdata->output->wlr_output->scale), */
/*             .height = round(h * rdata->output->wlr_output->scale) */
/*         }; */
/*         wlr_render_rect( */
/*                 rdata->output->wm_server->wlr_renderer, */
/*                 &box, */
/*                 color, */
/*                 rdata->output->wlr_output->transform_matrix); */
/*     } */
/*     { */
/*         double x = rdata->x - 2; */
/*         double y = rdata->y; */
/*         double w = 2; */
/*         double h = height; */
/*         float color[] = {0.0, 0.0, 0.0, 0.4}; */
/*  */
/*         struct wlr_box box = { */
/*             .x = round(x * rdata->output->wlr_output->scale), */
/*             .y = round(y * rdata->output->wlr_output->scale), */
/*             .width = round(w * rdata->output->wlr_output->scale), */
/*             .height = round(h * rdata->output->wlr_output->scale) */
/*         }; */
/*         wlr_render_rect( */
/*                 rdata->output->wm_server->wlr_renderer, */
/*                 &box, */
/*                 color, */
/*                 rdata->output->wlr_output->transform_matrix); */
/*     } */
/*     { */
/*         double x = rdata->x + width; */
/*         double y = rdata->y; */
/*         double w = 2; */
/*         double h = height; */
/*         float color[] = {0.0, 0.0, 0.0, 0.4}; */
/*  */
/*         struct wlr_box box = { */
/*             .x = round(x * rdata->output->wlr_output->scale), */
/*             .y = round(y * rdata->output->wlr_output->scale), */
/*             .width = round(w * rdata->output->wlr_output->scale), */
/*             .height = round(h * rdata->output->wlr_output->scale) */
/*         }; */
/*         wlr_render_rect( */
/*                 rdata->output->wm_server->wlr_renderer, */
/*                 &box, */
/*                 color, */
/*                 rdata->output->wlr_output->transform_matrix); */
/*     } */
/*     { */
/*         double x = rdata->x - 2; */
/*         double y = rdata->y + height; */
/*         double w = width + 4; */
/*         double h = 2; */
/*         float color[] = {0.0, 0.0, 0.0, 0.4}; */
/*  */
/*         struct wlr_box box = { */
/*             .x = round(x * rdata->output->wlr_output->scale), */
/*             .y = round(y * rdata->output->wlr_output->scale), */
/*             .width = round(w * rdata->output->wlr_output->scale), */
/*             .height = round(h * rdata->output->wlr_output->scale) */
/*         }; */
/*         wlr_render_rect( */
/*                 rdata->output->wm_server->wlr_renderer, */
/*                 &box, */
/*                 color, */
/*                 rdata->output->wlr_output->transform_matrix); */
/*     } */
/* } */

static void render_surface(struct wlr_surface *surface, int sx, int sy, void *data) {
	struct render_data *rdata = data;
	struct wm_output *output = rdata->output;


	struct wlr_texture *texture = wlr_surface_get_texture(surface);
	if(!texture) {
		return;
	}

	struct wlr_box box = {
		.x = round((rdata->x + sx*rdata->x_scale) * output->wlr_output->scale),
		.y = round((rdata->y + sy*rdata->y_scale) * output->wlr_output->scale),
		.width = round(surface->current.width * rdata->x_scale * output->wlr_output->scale),
		.height = round(surface->current.height * rdata->y_scale * output->wlr_output->scale)
	};

	float matrix[9];
	enum wl_output_transform transform = wlr_output_transform_invert(surface->current.transform);
	wlr_matrix_project_box(matrix, &box, transform, 0, output->wlr_output->transform_matrix);

    /* Actual rendering */
	wlr_render_texture_with_matrix(output->wm_server->wlr_renderer, texture, matrix, 1);

    /* Notify client */
	wlr_surface_send_frame_done(surface, &rdata->when);
}

static void render_widget(struct wm_output* output, struct wm_widget* widget){
    if(!widget->wlr_texture) return;

    struct wlr_box box = {
        .x = round(widget->display_x * output->wlr_output->scale),
        .y = round(widget->display_y * output->wlr_output->scale),
        .width = round(widget->display_width * output->wlr_output->scale),
        .height = round(widget->display_height * output->wlr_output->scale)
    };

    float matrix[9];
    wlr_matrix_project_box(matrix, &box, WL_OUTPUT_TRANSFORM_NORMAL, 0, output->wlr_output->transform_matrix);
    wlr_render_texture_with_matrix(output->wm_server->wlr_renderer, widget->wlr_texture, matrix, 1.);

}

static void render_view(struct wm_output* output, struct wm_view* view, struct timespec now){

    if(!view->mapped){
        return;
    }

    int width, height;
    wm_view_get_size(view, &width, &height);

    if(width<=0 || height<=0){
        return;
    }

    double display_x, display_y, display_width, display_height;
    wm_view_get_box(view, &display_x, &display_y, &display_width, &display_height);

    struct render_data rdata = {
        .output = output,
        .when = now,
        .x = display_x,
        .y = display_y,
        .x_scale = display_width / width,
        .y_scale = display_height / height
    };

    /* render_decoration(view, &rdata); */
    wm_view_for_each_surface(view, render_surface, &rdata);
}

static void handle_frame(struct wl_listener* listener, void* data){
    struct wm_output* output = wl_container_of(listener, output, frame);
    struct wlr_renderer* wlr_renderer = output->wm_server->wlr_renderer;

	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);

    /* Synchronous updates */
    wm_callback_update();

	if(!wlr_output_attach_render(output->wlr_output, NULL)) {
		return;
	}

	wlr_renderer_begin(wlr_renderer, output->wlr_output->width, output->wlr_output->height);

	float color[4] = { 0.3, 0.3, 0.3, 1.0 };
	wlr_renderer_clear(wlr_renderer, color);

    /* Find relevant z_indices */
    int z_index_min = 0;
    int z_index_max = 0;

    struct wm_widget* widget;
    wl_list_for_each_reverse(widget, &output->wm_server->wm_widgets, link){
        if(widget->z_index<z_index_min) z_index_min = widget->z_index;
        if(widget->z_index>z_index_max) z_index_max = widget->z_index;
    }

    struct wm_view* view;
    wl_list_for_each_reverse(view, &output->wm_server->wm_views, link){
        if(view->z_index<z_index_min) z_index_min = view->z_index;
        if(view->z_index>z_index_max) z_index_max = view->z_index;
    }

    /* Limit to -100 - 100 due to inefficient implementation */
    if(z_index_min < -100) z_index_min = -100;
    if(z_index_max > 100) z_index_max = 100;

    /* Render z_indices */
    for(int z_index = z_index_min; z_index<=z_index_max; z_index++){
        /* Render views */
        struct wm_view *view;
        wl_list_for_each_reverse(view, &output->wm_server->wm_views, link) {
            if(view->z_index != z_index) continue;
            render_view(output, view, now);

        }

        /* Render widgets */
        struct wm_widget* widget;
        wl_list_for_each_reverse(widget, &output->wm_server->wm_widgets, link){
            if(widget->z_index != z_index) continue;
            render_widget(output, widget);
        }
    }


	wlr_renderer_end(wlr_renderer);
	if(!wlr_output_commit(output->wlr_output)){
        wlr_log(WLR_INFO, "Output commit failed, manually scheduling frame");
        wlr_output_schedule_frame(output->wlr_output);
    }
}

/*
 * Class implementation
 */
void wm_output_init(struct wm_output* output, struct wm_server* server, struct wm_layout* layout, struct wlr_output* out){
	wlr_log(WLR_DEBUG, "New output");
    output->wm_server = server;
    output->wm_layout = layout;
    output->wlr_output = out;

    /* Set mode */
    if(!wl_list_empty(&output->wlr_output->modes)){
		struct wlr_output_mode *mode = wlr_output_preferred_mode(output->wlr_output);
		wlr_output_set_mode(output->wlr_output, mode);

		wlr_output_enable(output->wlr_output, true);
		if (!wlr_output_commit(output->wlr_output)) {
			wlr_log(WLR_INFO, "New output: Could not commit");
		}
    }

    /* Set HiDPI scale */
    wlr_output_set_scale(output->wlr_output, output->wm_server->wm_config->output_scale);

    output->destroy.notify = handle_destroy;
    wl_signal_add(&output->wlr_output->events.destroy, &output->destroy);

    output->mode.notify = handle_mode;
    wl_signal_add(&output->wlr_output->events.mode, &output->mode);

    output->transform.notify = handle_transform;
    wl_signal_add(&output->wlr_output->events.transform, &output->transform);

    output->present.notify = handle_present;
    wl_signal_add(&output->wlr_output->events.present, &output->present);

    output->frame.notify = handle_frame;
    wl_signal_add(&output->wlr_output->events.frame, &output->frame);
}

void wm_output_destroy(struct wm_output* output){
    wl_list_remove(&output->destroy.link);
    wl_list_remove(&output->mode.link);
    wl_list_remove(&output->transform.link);
    wl_list_remove(&output->present.link);
    wl_list_remove(&output->link);
}
