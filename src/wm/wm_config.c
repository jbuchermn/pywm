#include "wm/wm_config.h"
#include <stdlib.h>
#include <string.h>
#include <wlr/util/log.h>

#include "wm/wm_server.h"
#include "wm/wm_layout.h"
#include "wm/wm_seat.h"

void wm_config_init_default(struct wm_config *config) {
    config->enable_xwayland = false;

    config->callback_frequency = 20;

    strcpy(config->xkb_model, "");
    strcpy(config->xkb_layout, "");
    strcpy(config->xkb_options, "");

    wl_list_init(&config->outputs);

    config->xcursor_theme = NULL;
    config->xcursor_size = 24;

    config->natural_scroll = true;
    config->tap_to_click = true;

    config->focus_follows_mouse = true;
    config->constrain_popups_to_toplevel = false;

    config->encourage_csd = true;
    config->debug_f1 = false;
}

void wm_config_reset_default(struct wm_config* config){
    struct wm_config_output *output, *tmp;
    wl_list_for_each_safe(output, tmp, &config->outputs, link) {
        wl_list_remove(&output->link);
        free(output);
    }
    wm_config_init_default(config);
}

void wm_config_reconfigure(struct wm_config* config, struct wm_server* server){
    wm_seat_reconfigure(server->wm_seat);
    wm_layout_reconfigure(server->wm_layout);
    wm_server_reconfigure(server);
}

void wm_config_add_output(struct wm_config *config, const char *name,
                          double scale, int width, int height, int mHz,
                          int pos_x, int pos_y, enum wl_output_transform transform) {
    if(!name){
        wlr_log(WLR_ERROR, "Cannot add output config without name");
        return;
    }
    struct wm_config_output *new = calloc(1, sizeof(struct wm_config_output));

    strncpy(new->name, name, WM_CONFIG_STRLEN-1);
    new->scale = scale;
    new->width = width;
    new->height = height;
    new->mHz = mHz;
    new->pos_x = pos_x;
    new->pos_y = pos_y;
    new->transform = transform;
    wl_list_insert(&config->outputs, &new->link);
}

struct wm_config_output *wm_config_find_output(struct wm_config *config,
                                               const char *name) {
    if(!name){
        return NULL;
    }

    bool found = false;
    struct wm_config_output *output;
    wl_list_for_each(output, &config->outputs, link) {
        if (!strcmp(output->name, name)) {
            found = true;
            break;
        }
    }

    if (found)
        return output;
    else
        return NULL;
}

void wm_config_destroy(struct wm_config *config) {
    struct wm_config_output *output, *tmp;
    wl_list_for_each_safe(output, tmp, &config->outputs, link) {
        wl_list_remove(&output->link);
        free(output);
    }
}
