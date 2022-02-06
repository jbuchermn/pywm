#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <assert.h>
#include <wlr/util/log.h>
#include "wm/wm_layout.h"
#include "wm/wm_output.h"
#include "wm/wm.h"
#include "wm/wm_view.h"
#include "wm/wm_server.h"
#include "wm/wm_config.h"
#include "wm/wm_util.h"
#include "wm/wm_composite.h"

/*
 * Callbacks
 */

static int _wm_output_key = 0;
static void handle_change(struct wl_listener* listener, void* data){
    struct wm_layout* layout = wl_container_of(listener, layout, change);
    layout->fastest_output_mHz = 1000;

    struct wm_output* output;
    wl_list_for_each(output, &layout->wm_outputs, link){
        output->key = _wm_output_key++;

        if(output->wlr_output->refresh > layout->fastest_output_mHz){
            layout->fastest_output_mHz = output->wlr_output->refresh;
        }

        struct wlr_output_layout_output* o = wlr_output_layout_get(layout->wlr_output_layout, output->wlr_output);
        if(!o){
            wlr_log(WLR_ERROR, "Output not in output layout: %s", output->wlr_output->name);
        }else{
            output->layout_x = o->x;
            output->layout_y = o->y;
        }
    }

    wm_callback_layout_change(layout);
    wm_layout_damage_whole(layout);
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

    /* Some sensible (too slow) default */
    layout->fastest_output_mHz = 1000;
}

void wm_layout_destroy(struct wm_layout* layout) {
    wl_list_remove(&layout->change.link);
}

static void place(struct wm_layout* layout, struct wm_output* output){
    struct wm_config_output* config = wm_config_find_output(layout->wm_server->wm_config, output->wlr_output->name);
    if(!config || (config->pos_x < WM_CONFIG_POS_MIN || config->pos_y < WM_CONFIG_POS_MIN)){
        wlr_log(WLR_INFO, "Layout: Placing automatically");
        wlr_output_layout_add_auto(layout->wlr_output_layout, output->wlr_output);
    }else{
        wlr_log(WLR_INFO, "Layout: Placing at %d / %d", config->pos_x, config->pos_y);
        wlr_output_layout_add(layout->wlr_output_layout, output->wlr_output, config->pos_x, config->pos_y);
    }
}

void wm_layout_add_output(struct wm_layout* layout, struct wlr_output* out){
    struct wm_output* output = calloc(1, sizeof(struct wm_output));
    wm_output_init(output, layout->wm_server, layout, out);
    wl_list_insert(&layout->wm_outputs, &output->link);

    place(layout, output);
}

void wm_layout_remove_output(struct wm_layout* layout, struct wm_output* output){
    wlr_output_layout_remove(layout->wlr_output_layout, output->wlr_output);
}

void wm_layout_reconfigure(struct wm_layout* layout){
    struct wm_output* output;
    wl_list_for_each(output, &layout->wm_outputs, link){
        wm_output_reconfigure(output);
        place(layout, output);
    }
}


void wm_layout_damage_whole(struct wm_layout* layout){
    struct wm_output* output;
    wl_list_for_each(output, &layout->wm_outputs, link){
        wlr_log(WLR_DEBUG, "DEBUGLAG - Damaging whole on output %d", output->key);
        wlr_output_damage_add_whole(output->wlr_output_damage);

        /* No need to call wm_composite_on_damage_below here, as this method
         * is only meant to extend frame damage. This might change, if wm_composites
         * store their contents in buffers. */

        wlr_log(WLR_DEBUG, "DEBUGLAG - scheduling frame (2) on output %d", output->key);
        wlr_output_schedule_frame(output->wlr_output);
    }

}


void wm_layout_damage_from(struct wm_layout* layout, struct wm_content* content, struct wlr_surface* origin){
    wlr_log(WLR_DEBUG, "DEBUGLAG - wm_layout_damage_from %p", origin);
    struct wm_output* output;
    wl_list_for_each(output, &layout->wm_outputs, link){
        if(!wm_content_is_on_output(content, output)) continue;

        if(!content->lock_enabled && wm_server_is_locked(layout->wm_server)){
            wm_content_damage_output(content, output, NULL);
        }else{
            wm_content_damage_output(content, output, origin);
        }

        wlr_log(WLR_DEBUG, "DEBUGLAG - scheduling frame (1) on output %d", output->key);
        wlr_output_schedule_frame(output->wlr_output);
    }
}

void wm_layout_damage_output(struct wm_layout* layout, struct wm_output* output, pixman_region32_t* damage, struct wm_content* from){

    wlr_log(WLR_DEBUG, "DEBUGLAG - Adding damage on output %d", output->key);
    wlr_output_damage_add(output->wlr_output_damage, damage);

    struct wm_content* content;
    wl_list_for_each(content, &layout->wm_server->wm_contents, link){
        if(!wm_content_is_composite(content)) continue;
        struct wm_composite* comp = wm_cast(wm_composite, content);
        if(&comp->super != from && comp->super.z_index > from->z_index){
            wm_composite_on_damage_below(comp, output, damage);
        }
    }
}

struct send_enter_leave_data {
    bool enter;
    struct wm_output* output;
};

static void send_enter_leave_it(struct wlr_surface *surface, int sx, int sy, bool constrained, void *data){
    struct send_enter_leave_data* edata = data;
    if(edata->enter){
        wlr_surface_send_enter(surface, edata->output->wlr_output);
    }else{
        wlr_surface_send_leave(surface, edata->output->wlr_output);
    }
}

void wm_layout_update_content_outputs(struct wm_layout* layout, struct wm_content* content){
    if(!wm_content_is_view(content)) return;
    struct wm_view* view = wm_cast(wm_view, content);

    struct wm_output* output;
    wl_list_for_each(output, &layout->wm_outputs, link){
        struct send_enter_leave_data data = {
            .enter = wm_content_is_on_output(&view->super, output),
            .output = output};
        wm_view_for_each_surface(view, send_enter_leave_it, &data);
    }
}

void wm_layout_printf(FILE* file, struct wm_layout* layout){
    fprintf(file, "wm_layout\n");
    struct wm_output* output;
    wl_list_for_each(output, &layout->wm_outputs, link){
        int width, height;
        wlr_output_transformed_resolution(output->wlr_output, &width, &height);
        fprintf(file, "  wm_output: %s (%d x %d) at %d, %d\n", output->wlr_output->name, width, height, output->layout_x, output->layout_y);
    }
}
