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

/*
 * Callbacks
 */
static void handle_change(struct wl_listener* listener, void* data){
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
}

void wm_layout_destroy(struct wm_layout* layout) {
    wl_list_remove(&layout->change.link);
}

void wm_layout_add_output(struct wm_layout* layout, struct wlr_output* out){
    struct wm_output* output = calloc(1, sizeof(struct wm_output));
    wm_output_init(output, layout->wm_server, layout, out);
    wl_list_insert(&layout->wm_outputs, &output->link);

    struct wm_config_output* config = wm_config_find_output(layout->wm_server->wm_config, output->wlr_output->name);
    if(!config || (config->pos_x < 0 || config->pos_y < 0)){
        wlr_log(WLR_INFO, "New output: Placing automatically");
        wlr_output_layout_add_auto(layout->wlr_output_layout, out);
    }else{
        wlr_log(WLR_INFO, "New output: Placing at %d / %d", config->pos_x, config->pos_y);
        wlr_output_layout_add(layout->wlr_output_layout, out, config->pos_x, config->pos_y);
    }
}

void wm_layout_remove_output(struct wm_layout* layout, struct wm_output* output){
    wlr_output_layout_remove(layout->wlr_output_layout, output->wlr_output);
}


void wm_layout_damage_whole(struct wm_layout* layout){
    /* TODO */
    /* if(!layout->default_output) return; */

    /* wlr_output_damage_add_whole(layout->default_output->wlr_output_damage); */
}

void wm_layout_damage_from(struct wm_layout* layout, struct wm_content* content, struct wlr_surface* origin){
    /* TODO */
    /* if(!layout->default_output) return; */

    /* if(!content->lock_enabled && wm_server_is_locked(layout->wm_server)){ */
    /*     wm_content_damage_output(content, layout->default_output, NULL); */
    /* }else{ */
    /*     wm_content_damage_output(content, layout->default_output, origin); */
    /* } */
}
