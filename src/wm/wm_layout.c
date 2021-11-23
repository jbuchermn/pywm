#define _POSIX_C_SOURCE 200112L

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
        wlr_output_damage_add_whole(output->wlr_output_damage);
    }
}


void wm_layout_damage_from(struct wm_layout* layout, struct wm_content* content, struct wlr_surface* origin){
    double display_x, display_y, display_width, display_height;
    wm_content_get_box(content, &display_x, &display_y, &display_width, &display_height);
    struct wlr_box box = {
        .x = display_x,
        .y = display_y,
        .width = display_width,
        .height = display_height
    };

    struct wm_output* output;
    wl_list_for_each(output, &layout->wm_outputs, link){
        if(content->fixed_output && content->fixed_output != output) continue;
        if(!wlr_output_layout_intersects(layout->wlr_output_layout, output->wlr_output, &box)) continue;

        if(!content->lock_enabled && wm_server_is_locked(layout->wm_server)){
            wm_content_damage_output(content, output, NULL);
        }else{
            wm_content_damage_output(content, output, origin);
        }
    }
}

struct send_enter_leave_data {
    bool enter;
    struct wm_output* output;
};

static void send_enter_leave_it(struct wlr_surface *surface, int sx, int sy, void *data){
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

    double display_x, display_y, display_width, display_height;
    wm_content_get_box(content, &display_x, &display_y, &display_width, &display_height);
    struct wlr_box box = {
        .x = display_x,
        .y = display_y,
        .width = display_width,
        .height = display_height
    };

    struct wm_output* output;
    wl_list_for_each(output, &layout->wm_outputs, link){
        struct send_enter_leave_data data = {.enter = true, .output = output};
        data.enter = view->super.fixed_output == output || (wlr_output_layout_intersects(layout->wlr_output_layout, output->wlr_output, &box) && view->super.fixed_output == NULL);
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
