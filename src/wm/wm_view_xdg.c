#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <stdlib.h>
#include <wayland-server.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <wlr/xwayland.h>
#include <wlr/types/wlr_output_layout.h>

#include "wm/wm_view_xdg.h"

#include "wm/wm_util.h"
#include "wm/wm_config.h"
#include "wm/wm_view.h"
#include "wm/wm_seat.h"
#include "wm/wm_server.h"
#include "wm/wm_layout.h"
#include "wm/wm.h"

struct wm_view_vtable wm_view_xdg_vtable;

/*
 * Callbacks
 */
static void popup_handle_map(struct wl_listener* listener, void* data){
    struct wm_popup_xdg* popup = wl_container_of(listener, popup, map);

    /* Noop */
}

static void popup_handle_unmap(struct wl_listener* listener, void* data){
    struct wm_popup_xdg* popup = wl_container_of(listener, popup, unmap);

    /* Noop */
}

static void popup_handle_destroy(struct wl_listener* listener, void* data){
    struct wm_popup_xdg* popup = wl_container_of(listener, popup, destroy);
    wm_popup_xdg_destroy(popup);
    free(popup);
}

static void popup_handle_new_popup(struct wl_listener* listener, void* data){
    struct wm_popup_xdg* popup = wl_container_of(listener, popup, new_popup);
    struct wlr_xdg_popup* wlr_xdg_popup = data;

    struct wm_popup_xdg* new_popup = calloc(1, sizeof(struct wm_popup_xdg));
    wm_popup_xdg_init(new_popup, popup->toplevel, wlr_xdg_popup);
}



static void handle_map(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, map);

    int min_w, max_w, min_h, max_h;
    wm_view_get_size_constraints(&view->super, &min_w, &max_w, &min_h, &max_h);

    bool has_parent = view->wlr_xdg_surface->toplevel && view->wlr_xdg_surface->toplevel->parent;

    /* Logic from sway */
    if(has_parent || (min_w > 0 && (min_w == max_w) && min_h > 0 && (min_h = max_h))){
        view->floating = true;
        wlr_xdg_toplevel_set_tiled(view->wlr_xdg_surface, 0);
    }

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
    wm_content_destroy(&view->super.super);
    free(view);
}

static void handle_new_popup(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, new_popup);
    struct wlr_xdg_popup* wlr_xdg_popup = data;

    struct wm_popup_xdg* popup = calloc(1, sizeof(struct wm_popup_xdg));
    wm_popup_xdg_init(popup, view, wlr_xdg_popup);
}

static void handle_fullscreen(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, request_fullscreen);

    wlr_log(WLR_INFO, "Fullscreen requested");
}

static void handle_move(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, request_move);

    wlr_log(WLR_INFO, "Move requested");
}

static void handle_resize(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, request_resize);

    wlr_log(WLR_INFO, "Resize requested");
}

static void handle_minimize(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, request_minimize);

    wlr_log(WLR_INFO, "Minimize requested");
}

static void handle_maximize(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, request_maximize);

    wlr_log(WLR_INFO, "Maximize requested");
}

static void handle_show_window_menu(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, request_show_window_menu);

    wlr_log(WLR_INFO, "Show window requested");
}

/*
 * Class implementation wm_popup_xdg
 */
void wm_popup_xdg_init(struct wm_popup_xdg* popup, struct wm_view_xdg* toplevel, struct wlr_xdg_popup* wlr_xdg_popup){

    popup->wlr_xdg_popup = wlr_xdg_popup;
    popup->toplevel = toplevel;

    popup->map.notify = &popup_handle_map;
    wl_signal_add(&wlr_xdg_popup->base->events.map, &popup->map);

    popup->unmap.notify = &popup_handle_unmap;
    wl_signal_add(&wlr_xdg_popup->base->events.unmap, &popup->unmap);

    popup->destroy.notify = &popup_handle_destroy;
    wl_signal_add(&wlr_xdg_popup->base->events.destroy, &popup->destroy);

    popup->new_popup.notify = &popup_handle_new_popup;
    wl_signal_add(&wlr_xdg_popup->base->events.new_popup, &popup->new_popup);


    /* Unconstrain popup */
    int width, height;
    wm_view_get_size(&popup->toplevel->super, &width, &height);

    if(popup->toplevel->constrain_popups_to_toplevel){
        struct wlr_box box = {
            .x = 0,
            .y = 0,
            .width = width,
            .height = height
        };
        wlr_xdg_popup_unconstrain_from_box(popup->wlr_xdg_popup, &box);
    }else{
        struct wlr_box* output_box = wlr_output_layout_get_box(
                popup->toplevel->super.super.wm_server->wm_layout->wlr_output_layout, NULL);

        double x_scale = width / popup->toplevel->super.super.display_width;
        double y_scale = height / popup->toplevel->super.super.display_height;
        struct wlr_box box = {
            .x = -popup->toplevel->super.super.display_x * x_scale,
            .y = -popup->toplevel->super.super.display_y * y_scale,
            .width = output_box->width * x_scale,
            .height = output_box->height * y_scale
        };
        wlr_xdg_popup_unconstrain_from_box(popup->wlr_xdg_popup, &box);
    }
}

void wm_popup_xdg_destroy(struct wm_popup_xdg* popup){
    wl_list_remove(&popup->map.link);
    wl_list_remove(&popup->unmap.link);
    wl_list_remove(&popup->destroy.link);
    wl_list_remove(&popup->new_popup.link);
}


/*
 * Class implementation wm_view_xdg
 */
void wm_view_xdg_init(struct wm_view_xdg* view, struct wm_server* server, struct wlr_xdg_surface* surface){
    wm_view_base_init(&view->super, server);
    view->super.vtable = &wm_view_xdg_vtable;

    view->wlr_xdg_surface = surface;

    view->map.notify = &handle_map;
    wl_signal_add(&surface->events.map, &view->map);

    view->unmap.notify = &handle_unmap;
    wl_signal_add(&surface->events.unmap, &view->unmap);

    view->destroy.notify = &handle_destroy;
    wl_signal_add(&surface->events.destroy, &view->destroy);

    view->new_popup.notify = &handle_new_popup;
    wl_signal_add(&surface->events.new_popup, &view->new_popup);

    view->request_fullscreen.notify = &handle_fullscreen;
    wl_signal_add(&surface->toplevel->events.request_fullscreen, &view->request_fullscreen);

    view->request_move.notify = &handle_move;
    wl_signal_add(&surface->toplevel->events.request_move, &view->request_move);

    view->request_resize.notify = &handle_resize;
    wl_signal_add(&surface->toplevel->events.request_resize, &view->request_resize);

    view->request_maximize.notify = &handle_maximize;
    wl_signal_add(&surface->toplevel->events.request_maximize, &view->request_maximize);

    view->request_minimize.notify = &handle_minimize;
    wl_signal_add(&surface->toplevel->events.request_minimize, &view->request_minimize);

    view->request_show_window_menu.notify = &handle_show_window_menu;
    wl_signal_add(&surface->toplevel->events.request_show_window_menu, &view->request_show_window_menu);


    /* Get rid of white spaces around; therefore geometry.width/height should always equal current.width/height */
    view->floating = false;
    wlr_xdg_toplevel_set_tiled(surface, 15);

    view->constrain_popups_to_toplevel = server->wm_config->constrain_popups_to_toplevel;
}

static void wm_view_xdg_destroy(struct wm_view* super){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);

    wl_list_remove(&view->map.link);
    wl_list_remove(&view->unmap.link);
    wl_list_remove(&view->destroy.link);
    wl_list_remove(&view->new_popup.link);
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


    /* Again following sway here */
    *width = view->wlr_xdg_surface->geometry.width;
    *height = view->wlr_xdg_surface->geometry.height;

    if(!(*width) || !(*height)){
        *width = view->wlr_xdg_surface->surface->current.width;
        *height = view->wlr_xdg_surface->surface->current.height;
    }

}

static void wm_view_xdg_get_offset(struct wm_view* super, int* offset_x, int* offset_y){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);

    *offset_x = view->wlr_xdg_surface->geometry.x;
    *offset_y = view->wlr_xdg_surface->geometry.y;
}

static void wm_view_xdg_set_resizing(struct wm_view* super, bool resizing){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);
    wlr_xdg_toplevel_set_resizing(view->wlr_xdg_surface, resizing);
}
static void wm_view_xdg_set_fullscreen(struct wm_view* super, bool fullscreen){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);
    wlr_xdg_toplevel_set_fullscreen(view->wlr_xdg_surface, fullscreen);
}
static void wm_view_xdg_set_maximized(struct wm_view* super, bool maximized){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);
    wlr_xdg_toplevel_set_maximized(view->wlr_xdg_surface, maximized);
}
static void wm_view_xdg_set_activated(struct wm_view* super, bool activated){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);

    /* Close popups on deactivate */
    if(!activated){
        struct wlr_xdg_popup* popup, *tmp;
        wl_list_for_each_safe(popup, tmp, &view->wlr_xdg_surface->popups, link){
            wlr_xdg_popup_destroy(popup->base);
        }
    }

    wlr_xdg_toplevel_set_activated(view->wlr_xdg_surface, activated);
}

static void wm_view_xdg_focus(struct wm_view* super, struct wm_seat* seat){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);
    wm_seat_focus_surface(seat, view->wlr_xdg_surface->surface);
}

static struct wlr_surface* wm_view_xdg_surface_at(struct wm_view* super, double at_x, double at_y, double* sx, double* sy){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);
    return wlr_xdg_surface_surface_at(view->wlr_xdg_surface, at_x, at_y, sx, sy);
}

static void wm_view_xdg_for_each_surface(struct wm_view* super, wlr_surface_iterator_func_t iterator, void* user_data){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);
    wlr_xdg_surface_for_each_surface(view->wlr_xdg_surface, iterator, user_data);
}

static bool wm_view_xdg_is_floating(struct wm_view* super){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);
    return view->floating;
}

static struct wm_view* wm_view_xdg_get_parent(struct wm_view* super){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);
    struct wlr_xdg_surface* parent_surface = view->wlr_xdg_surface->toplevel ? view->wlr_xdg_surface->toplevel->parent : NULL;
    if(!parent_surface){
        return NULL;
    }

    struct wm_content* parent;
    wl_list_for_each(parent, &view->super.super.wm_server->wm_contents, link){
        if(parent->vtable == view->super.super.vtable){
            struct wm_view* parent_view = wm_cast(wm_view, parent);
            if(parent_view->vtable == view->super.vtable){
                struct wm_view_xdg* parent_xdg = wm_cast(wm_view_xdg, parent_view);
                if(parent_xdg->wlr_xdg_surface == parent_surface){
                    return parent_view;
                }
            }
        }
    }

    wlr_log(WLR_DEBUG, "Warning! Could not find parent surface");
    return NULL;
}

struct wm_view_vtable wm_view_xdg_vtable = {
    .destroy = wm_view_xdg_destroy,
    .get_info = wm_view_xdg_get_info,
    .request_size = wm_view_xdg_request_size,
    .get_size_constraints = wm_view_xdg_get_size_constraints,
    .get_size = wm_view_xdg_get_size,
    .get_offset = wm_view_xdg_get_offset,
    .focus = wm_view_xdg_focus,
    .set_resizing = wm_view_xdg_set_resizing,
    .set_fullscreen = wm_view_xdg_set_fullscreen,
    .set_maximized = wm_view_xdg_set_maximized,
    .set_activated = wm_view_xdg_set_activated,
    .surface_at = wm_view_xdg_surface_at,
    .for_each_surface = wm_view_xdg_for_each_surface,
    .is_floating = wm_view_xdg_is_floating,
    .get_parent = wm_view_xdg_get_parent
};
