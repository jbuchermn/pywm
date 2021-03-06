#define _POSIX_C_SOURCE 200112L

#include "wm/wm_output.h"
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
#include <wlr/types/wlr_matrix.h>

/* #define DEBUG_DAMAGE */

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

    /* 
     * Synchronous update is best scheduled immediately after
     * frame present
     */
    wm_server_callback_update(output->wm_server);
}

static void render(struct wm_output *output, struct timespec now, pixman_region32_t *damage) {
    struct wm_renderer *renderer = output->wm_server->wm_renderer;

    /* Ensure z-index */
    wm_server_update_contents(output->wm_server);

    /* Begin render */
    wm_renderer_begin(renderer, output);

#ifdef DEBUG_DAMAGE
    wlr_renderer_clear(renderer->wlr_renderer, (float[]){1, 1, 0, 1});
#endif

    bool needs_clear = false;
    struct wm_content *r;
    wl_list_for_each_reverse(r, &output->wm_server->wm_contents, link) {
        if(wm_content_get_opacity(r) < 1. - 0.0001){
            needs_clear=true;
            break;
        }
    }

    if(needs_clear){
        int nrects;
        pixman_box32_t* rects = pixman_region32_rectangles(damage, &nrects);
        for(int i=0; i<nrects; i++){
            struct wlr_box damage_box = {
                .x = rects[i].x1,
                .y = rects[i].y1,
                .width = rects[i].x2 - rects[i].x1,
                .height = rects[i].y2 - rects[i].y1
            };

            float matrix[9];
            wlr_matrix_project_box(matrix, &damage_box,
                    WL_OUTPUT_TRANSFORM_NORMAL, 0,
                    renderer->current->wlr_output->transform_matrix);
            wlr_render_rect(
                    renderer->wlr_renderer,
                    &damage_box, (float[]){0., 0., 0., 1.}, renderer->current->wlr_output->transform_matrix);
        }
    }

    /* Do render */
    if(output == output->wm_server->wm_layout->default_output){
        struct wm_content *r;
        wl_list_for_each_reverse(r, &output->wm_server->wm_contents, link) {
            if(wm_content_get_opacity(r) < 0.0001) continue;
            wm_content_render(r, output, damage, now);
        }
    }else{
        wlr_renderer_clear(renderer->wlr_renderer, (float[]){.3, .3, .3, 1});
    }

    /* End render */
    wm_renderer_end(renderer, damage, output);

    /* Commit */
#ifdef DEBUG_DAMAGE
    pixman_region32_t frame_damage;
    pixman_region32_init(&frame_damage);
    pixman_region32_union_rect(&frame_damage, &frame_damage,
        0, 0, output->wlr_output->width, output->wlr_output->height);
    wlr_output_set_damage(
            output->wlr_output, &output->wlr_output_damage->current);
    pixman_region32_fini(&frame_damage);
#else
    wlr_output_set_damage(
            output->wlr_output, &output->wlr_output_damage->current);
#endif


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
