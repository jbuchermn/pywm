#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <stdlib.h>
#include <wayland-server.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <wlr/xwayland.h>

#include "wm/wm_util.h"
#include "wm/wm_view_xdg.h"
#include "wm/wm_view.h"
#include "wm/wm_seat.h"
#include "wm/wm_server.h"
#include "wm/wm.h"

/*
 * Callbacks: Popup
 */
static void popup_handle_map(struct wl_listener* listener, void* data){
    struct wm_view_xdg_popup* popup = wl_container_of(listener, popup, map);
    /*
     * We don't actually do anything with the popups, merely a blueprint
     */
}

static void popup_handle_unmap(struct wl_listener* listener, void* data){
    struct wm_view_xdg_popup* popup = wl_container_of(listener, popup, unmap);
}

static void popup_handle_destroy(struct wl_listener* listener, void* data){
    struct wm_view_xdg_popup* popup = wl_container_of(listener, popup, destroy);
    wm_view_xdg_popup_destroy(popup);
    free(popup);
}

static void popup_handle_new_popup(struct wl_listener* listener, void* data){
    struct wm_view_xdg_popup* popup = wl_container_of(listener, popup, new_popup);
    struct wlr_xdg_popup* wlr_xdg_popup = data;

    struct wm_view_xdg_popup* popup_new = calloc(1, sizeof(struct wm_view_xdg_popup));
    wm_view_xdg_popup_init(popup_new, wlr_xdg_popup, popup->parent);
}

/*
 * Callbacks: View
 */
static void handle_map(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, map);

    const char* title;
    const char* app_id; 
    const char* role;
    wm_view_get_info(&view->super, &title, &app_id, &role);
    wlr_log(WLR_DEBUG, "New wm_view (xdg): %s, %s, %s", title, app_id, role);

    wm_callback_init_view(&view->super);

    view->super.mapped = true;
}

static void handle_unmap(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, unmap);
    view->super.mapped = false;
    wm_callback_destroy_view(&view->super);
}

static void handle_destroy(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, destroy);
    wm_view_destroy(&view->super);
    free(view);
}

static void handle_new_popup(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, new_popup);
    struct wlr_xdg_popup* wlr_xdg_popup = data;

    struct wm_view_xdg_popup* popup = calloc(1, sizeof(struct wm_view_xdg_popup));
    wm_view_xdg_popup_init(popup, wlr_xdg_popup, view);
}


/*
 * Class implementation
 */
void wm_view_xdg_popup_init(struct wm_view_xdg_popup* popup, struct wlr_xdg_popup* wlr_xdg_popup, struct wm_view_xdg* parent){
    popup->parent = parent;
    wl_list_insert(&parent->popups, &popup->link);

    popup->map.notify = &popup_handle_map;
    wl_signal_add(&wlr_xdg_popup->base->events.map, &popup->map);

    popup->unmap.notify = &popup_handle_unmap;
    wl_signal_add(&wlr_xdg_popup->base->events.unmap, &popup->unmap);

    popup->destroy.notify = &popup_handle_destroy;
    wl_signal_add(&wlr_xdg_popup->base->events.destroy, &popup->destroy);

    popup->new_popup.notify = &popup_handle_new_popup;
    wl_signal_add(&wlr_xdg_popup->base->events.new_popup, &popup->new_popup);
}

void wm_view_xdg_popup_destroy(struct wm_view_xdg_popup* popup){
    wl_list_remove(&popup->map.link);
    wl_list_remove(&popup->unmap.link);
    wl_list_remove(&popup->destroy.link);
    wl_list_remove(&popup->new_popup.link);
    wl_list_remove(&popup->link);
}

void wm_view_xdg_init(struct wm_view_xdg* view, struct wm_server* server, struct wlr_xdg_surface* surface){
    wm_view_base_init(&view->super, server);
    view->super.vtable = &wm_view_xdg_vtable;

    wl_list_init(&view->popups);

    view->wlr_xdg_surface = surface;


    view->map.notify = &handle_map;
    wl_signal_add(&surface->events.map, &view->map);

    view->unmap.notify = &handle_unmap;
    wl_signal_add(&surface->events.unmap, &view->unmap);

    view->destroy.notify = &handle_destroy;
    wl_signal_add(&surface->events.destroy, &view->destroy);

    view->new_popup.notify = &handle_new_popup;
    wl_signal_add(&surface->events.new_popup, &view->new_popup);

    /* Get rid of white spaces around; therefore geometry.width/height should always equal current.width/height */
    /*
     * Problematic for toplevels that are actually popups; why the hell do apps create toplevels
     * for save/... dialogs? Anyway.. see sway xdg_shell.c wants_floating()
     */
    wlr_xdg_toplevel_set_tiled(surface, 15);
}

static void wm_view_xdg_destroy(struct wm_view* super){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);

    wl_list_remove(&view->map.link);
    wl_list_remove(&view->unmap.link);
    wl_list_remove(&view->destroy.link);
    wl_list_remove(&view->new_popup.link);

    wm_view_base_destroy(&view->super);
}

static void wm_view_xdg_get_info(struct wm_view* super, const char** title, const char** app_id, const char** role){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);

    *title = view->wlr_xdg_surface->toplevel->title;
    *app_id = view->wlr_xdg_surface->toplevel->app_id;
    *role = "toplevel";

}

static void wm_view_xdg_request_size(struct wm_view* super, int width, int height){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);
    if(!view->wlr_xdg_surface){
        wlr_log(WLR_DEBUG, "Warning: view with wlr_xdg_surface == 0");
        return;
    }

    if(view->wlr_xdg_surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL){
        wlr_xdg_toplevel_set_size(view->wlr_xdg_surface, width, height);
    }else{
        wlr_log(WLR_DEBUG, "Warning: Not toplevel");
    }
}

static void wm_view_xdg_get_size_constraints(struct wm_view* super, int* min_width, int* max_width, int* min_height, int* max_height){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);

    if(view->wlr_xdg_surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL){
        *min_width = view->wlr_xdg_surface->toplevel->current.min_width;
        *max_width = view->wlr_xdg_surface->toplevel->current.max_width;
        *min_height = view->wlr_xdg_surface->toplevel->current.min_height;
        *max_height = view->wlr_xdg_surface->toplevel->current.max_height;
    }else{
        wlr_log(WLR_DEBUG, "Warning: Not toplevel");
        *min_width = -1;
        *max_width = -1;
        *min_height = -1;
        *max_height = -1;
    }
}

static void wm_view_xdg_get_size(struct wm_view* super, int* width, int* height){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);

    /* Fixed by set_tiled */
    /* Although during updates not strictly equal? */
    /* assert(view->wlr_xdg_surface->geometry.width == view->wlr_xdg_surface->surface->current.width); */
    /* assert(view->wlr_xdg_surface->geometry.height == view->wlr_xdg_surface->surface->current.height); */


    *width = view->wlr_xdg_surface->geometry.width;
    *height = view->wlr_xdg_surface->geometry.height;
}


static void wm_view_xdg_focus(struct wm_view* super, struct wm_seat* seat){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);
    wm_seat_focus_surface(seat, view->wlr_xdg_surface->surface);
}

static void wm_view_xdg_set_activated(struct wm_view* super, bool activated){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);
    if(!view->wlr_xdg_surface){
        return;
    }
    wlr_xdg_toplevel_set_activated(view->wlr_xdg_surface, activated);

}

static struct wlr_surface* wm_view_xdg_surface_at(struct wm_view* super, double at_x, double at_y, double* sx, double* sy){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);
    return wlr_xdg_surface_surface_at(view->wlr_xdg_surface, at_x, at_y, sx, sy);
}

static void wm_view_xdg_for_each_surface(struct wm_view* super, wlr_surface_iterator_func_t iterator, void* user_data){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);
    wlr_xdg_surface_for_each_surface(view->wlr_xdg_surface, iterator, user_data);
}

struct wm_view_vtable wm_view_xdg_vtable = {
    .destroy = wm_view_xdg_destroy,
    .get_info = wm_view_xdg_get_info,
    .set_box = wm_view_base_set_box,
    .get_box = wm_view_base_get_box,
    .request_size = wm_view_xdg_request_size,
    .get_size_constraints = wm_view_xdg_get_size_constraints,
    .get_size = wm_view_xdg_get_size,
    .focus = wm_view_xdg_focus,
    .set_activated = wm_view_xdg_set_activated,
    .surface_at = wm_view_xdg_surface_at,
    .for_each_surface = wm_view_xdg_for_each_surface,
};
