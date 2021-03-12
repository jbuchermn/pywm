#define _POSIX_C_SOURCE 200112L

#include <stdlib.h>
#include <wayland-server.h>
#include <wlr/util/log.h>
#include <wlr/types/wlr_idle_inhibit_v1.h>

#include "wm/wm_server.h"
#include "wm/wm_view.h"
#include "wm/wm_idle_inhibit.h"

static void handle_destroy(struct wl_listener* listener, void* data){
    wlr_log(WLR_DEBUG, "Inhibit: Destroying idle inhibitor");
    struct wm_idle_inhibitor* inhibitor = wl_container_of(listener, inhibitor, destroy);

    wm_idle_inhibitor_destroy(inhibitor);
    free(inhibitor);
}

void wm_idle_inhibitor_init(struct wm_idle_inhibitor* inhibitor, struct wm_idle_inhibit* parent, struct wlr_idle_inhibitor_v1* wlr_inhibitor){
    inhibitor->wlr_inhibitor = wlr_inhibitor;
    inhibitor->parent = parent;
    inhibitor->view = wm_server_view_for_surface(parent->wm_server, wlr_inhibitor->surface);

    inhibitor->destroy.notify = handle_destroy;
    wl_signal_add(&inhibitor->wlr_inhibitor->events.destroy, &inhibitor->destroy);

    if(inhibitor->view){
        wm_view_set_inhibiting_idle(inhibitor->view, true);
    }

}

void wm_idle_inhibitor_destroy(struct wm_idle_inhibitor* inhibitor){
    wl_list_remove(&inhibitor->destroy.link);

    if(inhibitor->view){
        wm_view_set_inhibiting_idle(inhibitor->view, false);
    }
}

static void handle_new_idle_inhibitor(struct wl_listener* listener, void* data){
    wlr_log(WLR_DEBUG, "Inhibit: New idle inhibitor");
    struct wm_idle_inhibit* inhibit = wl_container_of(listener, inhibit, new_idle_inhibitor);
    struct wlr_idle_inhibitor_v1* wlr_inhibitor = data;

    struct wm_idle_inhibitor* inhibitor = calloc(1, sizeof(struct wm_idle_inhibitor));
    wm_idle_inhibitor_init(inhibitor, inhibit, wlr_inhibitor);
}

void wm_idle_inhibit_init(struct wm_idle_inhibit* inhibit, struct wm_server* server){
    inhibit->wm_server = server;
    inhibit->wlr_idle_inhibit_manager = wlr_idle_inhibit_v1_create(server->wl_display);

    inhibit->new_idle_inhibitor.notify = handle_new_idle_inhibitor;
    wl_signal_add(&inhibit->wlr_idle_inhibit_manager->events.new_inhibitor, &inhibit->new_idle_inhibitor);
}

void wm_idle_inhibit_destroy(struct wm_idle_inhibit* inhibit){
    wl_list_remove(&inhibit->new_idle_inhibitor.link);
}
