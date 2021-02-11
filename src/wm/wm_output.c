#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <time.h>
#include <wlr/util/log.h>
#include "wm/wm_server.h"
#include "wm/wm_output.h"
#include "wm/wm_view.h"
#include "wm/wm_layout.h"
#include "wm/wm_widget.h"
#include "wm/wm_config.h"
#include "wm/wm_renderer.h"
#include "wm/wm.h"
#include "wm/wm_util.h"

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

    wm_renderer_render_texture_at(output->wm_server->wm_renderer, texture, &box);

    /* Notify client */
	wlr_surface_send_frame_done(surface, &rdata->when);
}

static void render_widget(struct wm_output* output, struct wm_widget* widget){
    if(!widget->wlr_texture) return;

    struct wlr_box box = {
        .x = round(widget->super.display_x * output->wlr_output->scale),
        .y = round(widget->super.display_y * output->wlr_output->scale),
        .width = round(widget->super.display_width * output->wlr_output->scale),
        .height = round(widget->super.display_height * output->wlr_output->scale)
    };

    wm_renderer_render_texture_at(output->wm_server->wm_renderer, widget->wlr_texture, &box);

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
    wm_content_get_box(&view->super, &display_x, &display_y, &display_width, &display_height);

    struct render_data rdata = {
        .output = output,
        .when = now,
        .x = display_x,
        .y = display_y,
        .x_scale = display_width / width,
        .y_scale = display_height / height
    };

    wm_view_for_each_surface(view, render_surface, &rdata);
}

static void handle_frame(struct wl_listener* listener, void* data){
    struct wm_output* output = wl_container_of(listener, output, frame);
    struct wm_renderer* renderer = output->wm_server->wm_renderer;

	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);

    /* Synchronous updates */
    wm_callback_update();
    wm_server_update_contents(output->wm_server);
    /* ------------------- */

	if(!wlr_output_attach_render(output->wlr_output, NULL)) {
		return;
	}

    wm_renderer_begin(renderer, output);

    /* Render views and widgets */
    struct wm_content *r;
    wl_list_for_each_reverse(r, &output->wm_server->wm_contents, link) {
        if(wm_content_is_view(r)){
            render_view(output, wm_cast(wm_view, r), now);
        }else{
            render_widget(output, wm_cast(wm_widget, r));
        }

    }


    wm_renderer_end(renderer, output);
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
