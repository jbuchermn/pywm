#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <stdlib.h>
#include <wayland-server.h>
#include <wlr/util/log.h>
#include <wlr/xwayland.h>

#include "wm/wm_util.h"
#include "wm/wm_view_xwayland.h"
#include "wm/wm_view.h"
#include "wm/wm_seat.h"
#include "wm/wm_server.h"
#include "wm/wm.h"

/*
 * Callbacks
 */
static void handle_map(struct wl_listener* listener, void* data){
    struct wm_view_xwayland* view = wl_container_of(listener, view, map);

    const char* title;
    const char* app_id; 
    const char* role;
    wm_view_get_info(&view->super, &title, &app_id, &role);
    wlr_log(WLR_DEBUG, "New wm_view (xwayland): %s, %s, %s", title, app_id, role);

    wm_callback_init_view(&view->super);

    view->super.mapped = true;
}

static void handle_unmap(struct wl_listener* listener, void* data){
    struct wm_view_xwayland* view = wl_container_of(listener, view, unmap);
    view->super.mapped = false;
    wm_callback_destroy_view(&view->super);
}

static void handle_destroy(struct wl_listener* listener, void* data){
    struct wm_view_xwayland* view = wl_container_of(listener, view, destroy);
    wm_view_destroy(&view->super);

    free(view);
}


/*
 * Class implementation
 */
void wm_view_xwayland_init(struct wm_view_xwayland* view, struct wm_server* server, struct wlr_xwayland_surface* surface){
    wm_view_base_init(&view->super, server);

    view->super.vtable = &wm_view_xwayland_vtable;

    view->wlr_xwayland_surface = surface;

    view->map.notify = &handle_map;
    wl_signal_add(&surface->events.map, &view->map);

    view->unmap.notify = &handle_unmap;
    wl_signal_add(&surface->events.unmap, &view->unmap);

    view->destroy.notify = &handle_destroy;
    wl_signal_add(&surface->events.destroy, &view->destroy);
}

static void wm_view_xwayland_destroy(struct wm_view* super){
    struct wm_view_xwayland* view = wm_cast(wm_view_xwayland, super);

    wl_list_remove(&view->map.link);
    wl_list_remove(&view->unmap.link);
    wl_list_remove(&view->destroy.link);

    wm_view_base_destroy(&view->super);
}

static void wm_view_xwayland_get_info(struct wm_view* super, const char** title, const char** app_id, const char** role){
    struct wm_view_xwayland* view = wm_cast(wm_view_xwayland, super);

    *title = view->wlr_xwayland_surface->title;
    *app_id = view->wlr_xwayland_surface->class;
    *role = view->wlr_xwayland_surface->instance;

}

static void wm_view_xwayland_request_size(struct wm_view* super, int width, int height){
    struct wm_view_xwayland* view = wm_cast(wm_view_xwayland, super);

    wlr_xwayland_surface_configure(view->wlr_xwayland_surface, 0, 0, width, height);
}

static void wm_view_xwayland_get_size_constraints(struct wm_view* super, int* min_width, int* max_width, int* min_height, int* max_height){
    struct wm_view_xwayland* view = wm_cast(wm_view_xwayland, super);
    if(view->wlr_xwayland_surface->size_hints){
        *min_width = view->wlr_xwayland_surface->size_hints->min_width;
        *max_width = view->wlr_xwayland_surface->size_hints->max_width;
        *min_height = view->wlr_xwayland_surface->size_hints->min_height;
        *max_height = view->wlr_xwayland_surface->size_hints->max_height;
    }else{
        *min_width = -1;
        *max_width = -1;
        *min_height = -1;
        *max_height = -1;
    }
}

static void wm_view_xwayland_get_size(struct wm_view* super, int* width, int* height){
    struct wm_view_xwayland* view = wm_cast(wm_view_xwayland, super);

    if(!view->wlr_xwayland_surface->surface){
        *width = 0;
        *height = 0;
        return;
    }

    *width = view->wlr_xwayland_surface->surface->current.width;
    *height = view->wlr_xwayland_surface->surface->current.height;
}


static void wm_view_xwayland_focus(struct wm_view* super, struct wm_seat* seat){
    struct wm_view_xwayland* view = wm_cast(wm_view_xwayland, super);

    if(!view->wlr_xwayland_surface->surface){
        return;
    }
    wm_seat_focus_surface(seat, view->wlr_xwayland_surface->surface);
}

static void wm_view_xwayland_set_activated(struct wm_view* super, bool activated){
    struct wm_view_xwayland* view = wm_cast(wm_view_xwayland, super);

    if(!view->wlr_xwayland_surface->surface){
        return;
    }
    wlr_xwayland_surface_activate(view->wlr_xwayland_surface, activated);

}

static struct wlr_surface* wm_view_xwayland_surface_at(struct wm_view* super, double at_x, double at_y, double* sx, double* sy){
    struct wm_view_xwayland* view = wm_cast(wm_view_xwayland, super);

    if(!view->wlr_xwayland_surface->surface){
        return NULL;
    }
    return wlr_surface_surface_at(view->wlr_xwayland_surface->surface, at_x, at_y, sx, sy);
}

static void wm_view_xwayland_for_each_surface(struct wm_view* super, wlr_surface_iterator_func_t iterator, void* user_data){
    struct wm_view_xwayland* view = wm_cast(wm_view_xwayland, super);
    if(!view->wlr_xwayland_surface->surface){
        return;
    }
    wlr_surface_for_each_surface(view->wlr_xwayland_surface->surface, iterator, user_data);
}

static bool wm_view_xwayland_is_floating(struct wm_view* super){
    int min_w, max_w, min_h, max_h;
    wm_view_get_size_constraints(super, &min_w, &max_w, &min_h, &max_h);
    return min_w > 0 && min_h > 0 && min_w == max_w && min_h == max_h;
}

static struct wm_view* wm_view_xwayland_get_parent(struct wm_view* super){
    return NULL;
}

struct wm_view_vtable wm_view_xwayland_vtable = {
    .destroy = wm_view_xwayland_destroy,
    .get_info = wm_view_xwayland_get_info,
    .set_box = wm_view_base_set_box,
    .get_box = wm_view_base_get_box,
    .request_size = wm_view_xwayland_request_size,
    .get_size = wm_view_xwayland_get_size,
    .get_size_constraints = wm_view_xwayland_get_size_constraints,
    .focus = wm_view_xwayland_focus,
    .set_activated = wm_view_xwayland_set_activated,
    .surface_at = wm_view_xwayland_surface_at,
    .for_each_surface = wm_view_xwayland_for_each_surface,
    .is_floating = wm_view_xwayland_is_floating,
    .get_parent = wm_view_xwayland_get_parent
};
