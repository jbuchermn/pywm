#define _POSIX_C_SOURCE 200112L

#include <wayland-server.h>
#include <wlr/util/log.h>
#include "wm/wm_drag.h"

static void handle_destroy(struct wl_listener* listener, void* data){
    wlr_log(WLR_DEBUG, "Drag destroyed");
    struct wm_drag* drag = wl_container_of(listener, drag, destroy);
}

static void icon_handle_surface_commit(struct wl_listener* listener, void* data){
    struct wm_drag* drag = wl_container_of(listener, drag, icon_surface_commit);
}

static void icon_handle_destroy(struct wl_listener* listener, void* data){
    struct wm_drag* drag = wl_container_of(listener, drag, icon_destroy);
    wlr_log(WLR_DEBUG, "Drag-icon destroyed");
}
static void icon_handle_map(struct wl_listener* listener, void* data){
    struct wm_drag* drag = wl_container_of(listener, drag, icon_map);
}
static void icon_handle_unmap(struct wl_listener* listener, void* data){
    struct wm_drag* drag = wl_container_of(listener, drag, icon_unmap);
}

void wm_drag_init(struct wm_drag* drag, struct wm_seat* seat, struct wlr_drag* wlr_drag){
    drag->wm_seat = seat;
    drag->wlr_drag = wlr_drag;

    drag->destroy.notify = handle_destroy;
    wl_signal_add(&wlr_drag->events.destroy, &drag->destroy);

    drag->icon.wlr_drag_icon = NULL;
    if(wlr_drag->icon){
        drag->icon.wlr_drag_icon = wlr_drag->icon;

        drag->icon_destroy.notify = icon_handle_destroy;
        wl_signal_add(&wlr_drag->icon->events.destroy, &drag->icon_destroy);

        drag->icon_map.notify = icon_handle_map;
        wl_signal_add(&wlr_drag->icon->events.map, &drag->icon_map);

        drag->icon_unmap.notify = icon_handle_unmap;
        wl_signal_add(&wlr_drag->icon->events.unmap, &drag->icon_unmap);

        drag->icon_surface_commit.notify = icon_handle_surface_commit;
        wl_signal_add(&wlr_drag->icon->surface->events.commit, &drag->icon_surface_commit);
    }
}

void wm_drag_destroy(struct wm_drag* drag){
    wl_list_remove(&drag->destroy.link);
    if(drag->icon.wlr_drag_icon){
        wl_list_remove(&drag->icon_map.link);
        wl_list_remove(&drag->icon_unmap.link);
        wl_list_remove(&drag->icon_destroy.link);
        wl_list_remove(&drag->icon_surface_commit.link);
    }
}
