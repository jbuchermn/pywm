#define _POSIX_C_SOURCE 200112L

#include "wm/wm_output.h"
#include "wm/wm.h"
#include "wm/wm_config.h"
#include "wm/wm_layout.h"
#include "wm/wm_renderer.h"
#include "wm/wm_server.h"
#include "wm/wm_util.h"
#include "wm/wm_view.h"
#include "wm/wm_widget.h"
#include <assert.h>
#include <time.h>
#include <wlr/util/log.h>
#include <wlr/util/region.h>

/*
 * Callbacks
 */
static void handle_destroy(struct wl_listener *listener, void *data) {
    wlr_log(WLR_DEBUG, "Output: Destroy");
    struct wm_output *output = wl_container_of(listener, output, destroy);
    wm_output_destroy(output);
}

static void handle_commit(struct wl_listener *listener, void *data) {
    struct wm_output *output = wl_container_of(listener, output, commit);
}

static void handle_mode(struct wl_listener *listener, void *data) {
    wlr_log(WLR_DEBUG, "Output: Mode");
    struct wm_output *output = wl_container_of(listener, output, mode);
}

static void handle_present(struct wl_listener *listener, void *data) {
    struct wm_output *output = wl_container_of(listener, output, present);
    struct wlr_output_event_present* event = data;

    /* TODO: Find better place */
    /* Synchronous updates */
    wm_callback_update();
    wm_server_update_contents(output->wm_server);
    /* ------------------- */

}

struct render_data {
    struct wm_output *output;
    struct timespec when;
    double x;
    double y;
    double x_scale;
    double y_scale;
    double corner_radius;
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
    wm_renderer_render_texture_at(output->wm_server->wm_renderer, texture, &box,
            corner_radius);

    /* Notify client */
    wlr_surface_send_frame_done(surface, &rdata->when);
}

static void render_widget(struct wm_output *output, struct wm_widget *widget) {
    if (!widget->wlr_texture)
        return;

    struct wlr_box box = {
        .x = round(widget->super.display_x * output->wlr_output->scale),
        .y = round(widget->super.display_y * output->wlr_output->scale),
        .width = round(widget->super.display_width * output->wlr_output->scale),
        .height =
            round(widget->super.display_height * output->wlr_output->scale)};
    double corner_radius =
        wm_content_get_corner_radius(&widget->super) * output->wlr_output->scale;

    wm_renderer_render_texture_at(output->wm_server->wm_renderer,
            widget->wlr_texture, &box, corner_radius);
}

static void render_view(struct wm_output *output, struct wm_view *view,
        struct timespec now) {

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

    struct render_data rdata = {.output = output,
        .when = now,
        .x = display_x,
        .y = display_y,
        .x_scale = display_width / width,
        .y_scale = display_height / height,
        .corner_radius = corner_radius};

    wm_view_for_each_surface(view, render_surface, &rdata);
}

static void render(struct wm_output *output, struct timespec now, pixman_region32_t *damage) {
    struct wm_renderer *renderer = output->wm_server->wm_renderer;

    /* Begin render */
    wm_renderer_begin(renderer, output);

    /* Do render */
    struct wm_content *r;
    wl_list_for_each_reverse(r, &output->wm_server->wm_contents, link) {
        if (wm_content_is_view(r)) {
            render_view(output, wm_cast(wm_view, r), now);
        } else {
            render_widget(output, wm_cast(wm_widget, r));
        }
    }

    wlr_output_render_software_cursors(output->wlr_output, damage);

    /* End render */
    wm_renderer_end(renderer, output);

    /* Commit */
    int width, height;
    wlr_output_transformed_resolution(output->wlr_output, &width, &height);

    pixman_region32_t frame_damage;
    pixman_region32_init(&frame_damage);

    enum wl_output_transform transform =
        wlr_output_transform_invert(output->wlr_output->transform);
    wlr_region_transform(&frame_damage, &output->wlr_output_damage->current,
            transform, width, height);

    wlr_output_set_damage(output->wlr_output, &frame_damage);
    pixman_region32_fini(&frame_damage);

    if (!wlr_output_commit(output->wlr_output)) {
        wlr_log(WLR_DEBUG, "Commit frame failed");
    }
}

static void handle_damage_frame(struct wl_listener *listener, void *data) {
    struct wm_output *output = wl_container_of(listener, output, damage_frame);

    bool needs_frame;
    pixman_region32_t damage;
    pixman_region32_init(&damage);
    if (wlr_output_damage_attach_render(
                output->wlr_output_damage, &needs_frame, &damage)) {

        if (needs_frame) {
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);

            TIMER_START(render)
            render(output, now, &damage);
            TIMER_STOP(render);
            TIMER_PRINT(render);
        } else {
            wlr_output_rollback(output->wlr_output);
        }

        pixman_region32_fini(&damage);
    }


}

static void handle_damage_destroy(struct wl_listener *listener, void *data) {
    struct wm_output *output = wl_container_of(listener, output, damage_destroy);

    wl_list_remove(&output->damage_frame.link);
    wl_list_remove(&output->damage_destroy.link);
}

/*
 * Class implementation
 */
void wm_output_init(struct wm_output *output, struct wm_server *server,
        struct wm_layout *layout, struct wlr_output *out) {
    wlr_log(WLR_DEBUG, "New output");
    output->wm_server = server;
    output->wm_layout = layout;
    output->wlr_output = out;

    output->wlr_output_damage = wlr_output_damage_create(output->wlr_output);

    /* Set mode */
    if (!wl_list_empty(&output->wlr_output->modes)) {
        struct wlr_output_mode *mode =
            wlr_output_preferred_mode(output->wlr_output);
        wlr_output_set_mode(output->wlr_output, mode);

        wlr_output_enable(output->wlr_output, true);
        if (!wlr_output_commit(output->wlr_output)) {
            wlr_log(WLR_INFO, "New output: Could not commit");
        }
    }

    /* Set HiDPI scale */
    wlr_output_set_scale(output->wlr_output,
            output->wm_server->wm_config->output_scale);

    output->destroy.notify = handle_destroy;
    wl_signal_add(&output->wlr_output->events.destroy, &output->destroy);

    output->commit.notify = handle_commit;
    wl_signal_add(&output->wlr_output->events.commit, &output->commit);

    output->mode.notify = handle_mode;
    wl_signal_add(&output->wlr_output->events.mode, &output->mode);

    output->present.notify = handle_present;
    wl_signal_add(&output->wlr_output->events.present, &output->present);

    output->damage_frame.notify = handle_damage_frame;
    wl_signal_add(&output->wlr_output_damage->events.frame,
            &output->damage_frame);

    output->damage_destroy.notify = handle_damage_destroy;
    wl_signal_add(&output->wlr_output_damage->events.destroy,
            &output->damage_destroy);
}

void wm_output_destroy(struct wm_output *output) {
    wl_list_remove(&output->destroy.link);
    wl_list_remove(&output->commit.link);
    wl_list_remove(&output->mode.link);
    wl_list_remove(&output->present.link);
    wl_list_remove(&output->link);
}
