#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <stdlib.h>
#include <wayland-server.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_subcompositor.h>

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
 * Debug all relevant size / resize information for given app_id
 */
/* #define DEBUG_SIZE "catapult" */
/* #define DEBUG_SIZE "obs" */
/* #define DEBUG_SIZE "libreoffice-startcenter" */
/* #define DEBUG_SIZE "soffice" */


/*
 * Callbacks
 */
static void subsurface_handle_map(struct wl_listener* listener, void* data){
    struct wm_xdg_subsurface* subsurface = wl_container_of(listener, subsurface, map);

    wm_layout_damage_from(
        subsurface->toplevel->super.super.wm_server->wm_layout,
        &subsurface->toplevel->super.super,
        subsurface->wlr_subsurface->surface);
}

static void subsurface_handle_unmap(struct wl_listener* listener, void* data){
    struct wm_xdg_subsurface* subsurface = wl_container_of(listener, subsurface, unmap);

    wm_layout_damage_whole(subsurface->toplevel->super.super.wm_server->wm_layout);
}

static void subsurface_handle_destroy(struct wl_listener* listener, void* data){
    struct wm_xdg_subsurface* subsurface = wl_container_of(listener, subsurface, destroy);
    wm_xdg_subsurface_destroy(subsurface);
    free(subsurface);
}

static void subsurface_handle_new_subsurface(struct wl_listener* listener, void* data){
    struct wm_xdg_subsurface* subsurface = wl_container_of(listener, subsurface, new_subsurface);
    struct wlr_subsurface* wlr_subsurface = data;

    struct wm_xdg_subsurface* new_subsurface = calloc(1, sizeof(struct wm_xdg_subsurface));
    wm_xdg_subsurface_init(new_subsurface, subsurface->toplevel, wlr_subsurface);
    wl_list_insert(&subsurface->subsurfaces, &new_subsurface->link);
}

static void subsurface_handle_surface_commit(struct wl_listener* listener, void* data){
    struct wm_xdg_subsurface* subsurface = wl_container_of(listener, subsurface, surface_commit);

    wm_layout_damage_from(
            subsurface->toplevel->super.super.wm_server->wm_layout,
            &subsurface->toplevel->super.super,
            subsurface->wlr_subsurface->surface
    );
}
static void popup_handle_map(struct wl_listener* listener, void* data){
    struct wm_popup_xdg* popup = wl_container_of(listener, popup, map);

    wm_layout_damage_from(
        popup->toplevel->super.super.wm_server->wm_layout,
        &popup->toplevel->super.super,
        popup->wlr_xdg_popup->base->surface);
}

static void popup_handle_unmap(struct wl_listener* listener, void* data){
    struct wm_popup_xdg* popup = wl_container_of(listener, popup, unmap);

    wm_layout_damage_whole(popup->toplevel->super.super.wm_server->wm_layout);
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
    wl_list_insert(&popup->popups, &new_popup->link);
}

static void popup_handle_new_subsurface(struct wl_listener* listener, void* data){
    struct wm_popup_xdg* popup = wl_container_of(listener, popup, new_subsurface);
    struct wlr_subsurface* wlr_subsurface = data;

    struct wm_xdg_subsurface* subsurface = calloc(1, sizeof(struct wm_xdg_subsurface));
    wm_xdg_subsurface_init(subsurface, popup->toplevel, wlr_subsurface);
    wl_list_insert(&popup->subsurfaces, &subsurface->link);
}

static void popup_handle_surface_commit(struct wl_listener* listener, void* data){
    struct wm_popup_xdg* popup = wl_container_of(listener, popup, surface_commit);

    wm_layout_damage_from(
            popup->toplevel->super.super.wm_server->wm_layout,
            &popup->toplevel->super.super,
            popup->wlr_xdg_popup->base->surface
    );
}



static void handle_map(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, map);

    view->super.mapped = true;

    wm_layout_damage_from(
        view->super.super.wm_server->wm_layout,
        &view->super.super, NULL);
}


static void handle_unmap(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, unmap);
    view->super.mapped = false;

    wm_layout_damage_whole(view->super.super.wm_server->wm_layout);
}

static void handle_destroy(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, destroy);
    wm_callback_destroy_view(&view->super);
    wm_content_destroy(&view->super.super);
    free(view);
}


static void handle_new_popup(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, new_popup);
    struct wlr_xdg_popup* wlr_xdg_popup = data;

    struct wm_popup_xdg* popup = calloc(1, sizeof(struct wm_popup_xdg));
    wm_popup_xdg_init(popup, view, wlr_xdg_popup);
    wl_list_insert(&view->popups, &popup->link);
}

static void handle_new_subsurface(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, new_subsurface);
    struct wlr_subsurface* wlr_subsurface = data;

    struct wm_xdg_subsurface* subsurface = calloc(1, sizeof(struct wm_xdg_subsurface));
    wm_xdg_subsurface_init(subsurface, view, wlr_subsurface);
    wl_list_insert(&view->subsurfaces, &subsurface->link);
}

static bool possibly_update_size(struct wm_view_xdg* view){
    /* Possible sizes to use */
    int xdg_surface_width = view->wlr_xdg_surface->current.geometry.width;
    int xdg_surface_height = view->wlr_xdg_surface->current.geometry.height;

    int toplevel_width = view->wlr_xdg_surface->toplevel->current.width;
    int toplevel_height = view->wlr_xdg_surface->toplevel->current.height;

    int surface_width = view->wlr_xdg_surface->surface->current.width;
    int surface_height = view->wlr_xdg_surface->surface->current.height;

    /* Decide on size */
    int width = toplevel_width;
    int height = toplevel_height;

    if(!width || !height){
        width = xdg_surface_width;
        height = xdg_surface_height;
    }

    if(!width || !height){
        width = surface_width;
        height = surface_height;
    }

#ifdef DEBUG_SIZE
    const char *title, *app_id, *role;
    wm_view_get_info(&view->super, &title, &app_id, &role);
    if(app_id && !strcmp(app_id, DEBUG_SIZE)){
        wlr_log(WLR_DEBUG, "DEBUG_SIZE: update        %dx%d (toplevel=%dx%d, xdg_surface=%dx%d, surface=%dx%d)",
                width, height, toplevel_width, toplevel_height, xdg_surface_width, xdg_surface_height, surface_width, surface_height);
    }
#endif

    /* Update size */
    if(width != view->width || height != view->height){
        view->width = width;
        view->height = height;
        return true;
    }

    return false;
}

static void handle_surface_configure(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, surface_configure);

#ifdef DEBUG_SIZE
    const char *title, *app_id, *role;
    wm_view_get_info(&view->super, &title, &app_id, &role);
    if(app_id && !strcmp(app_id, DEBUG_SIZE)){
        wlr_log(WLR_DEBUG, "DEBUG_SIZE: configure");
    }
#endif
}


static void handle_surface_ack_configure(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, surface_ack_configure);

#ifdef DEBUG_SIZE
    const char *title, *app_id, *role;
    wm_view_get_info(&view->super, &title, &app_id, &role);
    if(app_id && !strcmp(app_id, DEBUG_SIZE)){
        wlr_log(WLR_DEBUG, "DEBUG_SIZE: ack_configure");
    }
#endif

    possibly_update_size(view);
}

static void handle_surface_commit(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, surface_commit);

    bool update = false;
    if(!view->initialized){
        wm_callback_init_view(&view->super);
        view->initialized = true;
        update = true;
    }

#ifdef DEBUG_SIZE
    const char *title, *app_id, *role;
    wm_view_get_info(&view->super, &title, &app_id, &role);
    if(app_id && !strcmp(app_id, DEBUG_SIZE)){
        wlr_log(WLR_DEBUG, "DEBUG_SIZE: commit");
    }
#endif

    update |= possibly_update_size(view);

    if(update){
        wm_callback_update_view(&view->super);
    }

    wm_layout_damage_from(
            view->super.super.wm_server->wm_layout,
            &view->super.super, view->wlr_xdg_surface->surface);
}

static void handle_fullscreen(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, request_fullscreen);

    if(view->wlr_xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) return;

    if(view->wlr_xdg_surface->toplevel->requested.fullscreen){
        wm_callback_view_event(&view->super, "request_fullscreen");
    }else{
        wm_callback_view_event(&view->super, "request_nofullscreen");
    }
}

static void handle_move(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, request_move);

    wm_callback_view_event(&view->super, "request_move");
}

static void handle_resize(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, request_resize);

    wm_callback_view_event(&view->super, "request_resize");
}

static void handle_minimize(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, request_minimize);

    wm_callback_view_event(&view->super, "request_minimize");
}

static void handle_maximize(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, request_maximize);

    wm_callback_view_event(&view->super, "request_maximize");
}

static void handle_show_window_menu(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, request_show_window_menu);

    wm_callback_view_event(&view->super, "request_show_window_menu");
}

/*
 * Class implementation wm_xdg_subsurface
 */
void wm_xdg_subsurface_init(struct wm_xdg_subsurface* subsurface, struct wm_view_xdg* toplevel, struct wlr_subsurface* wlr_subsurface){
    subsurface->toplevel = toplevel;
    subsurface->wlr_subsurface = wlr_subsurface;

    wl_list_init(&subsurface->subsurfaces);

    subsurface->map.notify = &subsurface_handle_map;
    wl_signal_add(&wlr_subsurface->events.map, &subsurface->map);

    subsurface->unmap.notify = &subsurface_handle_unmap;
    wl_signal_add(&wlr_subsurface->events.unmap, &subsurface->unmap);

    subsurface->destroy.notify = &subsurface_handle_destroy;
    wl_signal_add(&wlr_subsurface->events.destroy, &subsurface->destroy);

    subsurface->new_subsurface.notify = &subsurface_handle_new_subsurface;
    wl_signal_add(&wlr_subsurface->surface->events.new_subsurface, &subsurface->new_subsurface);

    subsurface->surface_commit.notify = &subsurface_handle_surface_commit;
    wl_signal_add(&wlr_subsurface->surface->events.commit, &subsurface->surface_commit);

    struct wlr_subsurface* ss;
    wl_list_for_each(ss, &wlr_subsurface->surface->current.subsurfaces_below, current.link){
        wlr_log(WLR_DEBUG, "Subsurface: Adding \"old\" subsurface (below)");
        subsurface_handle_new_subsurface(&subsurface->new_subsurface, ss);
    }
    wl_list_for_each(ss, &wlr_subsurface->surface->current.subsurfaces_above, current.link){
        wlr_log(WLR_DEBUG, "Subsurface: Adding \"old\" subsurface (above)");
        subsurface_handle_new_subsurface(&subsurface->new_subsurface, ss);
    }

}

void wm_xdg_subsurface_destroy(struct wm_xdg_subsurface* subsurface){
    wl_list_remove(&subsurface->link);
    wl_list_remove(&subsurface->subsurfaces);
    wl_list_remove(&subsurface->map.link);
    wl_list_remove(&subsurface->unmap.link);
    wl_list_remove(&subsurface->destroy.link);
    wl_list_remove(&subsurface->new_subsurface.link);
    wl_list_remove(&subsurface->surface_commit.link);
}


/*
 * Class implementation wm_popup_xdg
 */
void wm_popup_xdg_init(struct wm_popup_xdg* popup, struct wm_view_xdg* toplevel, struct wlr_xdg_popup* wlr_xdg_popup){

    popup->wlr_xdg_popup = wlr_xdg_popup;
    popup->toplevel = toplevel;

    wl_list_init(&popup->subsurfaces);
    wl_list_init(&popup->popups);

    popup->map.notify = &popup_handle_map;
    wl_signal_add(&wlr_xdg_popup->base->events.map, &popup->map);

    popup->unmap.notify = &popup_handle_unmap;
    wl_signal_add(&wlr_xdg_popup->base->events.unmap, &popup->unmap);

    popup->destroy.notify = &popup_handle_destroy;
    wl_signal_add(&wlr_xdg_popup->base->events.destroy, &popup->destroy);

    popup->new_popup.notify = &popup_handle_new_popup;
    wl_signal_add(&wlr_xdg_popup->base->events.new_popup, &popup->new_popup);

    popup->new_subsurface.notify = &popup_handle_new_subsurface;
    wl_signal_add(&wlr_xdg_popup->base->surface->events.new_subsurface, &popup->new_subsurface);

    popup->surface_commit.notify = &popup_handle_surface_commit;
    wl_signal_add(&wlr_xdg_popup->base->surface->events.commit, &popup->surface_commit);

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
        struct wlr_box output_box;
        wlr_output_layout_get_box(
                popup->toplevel->super.super.wm_server->wm_layout->wlr_output_layout, NULL, &output_box);

        double x_scale = width / popup->toplevel->super.super.display_width;
        double y_scale = height / popup->toplevel->super.super.display_height;
        struct wlr_box box = {
            .x = output_box.x -popup->toplevel->super.super.display_x * x_scale,
            .y = output_box.y -popup->toplevel->super.super.display_y * y_scale,
            .width = output_box.width * x_scale,
            .height = output_box.height * y_scale
        };
        wlr_xdg_popup_unconstrain_from_box(popup->wlr_xdg_popup, &box);
    }

    struct wlr_subsurface* ss;
    wl_list_for_each(ss, &wlr_xdg_popup->base->surface->current.subsurfaces_below, current.link){
        wlr_log(WLR_DEBUG, "Popup: Adding \"old\" subsurface (below)");
        popup_handle_new_subsurface(&popup->new_subsurface, ss);
    }
    wl_list_for_each(ss, &wlr_xdg_popup->base->surface->current.subsurfaces_above, current.link){
        wlr_log(WLR_DEBUG, "Popup: Adding \"old\" subsurface (above)");
        popup_handle_new_subsurface(&popup->new_subsurface, ss);
    }
}

void wm_popup_xdg_destroy(struct wm_popup_xdg* popup){
    wl_list_remove(&popup->link);
    wl_list_remove(&popup->popups);
    wl_list_remove(&popup->subsurfaces);

    wl_list_remove(&popup->map.link);
    wl_list_remove(&popup->unmap.link);
    wl_list_remove(&popup->destroy.link);
    wl_list_remove(&popup->new_popup.link);
    wl_list_remove(&popup->new_subsurface.link);
    wl_list_remove(&popup->surface_commit.link);
}


/*
 * Class implementation wm_view_xdg
 */
void wm_view_xdg_init(struct wm_view_xdg* view, struct wm_server* server, struct wlr_xdg_surface* surface){
    wm_view_base_init(&view->super, server);
    view->super.vtable = &wm_view_xdg_vtable;
    view->super.floating = false;
    view->floating_set = -1;

    view->wlr_xdg_surface = surface;
    view->wlr_deco = NULL;
    view->wlr_server_deco = NULL;

    wl_list_init(&view->popups);
    wl_list_init(&view->subsurfaces);

    view->map.notify = &handle_map;
    wl_signal_add(&surface->events.map, &view->map);

    view->unmap.notify = &handle_unmap;
    wl_signal_add(&surface->events.unmap, &view->unmap);

    view->destroy.notify = &handle_destroy;
    wl_signal_add(&surface->events.destroy, &view->destroy);

    view->new_popup.notify = &handle_new_popup;
    wl_signal_add(&surface->events.new_popup, &view->new_popup);

    view->new_subsurface.notify = &handle_new_subsurface;
    wl_signal_add(&surface->surface->events.new_subsurface, &view->new_subsurface);

    view->surface_commit.notify = &handle_surface_commit;
    wl_signal_add(&surface->surface->events.commit, &view->surface_commit);

    view->surface_configure.notify = &handle_surface_configure;
    wl_signal_add(&surface->events.configure, &view->surface_configure);

    view->surface_ack_configure.notify = &handle_surface_ack_configure;
    wl_signal_add(&surface->events.ack_configure, &view->surface_ack_configure);

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

    view->initialized = false;

    view->constrain_popups_to_toplevel = server->wm_config->constrain_popups_to_toplevel;

    const char* title;
    const char* app_id;
    const char* role;
    wm_view_get_info(&view->super, &title, &app_id, &role);
    wlr_log(WLR_DEBUG, "New wm_view (xdg): %s, %s, %s", title, app_id, role);

    struct wlr_subsurface* ss;
    wl_list_for_each(ss, &surface->surface->current.subsurfaces_below, current.link){
        wlr_log(WLR_DEBUG, "View: Adding \"old\" subsurface (below)");
        handle_new_subsurface(&view->new_subsurface, ss);
    }
    wl_list_for_each(ss, &surface->surface->current.subsurfaces_above, current.link){
        wlr_log(WLR_DEBUG, "View: Adding \"old\" subsurface (above)");
        handle_new_subsurface(&view->new_subsurface, ss);
    }
}

static void wm_view_xdg_destroy(struct wm_view* super){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);

    wl_list_remove(&view->subsurfaces);
    wl_list_remove(&view->popups);

    wl_list_remove(&view->map.link);
    wl_list_remove(&view->unmap.link);
    wl_list_remove(&view->destroy.link);
    wl_list_remove(&view->new_popup.link);
    wl_list_remove(&view->new_subsurface.link);

    wl_list_remove(&view->surface_commit.link);
    wl_list_remove(&view->surface_configure.link);
    wl_list_remove(&view->surface_ack_configure.link);

    wl_list_remove(&view->request_fullscreen.link);
    wl_list_remove(&view->request_move.link);
    wl_list_remove(&view->request_resize.link);
    wl_list_remove(&view->request_maximize.link);
    wl_list_remove(&view->request_minimize.link);
    wl_list_remove(&view->request_show_window_menu.link);
}

static void wm_view_xdg_get_credentials(struct wm_view* super, pid_t* pid, uid_t* uid, gid_t* gid){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);
    wl_client_get_credentials(view->wlr_xdg_surface->client->client, pid, uid, gid);
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
#ifdef DEBUG_SIZE
        const char *title, *app_id, *role;
        wm_view_get_info(super, &title, &app_id, &role);
        if(app_id && !strcmp(app_id, DEBUG_SIZE)){
            wlr_log(WLR_DEBUG, "DEBUG_SIZE: Request %dx%d", width, height);
        }
#endif
        wlr_xdg_toplevel_set_size(view->wlr_xdg_surface->toplevel, width, height);
    }else{
        wlr_log(WLR_DEBUG, "Warning: Not toplevel");
    }
}

static void wm_view_xdg_get_size_constraints(struct wm_view* super, int** constraints, int* n_constraints){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);

    if(view->wlr_xdg_surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL){
        view->size_constraints[0] = view->wlr_xdg_surface->toplevel->current.min_width;
        view->size_constraints[1] = view->wlr_xdg_surface->toplevel->current.max_width;
        view->size_constraints[2] = view->wlr_xdg_surface->toplevel->current.min_height;
        view->size_constraints[3] = view->wlr_xdg_surface->toplevel->current.max_height;

    }else{
        wlr_log(WLR_DEBUG, "Warning: Not toplevel");
        view->size_constraints[0] = -1;
        view->size_constraints[1] = -1;
        view->size_constraints[2] = -1;
        view->size_constraints[3] = -1;
    }

    *constraints = view->size_constraints;
    *n_constraints = 4;
}

static void wm_view_xdg_get_size(struct wm_view* super, int* width, int* height){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);

#ifdef DEBUG_SIZE
        int w = view->wlr_xdg_surface->toplevel->current.width;
        int h = view->wlr_xdg_surface->toplevel->current.height;

        if(!w || !h){
            w = view->wlr_xdg_surface->surface->current.width;
            h = view->wlr_xdg_surface->surface->current.height;
        }

        if(w != view->width || h != view->height){
            const char *title, *app_id, *role;
            wm_view_get_info(super, &title, &app_id, &role);
            if(app_id && !strcmp(app_id, DEBUG_SIZE)){
                wlr_log(WLR_DEBUG, "DEBUG_SIZE: get %dx%d", w, h);
            }
        }
#endif

    *width = view->width;
    *height = view->height;
}

static void wm_view_xdg_get_offset(struct wm_view* super, int* offset_x, int* offset_y){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);

    *offset_x = view->wlr_xdg_surface->current.geometry.x;
    *offset_y = view->wlr_xdg_surface->current.geometry.y;
}

static void wm_view_xdg_set_floating(struct wm_view* super, bool floating){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);
    if(floating == view->floating_set) return;

    wlr_xdg_toplevel_set_tiled(view->wlr_xdg_surface->toplevel, floating ? 0 : 15);
    view->floating_set = floating;
}

static void wm_view_xdg_set_resizing(struct wm_view* super, bool resizing){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);
    wlr_xdg_toplevel_set_resizing(view->wlr_xdg_surface->toplevel, resizing);
}

static void wm_view_xdg_set_fullscreen(struct wm_view* super, bool fullscreen){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);
    wlr_xdg_toplevel_set_fullscreen(view->wlr_xdg_surface->toplevel, fullscreen);
}

static void wm_view_xdg_set_maximized(struct wm_view* super, bool maximized){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);
    wlr_xdg_toplevel_set_maximized(view->wlr_xdg_surface->toplevel, maximized);
}

static void wm_view_xdg_set_activated(struct wm_view* super, bool activated){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);

    /* Close popups on deactivate */
    if(!activated){
        struct wlr_xdg_popup* popup, *tmp;
        wl_list_for_each_safe(popup, tmp, &view->wlr_xdg_surface->popups, link){
            wlr_xdg_popup_destroy(popup);
        }
    }

    wlr_xdg_toplevel_set_activated(view->wlr_xdg_surface->toplevel, activated);
}

static void wm_view_xdg_request_close(struct wm_view* super){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);
    wlr_xdg_toplevel_send_close(view->wlr_xdg_surface->toplevel);
}

static void wm_view_xdg_focus(struct wm_view* super, struct wm_seat* seat){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);
    wm_seat_focus_surface(seat, view->wlr_xdg_surface->surface);
}

static struct wlr_surface* wm_view_xdg_surface_at(struct wm_view* super, double at_x, double at_y, double* sx, double* sy){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);
    return wlr_xdg_surface_surface_at(view->wlr_xdg_surface, at_x, at_y, sx, sy);
}

struct for_each_surface_data {
    wm_surface_iterator_func_t iterator;
    void* user_data;
    struct wlr_surface* constrained_root;
};

static void call_surface_iterator(struct wlr_surface* surface, int sx, int sy, void* data){
    struct for_each_surface_data* fedata = data;
    struct wlr_surface* root = surface;
    while(root && wlr_surface_is_subsurface(root)){
        root = wlr_subsurface_from_wlr_surface(root)->parent;
    }
    fedata->iterator(surface, sx, sy, root == fedata->constrained_root, fedata->user_data);
}

static void wm_view_xdg_for_each_surface(struct wm_view* super, wm_surface_iterator_func_t iterator, void* user_data){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);
    struct for_each_surface_data data = {
        .iterator = iterator,
        .user_data = user_data,
        .constrained_root = view->wlr_xdg_surface->surface
    };
    wlr_xdg_surface_for_each_surface(view->wlr_xdg_surface, call_surface_iterator, &data);
}

static struct wm_view* wm_view_xdg_get_parent(struct wm_view* super){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);
    struct wlr_xdg_surface* parent_surface = (view->wlr_xdg_surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL && view->wlr_xdg_surface->toplevel->parent) ? view->wlr_xdg_surface->toplevel->parent->base : NULL;
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

static void wm_xdg_subsurface_printf(FILE* file, struct wm_xdg_subsurface* subsurface, int indent){
    fprintf(file, "%*swm_xdg_subsurface for %p\n", indent, "", subsurface->wlr_subsurface->surface);

    struct wm_xdg_subsurface* nsubsurface;
    wl_list_for_each(nsubsurface, &subsurface->subsurfaces, link){
        wm_xdg_subsurface_printf(file, nsubsurface, indent + 2);
    }
}

static void wm_popup_xdg_printf(FILE* file, struct wm_popup_xdg* popup, int indent){
    fprintf(file, "%*swm_popup_xdg for %p\n", indent, "", popup->wlr_xdg_popup->base->surface);

    struct wm_popup_xdg* npopup;
    wl_list_for_each(npopup, &popup->popups, link){
        wm_popup_xdg_printf(file, npopup, indent + 2);
    }
    struct wm_xdg_subsurface* subsurface;
    wl_list_for_each(subsurface, &popup->subsurfaces, link){
        wm_xdg_subsurface_printf(file, subsurface, indent + 2);
    }
}

static void wm_view_xdg_structure_printf(FILE* file, struct wm_view* super){
    struct wm_view_xdg* view = wm_cast(wm_view_xdg, super);

    fprintf(file, "  wm_view_xdg for %p\n", view->wlr_xdg_surface->surface);
    struct wm_popup_xdg* popup;
    wl_list_for_each(popup, &view->popups, link){
        wm_popup_xdg_printf(file, popup, 4);
    }
    struct wm_xdg_subsurface* subsurface;
    wl_list_for_each(subsurface, &view->subsurfaces, link){
        wm_xdg_subsurface_printf(file, subsurface, 4);
    }
}

bool wm_view_is_xdg(struct wm_view* view){
    return view->vtable == &wm_view_xdg_vtable;
}

static bool wm_view_xdg_encourage_csd(struct wm_view_xdg* view){
    if(wm_view_is_floating(&view->super)){
        return true;
    }else{
        return view->super.super.wm_server->wm_config->encourage_csd;
    }
}

static void server_deco_handle_mode(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, server_deco_mode);

    wlr_log(WLR_DEBUG, "server decoration mode: %s", view->wlr_server_deco->mode == WLR_SERVER_DECORATION_MANAGER_MODE_SERVER ? "SSD" :
            (view->wlr_server_deco->mode == WLR_SERVER_DECORATION_MANAGER_MODE_CLIENT ? "CSD" : ""));

    view->super.shows_csd = view->wlr_server_deco->mode == WLR_SERVER_DECORATION_MANAGER_MODE_CLIENT;
}

static void server_deco_handle_destroy(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, server_deco_destroy);
    wl_list_remove(&view->server_deco_mode.link);
    wl_list_remove(&view->server_deco_destroy.link);
    view->wlr_server_deco = NULL;
}

void wm_view_xdg_register_server_decoration(struct wm_view_xdg* view, struct wlr_server_decoration* wlr_deco){
    view->wlr_server_deco = wlr_deco;
    wlr_log(WLR_DEBUG, "new server decoration: %s", wlr_deco->mode == WLR_SERVER_DECORATION_MANAGER_MODE_SERVER ? "SSD" :
            (wlr_deco->mode == WLR_SERVER_DECORATION_MANAGER_MODE_CLIENT ? "CSD" : ""));

    view->server_deco_mode.notify = &server_deco_handle_mode;
    wl_signal_add(&wlr_deco->events.mode, &view->server_deco_mode);

    view->server_deco_destroy.notify = &server_deco_handle_destroy;
    wl_signal_add(&wlr_deco->events.destroy, &view->server_deco_destroy);

    view->super.shows_csd = wlr_deco->mode == WLR_SERVER_DECORATION_MANAGER_MODE_CLIENT;
}

static void deco_handle_request_mode(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, deco_request_mode);

    wlr_log(WLR_DEBUG, "xdg decoration request: %s (setting default)", view->wlr_deco->requested_mode == WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE ? "SSD" :
            (view->wlr_deco->requested_mode == WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE ? "CSD" : ""));

    wlr_xdg_toplevel_decoration_v1_set_mode(view->wlr_deco, wm_view_xdg_encourage_csd(view) ?
            WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE :
            WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);

    view->super.shows_csd = wm_view_xdg_encourage_csd(view);
}

static void deco_handle_destroy(struct wl_listener* listener, void* data){
    struct wm_view_xdg* view = wl_container_of(listener, view, deco_destroy);
    wl_list_remove(&view->deco_request_mode.link);
    wl_list_remove(&view->deco_destroy.link);
    view->wlr_deco = NULL;
}

void wm_view_xdg_register_decoration(struct wm_view_xdg* view, struct wlr_xdg_toplevel_decoration_v1* wlr_deco){
    view->wlr_deco = wlr_deco;

    wlr_log(WLR_DEBUG, "new xdg decoration: %s (setting default)", wlr_deco->current.mode == WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE ? "SSD" :
            (wlr_deco->current.mode == WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE ? "CSD" : ""));

    wlr_xdg_toplevel_decoration_v1_set_mode(view->wlr_deco, wm_view_xdg_encourage_csd(view) ?
            WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE :
            WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);


    view->deco_request_mode.notify = &deco_handle_request_mode;
    wl_signal_add(&wlr_deco->events.request_mode, &view->deco_request_mode);

    view->deco_destroy.notify = &deco_handle_destroy;
    wl_signal_add(&wlr_deco->events.destroy, &view->deco_destroy);
}

struct wm_view_vtable wm_view_xdg_vtable = {
    .destroy = wm_view_xdg_destroy,
    .get_credentials = wm_view_xdg_get_credentials,
    .get_info = wm_view_xdg_get_info,
    .request_size = wm_view_xdg_request_size,
    .request_close = wm_view_xdg_request_close,
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
    .set_floating = wm_view_xdg_set_floating,
    .get_parent = wm_view_xdg_get_parent,
    .structure_printf = wm_view_xdg_structure_printf
};
