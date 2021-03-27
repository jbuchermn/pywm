#include <stdlib.h>
#include "wm/wm_config.h"

void wm_config_init_default(struct wm_config* config){
    config->output_scale = 1.;
    config->enable_output_manager = true;
    config->enable_xwayland = false;

    config->callback_frequency = 30;

    config->xkb_model = "";
    config->xkb_layout = "us";
    config->xkb_options = "";

    config->xcursor_theme = NULL;
    config->xcursor_size = 24;

    config->focus_follows_mouse = true;
    config->constrain_popups_to_toplevel = false;

    config->encourage_csd = true;
    config->debug_f1 = false;
}
