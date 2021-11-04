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
    wlr_log(WLR_DEBUG, "Layout: Change");
    struct wm_layout* layout = wl_container_of(listener, layout, change);

    if(layout->default_output){
        wlr_output_effective_resolution(layout->default_output->wlr_output, &layout->width, &layout->height);
    }else{
        layout->width = 0;
        layout->height = 0;
    }
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
    layout->width = 0;
    layout->height = 0;
}

void wm_layout_destroy(struct wm_layout* layout) {
    wl_list_remove(&layout->change.link);
}

void wm_layout_add_output(struct wm_layout* layout, struct wlr_output* out){
    wlr_log(WLR_DEBUG, "New output: %s", out->name);

    struct wm_output* output = calloc(1, sizeof(struct wm_output));
    wm_output_init(output, layout->wm_server, layout, out);
    wl_list_insert(&layout->wm_outputs, &output->link);

    if((strlen(layout->wm_server->wm_config->output_name) == 0 || !strcmp(layout->wm_server->wm_config->output_name, out->name)) && 
            !layout->default_output){
        layout->default_output = output;
    }else{
        wlr_log(WLR_ERROR, "Only one output supported - ignoring %s", out->name);
    }

    wlr_output_layout_add_auto(layout->wlr_output_layout, out);
}

void wm_layout_remove_output(struct wm_layout* layout, struct wm_output* output){
    if(output == layout->default_output){
        layout->default_output = NULL;
    }
    wlr_output_layout_remove(layout->wlr_output_layout, output->wlr_output);
}


void wm_layout_damage_whole(struct wm_layout* layout){
    if(!layout->default_output) return;

    wlr_output_damage_add_whole(layout->default_output->wlr_output_damage);
}

void wm_layout_damage_from(struct wm_layout* layout, struct wm_content* content, struct wlr_surface* origin){
    if(!layout->default_output) return;

    if(!content->lock_enabled && wm_server_is_locked(layout->wm_server)){
        wm_content_damage_output(content, layout->default_output, NULL);
    }else{
        wm_content_damage_output(content, layout->default_output, origin);
    }
}
