#define _POSIX_C_SOURCE 200809L

#include "wm/wm_output.h"
#include "wm/wm_config.h"
#include "wm/wm_layout.h"
#include "wm/wm_renderer.h"
#include "wm/wm_server.h"
#include "wm/wm_util.h"
#include "wm/wm_view.h"
#include "wm/wm_widget.h"
#include "wm/wm_seat.h"
#include "wm/wm_cursor.h"
#include "wm/wm_composite.h"
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <wlr/util/log.h>
#include <wlr/util/region.h>
#include <wlr/types/wlr_matrix.h>

/* #define DEBUG_DAMAGE_HIGHLIGHT */
/* #define DEBUG_DAMAGE_RERENDER */

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
}


static void render(struct wm_output *output, struct timespec now, pixman_region32_t *damage) {
    struct wm_renderer *renderer = output->wm_server->wm_renderer;

    int width, height;
    wlr_output_transformed_resolution(output->wlr_output, &width, &height);

    /* Ensure z-indes */
    wm_server_update_contents(output->wm_server);


    /* Begin render */
    wm_renderer_begin(renderer, output);

#ifdef DEBUG_DAMAGE_HIGHLIGHT
    wlr_renderer_clear(renderer->wlr_renderer, (float[]){1, 1, 0, 1});
#endif

    /* 
     * This does not catch all cases, where clearing is necessary - specifically, if only the texture contains transparency,
     * but compositor opacaity is set to 1, needs_clear will be false.
     *
     * In the end the assumption is there's always a background and this catches a fading out background */
    bool needs_clear = false;
    struct wm_content *r;
    wl_list_for_each_reverse(r, &output->wm_server->wm_contents, link) {
        if(wm_content_get_opacity(r) < 1. - 0.0001){
            needs_clear=true;
            break;
        }
    }

    /*
     * TODO:
     * - Select indirect based on damage and wm_composites,
     * - If true extend damage to render_damage based on wm_composite extends
     * - Possibly extend damage based on z-index
     */
    bool indirect = true;
    pixman_region32_t* render_damage = damage;

    if(needs_clear){
        wm_renderer_to_buffer(renderer, 0);
        wm_renderer_clear(renderer, damage, (float[]){ 0., 0., 0., 1.});

        if(indirect){
            wm_renderer_to_buffer(renderer, 1);
            wm_renderer_clear(renderer, render_damage, (float[]){ 0., 0., 0., 1.});
        }
    }

    if(indirect){
        wm_renderer_to_buffer(renderer, 1);
    }

    /* Do render */
    wl_list_for_each_reverse(r, &output->wm_server->wm_contents, link) {
        if(wm_content_get_opacity(r) < 0.0001) continue;
        if(wm_content_is_composite(r)){
            wm_composite_apply(wm_cast(wm_composite, r), output, render_damage, now);
        }else{
            wm_content_render(r, output, render_damage, now);
        }
    }

    if(indirect){
        wm_renderer_to_buffer(renderer, 1);
    }

    /* End render */
    wm_renderer_end(renderer, damage, output);

    /* Commit */
    pixman_region32_t frame_damage;
    pixman_region32_init(&frame_damage);

    enum wl_output_transform transform =
        wlr_output_transform_invert(output->wlr_output->transform);
    wlr_region_transform(&frame_damage, &output->wlr_output_damage->current, transform,
                         width, height);

#ifdef DEBUG_DAMAGE_HIGHLIGHT
    pixman_region32_union_rect(&frame_damage, &frame_damage, 0, 0, width, height);
#endif

    wlr_output_set_damage(output->wlr_output, &frame_damage);
    pixman_region32_fini(&frame_damage);

    if (!wlr_output_commit(output->wlr_output)) {
        wlr_log(WLR_DEBUG, "Commit frame failed");
    }

    /* 
     * Synchronous update is best scheduled immediately after frame
     */
    DEBUG_PERFORMANCE(present_frame, output->key);
    wm_server_schedule_update(output->wm_server, output);
}

static void handle_damage_frame(struct wl_listener *listener, void *data) {
    struct wm_output *output = wl_container_of(listener, output, damage_frame);

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    double diff = msec_diff(now, output->last_frame);
    if(output->expecting_frame &&  output->wlr_output->current_mode && diff > 1.5 * 1000000./output->wlr_output->current_mode->refresh){
        wlr_log(WLR_DEBUG, "Output %d dropped frame (%.2fms)", output->key, diff);
    }

    bool needs_frame;
    pixman_region32_t damage;
    pixman_region32_init(&damage);
    if (wlr_output_damage_attach_render(
                output->wlr_output_damage, &needs_frame, &damage)) {
#ifdef DEBUG_DAMAGE_RERENDER
        int width, height;
        wlr_output_transformed_resolution(output->wlr_output, &width, &height);
        pixman_region32_union_rect(&damage, &damage, 0, 0, width, height);
        needs_frame = true;
#endif

        if (needs_frame) {
            DEBUG_PERFORMANCE(render, output->key);
            TIMER_START(render);
            render(output, now, &damage);
            TIMER_STOP(render);
            TIMER_PRINT(render);

            output->expecting_frame = true;
        } else {
            DEBUG_PERFORMANCE(skip_frame, output->key);
            wlr_output_rollback(output->wlr_output);

            output->expecting_frame = false;
        }
        pixman_region32_fini(&damage);

        output->last_frame = now;
    }else{
        wlr_log(WLR_DEBUG, "Attaching to renderer failed");
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

static const char* wm_output_overridden_name = NULL;
void wm_output_override_name(const char* name){
    wm_output_overridden_name = name;
}

static double configure(struct wm_output* output){
    struct wm_config_output* config = wm_config_find_output(output->wm_layout->wm_server->wm_config, output->wlr_output->name);
    double dpi = 0.;

    /* Set mode */
    if (!wl_list_empty(&output->wlr_output->modes)) {
        struct wlr_output_mode *pref =
            wlr_output_preferred_mode(output->wlr_output);
        struct wlr_output_mode *best = NULL;

        struct wlr_output_mode *mode;
        wl_list_for_each(mode, &output->wlr_output->modes, link) {
            wlr_log(WLR_INFO, "Output: Output supports %dx%d(%d) %s",
                    mode->width, mode->height, mode->refresh,
                    mode->preferred ? "(Preferred)" : "");

            if (config) {
                // Sway logic
                if (mode->width == config->width &&
                    mode->height == config->height) {
                    if (mode->refresh == config->mHz) {
                        best = mode;
                        break;
                    }
                    if (best == NULL || mode->refresh > best->refresh) {
                        best = mode;
                    }
                }
            }
        }

        if (!best)
            best = pref;

        dpi = output->wlr_output->phys_width > 0 ? (double)best->width * 25.4 / output->wlr_output->phys_width : 0;
        wlr_log(WLR_INFO, "Output: Setting mode: %dx%d(%d)", best->width, best->height, best->refresh);
        wlr_output_set_mode(output->wlr_output, best);
    }else{
        int w = config ? config->width : 0;
        int h = config ? config->height : 0;
        int mHz = config ? config->mHz : 0;
        if(w <= 0){
            wlr_log(WLR_INFO, "Output: Need to configure width for custom mode - defaulting to 1920");
            w = 1920;
        }
        if(h <= 0){
            wlr_log(WLR_INFO, "Output: Need to configure height for custom mode - defaulting to 1280");
            h = 1280;
        }
        dpi = output->wlr_output->phys_width > 0 ? (double)w * 25.4 / output->wlr_output->phys_width : 0;
        wlr_log(WLR_INFO, "Output: Setting custom mode - %dx%d(%d)", w, h, mHz);
        wlr_output_set_custom_mode(output->wlr_output, w, h, mHz);
    }


    enum wl_output_transform transform = config ? config->transform : WL_OUTPUT_TRANSFORM_NORMAL;
    wlr_output_set_transform(output->wlr_output, transform);

    wlr_output_enable(output->wlr_output, true);
    if (!wlr_output_commit(output->wlr_output)) {
        wlr_log(WLR_INFO, "Output: Could not commit");
    }

    /* Set HiDPI scale */
    double scale = config ? config->scale : -1.0;
    if(scale < 0.1){
        if(dpi > 182){
            wlr_log(WLR_INFO, "Output: Assuming HiDPI scale");
            scale = 2.;
        }else{
            scale = 1.;
        }
    }

    wlr_log(WLR_INFO, "Output: Setting scale to %f", scale);
    wlr_output_set_scale(output->wlr_output, scale);

    return scale;
}

void wm_output_init(struct wm_output *output, struct wm_server *server,
        struct wm_layout *layout, struct wlr_output *out) {
    if(wm_output_overridden_name){
        strcpy(out->name, wm_output_overridden_name);
        wm_output_overridden_name = NULL;
    }
    wlr_log(WLR_INFO, "New output: %s: %s - %s (%s)", out->make, out->model, out->name, out->description);
    output->wm_server = server;
    output->wm_layout = layout;
    output->wlr_output = out;
    output->layout_x = 0;
    output->layout_y = 0;

    if (!wm_renderer_init_output(server->wm_renderer, output)) {
        wlr_log(WLR_ERROR, "Failed to init output render");
        return;
    }

    output->wlr_output_damage = wlr_output_damage_create(output->wlr_output);

    double scale = configure(output);

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

    /* Let the cursor know we possibly have a new scale */
    wm_cursor_ensure_loaded_for_scale(server->wm_seat->wm_cursor, scale);

#ifdef WM_CUSTOM_RENDERER
    output->renderer_buffers = NULL;
#endif

    output->expecting_frame = false;
    clock_gettime(CLOCK_MONOTONIC, &output->last_frame);
}

void wm_output_reconfigure(struct wm_output* output){
    double scale = configure(output);
    wm_cursor_ensure_loaded_for_scale(output->wm_layout->wm_server->wm_seat->wm_cursor, scale);
}

void wm_output_destroy(struct wm_output *output) {
    wl_list_remove(&output->destroy.link);
    wl_list_remove(&output->commit.link);
    wl_list_remove(&output->mode.link);
    wl_list_remove(&output->present.link);
    wl_list_remove(&output->link);
    wm_layout_remove_output(output->wm_layout, output);

#if WM_CUSTOM_RENDERER
    wm_renderer_buffers_destroy(output->renderer_buffers);
#endif
}
