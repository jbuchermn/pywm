#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <stdlib.h>
#include <wayland-server.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <wlr/util/region.h>
#include <wlr/xwayland.h>

#include "wm/wm_view.h"
#include "wm/wm_seat.h"
#include "wm/wm_output.h"
#include "wm/wm_renderer.h"
#include "wm/wm_server.h"
#include "wm/wm.h"

#include "wm/wm_util.h"

struct wm_content_vtable wm_view_vtable;

void wm_view_base_init(struct wm_view* view, struct wm_server* server){
    wm_content_init(&view->super, server);

    view->super.vtable = &wm_view_vtable;

    /* Abstract class */
    view->vtable = NULL;

    view->mapped = false;
    view->accepts_input = true;
}

static void wm_view_base_destroy(struct wm_content* super){
    struct wm_view* view = wm_cast(wm_view, super);

    (view->vtable->destroy)(view);
    wm_content_base_destroy(super);
}

int wm_content_is_view(struct wm_content* content){
    return content->vtable == &wm_view_vtable;
}

struct render_data {
    struct wm_output *output;
    pixman_region32_t* damage;
    struct timespec when;
    double x;
    double y;
    double x_scale;
    double y_scale;
    double opacity;
    double corner_radius;
    double lock_perc;
};


static void render_surface(struct wlr_surface *surface, int sx, int sy,
        void *data) {
    struct render_data *rdata = data;
    struct wm_output *output = rdata->output;

    struct wlr_texture *texture = wlr_surface_get_texture(surface);
    if (!texture) {
        return;
    }

    struct wlr_box box = {
        .x = round((rdata->x + sx * rdata->x_scale) * output->wlr_output->scale),
        .y = round((rdata->y + sy * rdata->y_scale) * output->wlr_output->scale),
        .width = round(surface->current.width * rdata->x_scale *
                output->wlr_output->scale),
        .height = round(surface->current.height * rdata->y_scale *
                output->wlr_output->scale)};

    double corner_radius = rdata->corner_radius * output->wlr_output->scale;
    if (sx || sy) {
        /* Only for surfaces which extend fully */
        corner_radius = 0;
    }
    wm_renderer_render_texture_at(output->wm_server->wm_renderer, rdata->damage, texture, &box,
            rdata->opacity, corner_radius, rdata->lock_perc);

    /* Notify client */
    wlr_surface_send_frame_done(surface, &rdata->when);
}


static void wm_view_render(struct wm_content* super, struct wm_output* output, pixman_region32_t* output_damage, struct timespec now){
    struct wm_view* view = wm_cast(wm_view, super);

    if (!view->mapped) {
        return;
    }

    int width, height;
    wm_view_get_size(view, &width, &height);

    if (width <= 0 || height <= 0) {
        return;
    }

    double display_x, display_y, display_width, display_height;
    wm_content_get_box(&view->super, &display_x, &display_y, &display_width,
            &display_height);
    double corner_radius = wm_content_get_corner_radius(&view->super);

    struct render_data rdata = {
        .output = output,
        .when = now,
        .damage = output_damage,
        .x = display_x,
        .y = display_y,
        .opacity = wm_content_get_opacity(&view->super),
        .x_scale = display_width / width,
        .y_scale = display_height / height,
        .corner_radius = corner_radius,
        .lock_perc = view->super.lock_enabled ? 0.0 : view->super.wm_server->lock_perc
    };

    wm_view_for_each_surface(view, render_surface, &rdata);
}


struct damage_data {
    struct wm_output *output;
    double x;
    double y;
    double x_scale;
    double y_scale;
    struct wlr_surface* origin;
};

static void damage_surface(struct wlr_surface *surface, int sx, int sy,
        void *data) {
    struct damage_data *ddata = data;
    struct wm_output *output = ddata->output;

    if(ddata->origin && ddata->origin != surface) return;

    struct wlr_box box = {
        .x = round((ddata->x + sx * ddata->x_scale) * output->wlr_output->scale),
        .y = round((ddata->y + sy * ddata->y_scale) * output->wlr_output->scale),
        .width = round(surface->current.width * ddata->x_scale *
                output->wlr_output->scale),
        .height = round(surface->current.height * ddata->y_scale *
                output->wlr_output->scale)};

    if(!ddata->origin){
        pixman_region32_t region;
        pixman_region32_init(&region);

        pixman_region32_union_rect(&region, &region,
                box.x, box.y, box.width, box.height);

        wlr_output_damage_add(output->wlr_output_damage, &region);
        pixman_region32_fini(&region);

    }else

	if (pixman_region32_not_empty(&surface->buffer_damage)) {
		pixman_region32_t region;
		pixman_region32_init(&region);

		wlr_surface_get_effective_damage(surface, &region);

        wlr_region_scale_xy(&region, &region, 
                ddata->x_scale * output->wlr_output->scale,
                ddata->y_scale * output->wlr_output->scale);

		pixman_region32_translate(&region, box.x, box.y);


		wlr_output_damage_add(output->wlr_output_damage, &region);
		pixman_region32_fini(&region);
	}

}

static void wm_view_damage_output(struct wm_content* super, struct wm_output* output, struct wlr_surface* origin){
    struct wm_view* view = wm_cast(wm_view, super);

    int width, height;
    wm_view_get_size(view, &width, &height);

    if (width <= 0 || height <= 0) {
        return;
    }

    double display_x, display_y, display_width, display_height;
    wm_content_get_box(&view->super, &display_x, &display_y, &display_width,
            &display_height);

    struct damage_data ddata = {
        .output = output,
        .x = display_x,
        .y = display_y,
        .x_scale = display_width / width,
        .y_scale = display_height / height,
        .origin = origin
    };

    wm_view_for_each_surface(view, damage_surface, &ddata);
}


struct wm_content_vtable wm_view_vtable = {
    .destroy = &wm_view_base_destroy,
    .render = &wm_view_render,
    .damage_output = &wm_view_damage_output,
};
