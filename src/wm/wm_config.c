#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <string.h>
#include <wlr/util/log.h>

#include "wm/wm_config.h"
#include "wm/wm_server.h"
#include "wm/wm_layout.h"
#include "wm/wm_seat.h"
#include "wm/wm_renderer.h"

static void xcursor_setenv(struct wm_config* config){
    char cursor_size_fmt[16];
    snprintf(cursor_size_fmt, sizeof(cursor_size_fmt), "%u", config->xcursor_size);
    setenv("XCURSOR_SIZE", cursor_size_fmt, 1);
    if (config->xcursor_theme != NULL) {
        setenv("XCURSOR_THEME", config->xcursor_theme, 1);
    }
}

void wm_config_init_default(struct wm_config *config) {
    config->enable_xwayland = false;

    config->callback_frequency = 10;

    strcpy(config->xkb_model, "");
    strcpy(config->xkb_layout, "");
    strcpy(config->xkb_variant, "");
    strcpy(config->xkb_options, "");
    strcpy(config->texture_shaders, "basic");

    wl_list_init(&config->outputs);

    const char *cursor_theme = getenv("XCURSOR_THEME");
    unsigned cursor_size = 24;
    const char *env_cursor_size = getenv("XCURSOR_SIZE");
    if (env_cursor_size && strlen(env_cursor_size) > 0) {
        errno = 0;
        char *end;
        unsigned size = strtoul(env_cursor_size, &end, 10);
        if (!*end && errno == 0) {
            cursor_size = size;
        }
    }

    config->xcursor_theme = cursor_theme ? strdup(cursor_theme) : "";
    config->xcursor_size = cursor_size;

    config->natural_scroll = true;
    config->tap_to_click = true;

    config->focus_follows_mouse = true;
    config->constrain_popups_to_toplevel = false;

    config->encourage_csd = true;
    config->debug = false;
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

    xcursor_setenv(config);
    wm_renderer_select_texture_shaders(server->wm_renderer, config->texture_shaders);
    wm_renderer_ensure_mode(server->wm_renderer, wm_config_get_renderer_mode(config));
}

enum wm_renderer_mode wm_config_get_renderer_mode(struct wm_config* config){
    if(!strcmp(config->renderer_mode, "wlr")){
        return WM_RENDERER_WLR;
    }else if(!strcmp(config->renderer_mode, "pywm")){
        return WM_RENDERER_PYWM;
    } 

    return WM_RENDERER_PYWM;
}

void wm_config_set_xcursor_theme(struct wm_config* config, const char* xcursor_theme){
    config->xcursor_theme = xcursor_theme;
    xcursor_setenv(config);
}

void wm_config_set_xcursor_size(struct wm_config* config, int xcursor_size){
    config->xcursor_size = xcursor_size;
    xcursor_setenv(config);
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
