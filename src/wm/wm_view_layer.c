#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <stdlib.h>
#include <wayland-server.h>
#include <wlr/types/wlr_layer_shell_v1.h>

#include "wm/wm_view_layer.h"
#include "wm/wm_layout.h"
#include "wm/wm_output.h"
#include "wm/wm_server.h"
#include "wm/wm_util.h"
#include "wm/wm_seat.h"
#include "wm/wm.h"

struct wm_view_vtable wm_view_layer_vtable;

/*
 * Callbacks
 */
static void subsurface_handle_map(struct wl_listener* listener, void* data){
    struct wm_layer_subsurface* subsurface = wl_container_of(listener, subsurface, map);

    wm_layout_damage_from(
        subsurface->root->super.super.wm_server->wm_layout,
        &subsurface->root->super.super,
        subsurface->wlr_subsurface->surface);
}

static void subsurface_handle_unmap(struct wl_listener* listener, void* data){
    struct wm_layer_subsurface* subsurface = wl_container_of(listener, subsurface, unmap);

    wm_layout_damage_whole(subsurface->root->super.super.wm_server->wm_layout);
}

static void subsurface_handle_destroy(struct wl_listener* listener, void* data){
    struct wm_layer_subsurface* subsurface = wl_container_of(listener, subsurface, destroy);
    wm_layer_subsurface_destroy(subsurface);
    free(subsurface);
}

static void subsurface_handle_new_subsurface(struct wl_listener* listener, void* data){
    struct wm_layer_subsurface* subsurface = wl_container_of(listener, subsurface, new_subsurface);
    struct wlr_subsurface* wlr_subsurface = data;

    struct wm_layer_subsurface* new_subsurface = calloc(1, sizeof(struct wm_layer_subsurface));
    wm_layer_subsurface_init(new_subsurface, subsurface->root, wlr_subsurface);
    wl_list_insert(&subsurface->subsurfaces, &new_subsurface->link);
}

static void subsurface_handle_surface_commit(struct wl_listener* listener, void* data){
    struct wm_layer_subsurface* subsurface = wl_container_of(listener, subsurface, surface_commit);

    wm_layout_damage_from(
            subsurface->root->super.super.wm_server->wm_layout,
            &subsurface->root->super.super,
            subsurface->wlr_subsurface->surface
    );
}
static void popup_handle_map(struct wl_listener* listener, void* data){
    struct wm_popup_layer* popup = wl_container_of(listener, popup, map);

    wm_layout_damage_from(
        popup->root->super.super.wm_server->wm_layout,
        &popup->root->super.super,
        popup->wlr_xdg_popup->base->surface);
}

static void popup_handle_unmap(struct wl_listener* listener, void* data){
    struct wm_popup_layer* popup = wl_container_of(listener, popup, unmap);

    wm_layout_damage_whole(popup->root->super.super.wm_server->wm_layout);
}

static void popup_handle_destroy(struct wl_listener* listener, void* data){
    struct wm_popup_layer* popup = wl_container_of(listener, popup, destroy);
    wm_popup_layer_destroy(popup);
    free(popup);
}

static void popup_handle_new_popup(struct wl_listener* listener, void* data){
    struct wm_popup_layer* popup = wl_container_of(listener, popup, new_popup);
    struct wlr_xdg_popup* wlr_xdg_popup = data;

    struct wm_popup_layer* new_popup = calloc(1, sizeof(struct wm_popup_layer));
    wm_popup_layer_init(new_popup, popup->root, wlr_xdg_popup);
    wl_list_insert(&popup->popups, &new_popup->link);
}

static void popup_handle_new_subsurface(struct wl_listener* listener, void* data){
    struct wm_popup_layer* popup = wl_container_of(listener, popup, new_subsurface);
    struct wlr_subsurface* wlr_subsurface = data;

    struct wm_layer_subsurface* subsurface = calloc(1, sizeof(struct wm_layer_subsurface));
    wm_layer_subsurface_init(subsurface, popup->root, wlr_subsurface);
    wl_list_insert(&popup->subsurfaces, &subsurface->link);
}

static void popup_handle_surface_commit(struct wl_listener* listener, void* data){
    struct wm_popup_layer* popup = wl_container_of(listener, popup, surface_commit);

    wm_layout_damage_from(
            popup->root->super.super.wm_server->wm_layout,
            &popup->root->super.super,
            popup->wlr_xdg_popup->base->surface
    );
}

static void handle_map(struct wl_listener* listener, void* data){
    struct wm_view_layer* view = wl_container_of(listener, view, map);

    view->super.mapped = true;

    wm_layout_damage_from(
        view->super.super.wm_server->wm_layout,
        &view->super.super, NULL);
}


static void handle_unmap(struct wl_listener* listener, void* data){
    struct wm_view_layer* view = wl_container_of(listener, view, unmap);
    view->super.mapped = false;
    wm_layout_damage_whole(view->super.super.wm_server->wm_layout);
}

static void handle_destroy(struct wl_listener* listener, void* data){
    struct wm_view_layer* view = wl_container_of(listener, view, destroy);
    wm_callback_destroy_view(&view->super);

    wm_content_destroy(&view->super.super);
    free(view);
}


static void handle_new_popup(struct wl_listener* listener, void* data){
    struct wm_view_layer* view = wl_container_of(listener, view, new_popup);
    struct wlr_xdg_popup* wlr_xdg_popup = data;

    struct wm_popup_layer* popup = calloc(1, sizeof(struct wm_popup_layer));
    wm_popup_layer_init(popup, view, wlr_xdg_popup);
    wl_list_insert(&view->popups, &popup->link);
}

static void handle_new_subsurface(struct wl_listener* listener, void* data){
    struct wm_view_layer* view = wl_container_of(listener, view, new_subsurface);
    struct wlr_subsurface* wlr_subsurface = data;

    struct wm_layer_subsurface* subsurface = calloc(1, sizeof(struct wm_layer_subsurface));
    wm_layer_subsurface_init(subsurface, view, wlr_subsurface);
    wl_list_insert(&view->subsurfaces, &subsurface->link);
}

static void handle_surface_commit(struct wl_listener* listener, void* data){
    struct wm_view_layer* view = wl_container_of(listener, view, surface_commit);

    int width = view->wlr_layer_surface->surface->current.width;
    int height = view->wlr_layer_surface->surface->current.height;

    if(width != view->width || height != view->height){
        view->width = width;
        view->height = height;
        wm_callback_update_view(&view->super);
    }else{
        view->width = width;
        view->height = height;
    }

    /*
     * A bit hacky: Take care of setting output here
     */
    struct wm_output* output = wm_content_get_output(&view->super.super);
    if(output){
        view->wlr_layer_surface->output = output->wlr_output;
    }

    wm_layout_damage_from(
            view->super.super.wm_server->wm_layout,
            &view->super.super, view->wlr_layer_surface->surface);
}

/*
 * Class implementation wm_layer_subsurface
 */
void wm_layer_subsurface_init(struct wm_layer_subsurface* subsurface, struct wm_view_layer* root, struct wlr_subsurface* wlr_subsurface){
    subsurface->root = root;
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

void wm_layer_subsurface_destroy(struct wm_layer_subsurface* subsurface){
    wl_list_remove(&subsurface->link);
    wl_list_remove(&subsurface->subsurfaces);
    wl_list_remove(&subsurface->map.link);
    wl_list_remove(&subsurface->unmap.link);
    wl_list_remove(&subsurface->destroy.link);
    wl_list_remove(&subsurface->new_subsurface.link);
    wl_list_remove(&subsurface->surface_commit.link);
}

/*
 * Class implementation wm_popup_layer
 */
void wm_popup_layer_init(struct wm_popup_layer* popup, struct wm_view_layer* root, struct wlr_xdg_popup* wlr_xdg_popup){

    popup->wlr_xdg_popup = wlr_xdg_popup;
    popup->root = root;

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
    struct wlr_box* output_box = wlr_output_layout_get_box(
            popup->root->super.super.wm_server->wm_layout->wlr_output_layout, popup->root->wlr_layer_surface->output);
    struct wlr_box box = {
        .x = output_box->x -popup->root->super.super.display_x,
        .y = output_box->y -popup->root->super.super.display_y,
        .width = output_box->width,
        .height = output_box->height
    };
    wlr_xdg_popup_unconstrain_from_box(popup->wlr_xdg_popup, &box);

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

void wm_popup_layer_destroy(struct wm_popup_layer* popup){
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
 * Class implementation wm_view_layer
 */
void wm_view_layer_init(struct wm_view_layer* view, struct wm_server* server, struct wlr_layer_surface_v1* surface){
    wm_view_base_init(&view->super, server);
    view->super.vtable = &wm_view_layer_vtable;

    view->wlr_layer_surface = surface;

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

    wm_content_set_output(&view->super.super, -1, surface->output);

    const char* title;
    const char* app_id;
    const char* role;
    wm_view_get_info(&view->super, &title, &app_id, &role);
    wlr_log(WLR_DEBUG, "New wm_view (layer): %s", app_id);

    wm_callback_init_view(&view->super);
    wm_callback_update_view(&view->super);
}

static void wm_view_layer_destroy(struct wm_view* super){
    struct wm_view_layer* view = wm_cast(wm_view_layer, super);

    wl_list_remove(&view->subsurfaces);
    wl_list_remove(&view->popups);

    wl_list_remove(&view->map.link);
    wl_list_remove(&view->unmap.link);
    wl_list_remove(&view->destroy.link);
    wl_list_remove(&view->new_popup.link);
    wl_list_remove(&view->new_subsurface.link);

    wl_list_remove(&view->surface_commit.link);
}

static void wm_view_layer_get_credentials(struct wm_view* super, pid_t* pid, uid_t* uid, gid_t* gid){
    struct wm_view_layer* view = wm_cast(wm_view_layer, super);
    wl_client_get_credentials(view->wlr_layer_surface->resource->client, pid, uid, gid);
}

static void wm_view_layer_get_info(struct wm_view* super, const char** title, const char** app_id, const char** role){
    struct wm_view_layer* view = wm_cast(wm_view_layer, super);

    *title = "Layer Shell";
    *app_id = view->wlr_layer_surface->namespace;
    *role = "layer";
}

static void wm_view_layer_get_size_constraints(struct wm_view* super, int** size_constraints, int* n_constraints){
    struct wm_view_layer* view = wm_cast(wm_view_layer, super);

    view->size_constraints[0] = view->wlr_layer_surface->current.anchor;
    view->size_constraints[1] = view->wlr_layer_surface->current.desired_width;
    view->size_constraints[2] = view->wlr_layer_surface->current.desired_height;
    view->size_constraints[3] = view->wlr_layer_surface->current.exclusive_zone;
    view->size_constraints[4] = view->wlr_layer_surface->current.layer;
    view->size_constraints[5] = view->wlr_layer_surface->current.margin.left;
    view->size_constraints[6] = view->wlr_layer_surface->current.margin.top;
    view->size_constraints[7] = view->wlr_layer_surface->current.margin.right;
    view->size_constraints[8] = view->wlr_layer_surface->current.margin.bottom;

    *size_constraints = view->size_constraints;
    *n_constraints = 9;

}

static void wm_view_layer_get_offset(struct wm_view* super, int* offset_x, int* offset_y){
    *offset_x = 0;
    *offset_y = 0;
}

static void wm_view_layer_focus(struct wm_view* super, struct wm_seat* seat){
    struct wm_view_layer* view = wm_cast(wm_view_layer, super);
    wm_seat_focus_surface(seat, view->wlr_layer_surface->surface);
}

static void wm_view_layer_get_size(struct wm_view* super, int* width, int* height){
    struct wm_view_layer* view = wm_cast(wm_view_layer, super);

    *width = view->width;
    *height = view->height;
}

static void wm_view_layer_request_size(struct wm_view* super, int width, int height){
    struct wm_view_layer* view = wm_cast(wm_view_layer, super);
    wlr_layer_surface_v1_configure(view->wlr_layer_surface, width, height);
}

static void wm_view_layer_request_close(struct wm_view* super){
    /* noop */
}

static void wm_view_layer_set_resizing(struct wm_view* super, bool resizing){
    super->resizing = false;
}

static void wm_view_layer_set_fullscreen(struct wm_view* super, bool fullscreen){
    super->fullscreen = false;
}

static void wm_view_layer_set_maximized(struct wm_view* super, bool maximized){
    super->maximized = false;
}

static void wm_view_layer_set_activated(struct wm_view* super, bool activated){
    struct wm_view_layer* view = wm_cast(wm_view_layer, super);

    /* Close popups on deactivate */
    if(!activated){
        struct wlr_xdg_popup* popup, *tmp;
        wl_list_for_each_safe(popup, tmp, &view->wlr_layer_surface->popups, link){
            wlr_xdg_popup_destroy(popup->base);
        }
    }
}

static struct wlr_surface* wm_view_layer_surface_at(struct wm_view* super, double at_x, double at_y, double* sx, double* sy){
    struct wm_view_layer* view = wm_cast(wm_view_layer, super);
    return wlr_layer_surface_v1_surface_at(view->wlr_layer_surface, at_x, at_y, sx, sy);
}

static void wm_view_layer_for_each_surface(struct wm_view* super, wlr_surface_iterator_func_t iterator, void* user_data){
    struct wm_view_layer* view = wm_cast(wm_view_layer, super);
    wlr_layer_surface_v1_for_each_surface(view->wlr_layer_surface, iterator, user_data);
}

static void wm_view_layer_set_floating(struct wm_view* super, bool floating){
    /* noop */
}

static struct wm_view* wm_view_layer_get_parent(struct wm_view* super){
    return NULL;
}

static void wm_layer_subsurface_printf(FILE* file, struct wm_layer_subsurface* subsurface, int indent){
    fprintf(file, "%*swm_layer_subsurface for %p\n", indent, "", subsurface->wlr_subsurface->surface);

    struct wm_layer_subsurface* nsubsurface;
    wl_list_for_each(nsubsurface, &subsurface->subsurfaces, link){
        wm_layer_subsurface_printf(file, nsubsurface, indent + 2);
    }
}

static void wm_popup_layer_printf(FILE* file, struct wm_popup_layer* popup, int indent){
    fprintf(file, "%*swm_popup_xdg for %p\n", indent, "", popup->wlr_xdg_popup->base->surface);

    struct wm_popup_layer* npopup;
    wl_list_for_each(npopup, &popup->popups, link){
        wm_popup_layer_printf(file, npopup, indent + 2);
    }
    struct wm_layer_subsurface* subsurface;
    wl_list_for_each(subsurface, &popup->subsurfaces, link){
        wm_layer_subsurface_printf(file, subsurface, indent + 2);
    }
}

static void wm_view_layer_structure_printf(FILE* file, struct wm_view* super){
    struct wm_view_layer* view = wm_cast(wm_view_layer, super);

    fprintf(file, "  wm_view_layer for %p on output %p\n", view->wlr_layer_surface->surface, view->super.super.fixed_output);
    struct wm_popup_layer* popup;
    wl_list_for_each(popup, &view->popups, link){
        wm_popup_layer_printf(file, popup, 4);
    }
    struct wm_layer_subsurface* subsurface;
    wl_list_for_each(subsurface, &view->subsurfaces, link){
        wm_layer_subsurface_printf(file, subsurface, 4);
    }
}

struct wm_view_vtable wm_view_layer_vtable = {
    .destroy = wm_view_layer_destroy,
    .get_credentials = wm_view_layer_get_credentials,
    .get_info = wm_view_layer_get_info,
    .request_size = wm_view_layer_request_size,
    .request_close = wm_view_layer_request_close,
    .get_size_constraints = wm_view_layer_get_size_constraints,
    .get_size = wm_view_layer_get_size,
    .get_offset = wm_view_layer_get_offset,
    .focus = wm_view_layer_focus,
    .set_resizing = wm_view_layer_set_resizing,
    .set_fullscreen = wm_view_layer_set_fullscreen,
    .set_maximized = wm_view_layer_set_maximized,
    .set_activated = wm_view_layer_set_activated,
    .surface_at = wm_view_layer_surface_at,
    .for_each_surface = wm_view_layer_for_each_surface,
    .set_floating = wm_view_layer_set_floating,
    .get_parent = wm_view_layer_get_parent,
    .structure_printf = wm_view_layer_structure_printf
};
