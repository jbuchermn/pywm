#include "wm/wm_config.h"
#include <stdlib.h>
#include <string.h>
#include <wlr/util/log.h>

void wm_config_init_default(struct wm_config *config) {
    config->enable_output_manager = true;
    config->enable_xwayland = false;

    config->callback_frequency = 30;

    config->xkb_model = "";
    config->xkb_layout = "us";
    config->xkb_options = "";

    wl_list_init(&config->outputs);

    config->xcursor_theme = NULL;
    config->xcursor_size = 24;

    config->focus_follows_mouse = true;
    config->constrain_popups_to_toplevel = false;

    config->encourage_csd = true;
    config->debug_f1 = false;
}

void wm_config_add_output(struct wm_config *config, const char *name,
                          double scale, int width, int height, int mHz,
                          int pos_x, int pos_y) {
    if(!name){
        wlr_log(WLR_ERROR, "Cannot add output config without name");
        return;
    }
    struct wm_config_output *new = calloc(1, sizeof(struct wm_config_output));

    new->name = name;
    new->scale = scale;
    new->width = width;
    new->height = height;
    new->mHz = mHz;
    new->pos_x = pos_x;
    new->pos_y = pos_y;
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
