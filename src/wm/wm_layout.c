#define _POSIX_C_SOURCE 200112L

#include <stdlib.h>
#include <assert.h>
#include <wlr/util/log.h>
#include "wm/wm_layout.h"
#include "wm/wm_output.h"
#include "wm/wm.h"
#include "wm/wm_view.h"

/*
 * Callbacks
 */
static void handle_change(struct wl_listener* listener, void* data){
    wlr_log(WLR_DEBUG, "Layout: Change");
    struct wm_layout* layout = wl_container_of(listener, layout, change);

    wm_callback_layout_change(layout);
}

/*
 * Class implementation
 */
void wm_layout_init(struct wm_layout* layout, struct wm_server* server){
    layout->wm_server = server;
    wl_list_init(&layout->wm_outputs);

    layout->wlr_output_layout = wlr_output_layout_create();
    assert(layout->wlr_output_layout);

    layout->change.notify = &handle_change;
    wl_signal_add(&layout->wlr_output_layout->events.change, &layout->change);

    layout->default_output = NULL;
}

void wm_layout_destroy(struct wm_layout* layout) {
    wl_list_remove(&layout->change.link);
}

void wm_layout_add_output(struct wm_layout* layout, struct wlr_output* out){
    wlr_log(WLR_DEBUG, "New output: %s", out->name);

    struct wm_output* output = calloc(1, sizeof(struct wm_output));
    wm_output_init(output, layout->wm_server, layout, out);
    wl_list_insert(&layout->wm_outputs, &output->link);

    wlr_output_layout_add_auto(layout->wlr_output_layout, out);

    if(!layout->default_output) layout->default_output = output;
}

struct damage_data {
    struct wm_output* output;
    double x;
    double y;
};

static void damage_surface(struct wlr_surface *surface, int sx, int sy, void *data) {
    struct damage_data* ddata = data;

    if(pixman_region32_not_empty(&surface->buffer_damage)){
        pixman_region32_t region;
        pixman_region32_init(&region);

        pixman_region32_union_rect(&region, &region, 0, 0, 2560, 1600);

        wlr_output_damage_add(
                ddata->output->wlr_output_damage,
                &region);
        pixman_region32_fini(&region);
    }
}

void wm_layout_damage_from(struct wm_layout* layout, struct wm_view* view){
    if(!layout->default_output) return;

    struct damage_data ddata = {
        .output = layout->default_output,
        .x = view->super.display_x,
        .y = view->super.display_y
    };
    wm_view_for_each_surface(view, damage_surface, &ddata);
}

void wm_layout_damage_whole(struct wm_layout* layout){
    if(!layout->default_output) return;

    wlr_output_damage_add_whole(layout->default_output->wlr_output_damage);
}
