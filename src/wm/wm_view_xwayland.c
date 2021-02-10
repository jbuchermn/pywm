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

struct wm_view_vtable wm_view_xwayland_vtable;

static void try_to_find_parent(struct wm_view_xwayland* view){
    struct wm_view_xwayland* parent = NULL;
    
    if(view->wlr_xwayland_surface->parent){
        unsigned int parent_id = view->wlr_xwayland_surface->parent->window_id;
        struct wm_content* it;
        wl_list_for_each(it, &view->super.super.wm_server->wm_contents, link){
            if(it->vtable != view->super.super.vtable) continue;
            struct wm_view* it_view = wm_cast(wm_view, it);
            if(it_view->vtable != view->super.vtable) continue;
            struct wm_view_xwayland* it_xwayland = wm_cast(wm_view_xwayland, it_view);
            if(it_xwayland->wlr_xwayland_surface->window_id == parent_id){
                parent = it_xwayland;
                goto Found;
            }

            struct wm_view_xwayland_child* sibling;
            wl_list_for_each(sibling, &it_xwayland->children, link){
                if(sibling->wlr_xwayland_surface->window_id == parent_id){
                    parent = it_xwayland;
                    goto Found;
                }
            }
        }
    }

    if(view->wlr_xwayland_surface->pid){
        struct wm_content* it;
        wl_list_for_each(it, &view->super.super.wm_server->wm_contents, link){
            if(it->vtable != view->super.super.vtable) continue;
            struct wm_view* it_view = wm_cast(wm_view, it);
            if(!it_view->mapped) continue;

            if(it_view->vtable != view->super.vtable) continue;
            struct wm_view_xwayland* it_xwayland = wm_cast(wm_view_xwayland, it_view);
            if(it_xwayland == view) continue;

            if(it_xwayland->wlr_xwayland_surface->pid == view->wlr_xwayland_surface->pid){
                parent = it_xwayland;
                goto Found;
            }
        }
    }

Found:

    if(parent){
        if(view->wlr_xwayland_surface->modal){
            /* Child view */
            view->floating = true;
            view->parent = parent;
        }else{
            /* Tooltip, context-menu, more of a subsurface than a sub-view */
            struct wm_view_xwayland_child* child = calloc(1, sizeof(struct wm_view_xwayland_child));

            wm_view_xwayland_child_init(child, parent, view->wlr_xwayland_surface);
            wm_content_destroy(&view->super.super);
            free(view);
        }
    }
}

/*
 * Callbacks
 */
static void child_handle_map(struct wl_listener* listener, void* data){
    struct wm_view_xwayland_child* child = wl_container_of(listener, child, map);
    child->mapped = true;
}

static void child_handle_request_configure(struct wl_listener* listener, void* data){
    struct wm_view_xwayland_child* child = wl_container_of(listener, child, request_configure);
    struct wlr_xwayland_surface_configure_event* event = data;

    wlr_xwayland_surface_configure(child->wlr_xwayland_surface, event->x, event->y, event->width, event->height);
}

static void child_handle_unmap(struct wl_listener* listener, void* data){
    struct wm_view_xwayland_child* child = wl_container_of(listener, child, unmap);
    child->mapped = false;
}

static void child_handle_destroy(struct wl_listener* listener, void* data){
    struct wm_view_xwayland_child* child = wl_container_of(listener, child, destroy);
    wm_view_xwayland_child_destroy(child);
}

static void handle_request_configure(struct wl_listener* listener, void* data){
    struct wm_view_xwayland* view = wl_container_of(listener, view, request_configure);
    struct wlr_xwayland_surface_configure_event* event = data;

    wlr_xwayland_surface_configure(view->wlr_xwayland_surface, 0., 0., event->width, event->height);
}

static void handle_set_pid(struct wl_listener* listener, void* data){
    struct wm_view_xwayland* view = wl_container_of(listener, view, set_pid);

    try_to_find_parent(view);
}

static void handle_set_parent(struct wl_listener* listener, void* data){
    struct wm_view_xwayland* view = wl_container_of(listener, view, set_parent);

    try_to_find_parent(view);
}

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
    wm_content_destroy(&view->super.super);

    free(view);
}


/*
 * Class implementation
 */
void wm_view_xwayland_child_init(struct wm_view_xwayland_child* child, struct wm_view_xwayland* parent, struct wlr_xwayland_surface* surface){
    child->parent = parent;
    child->wlr_xwayland_surface = surface;

    child->map.notify = &child_handle_map;
    wl_signal_add(&surface->events.map, &child->map);

    child->request_configure.notify = &child_handle_request_configure;
    wl_signal_add(&surface->events.request_configure, &child->request_configure);

    child->unmap.notify = &child_handle_unmap;
    wl_signal_add(&surface->events.unmap, &child->unmap);

    child->destroy.notify = &child_handle_destroy;
    wl_signal_add(&surface->events.destroy, &child->destroy);

    wl_list_insert(&parent->children, &child->link);
}

void wm_view_xwayland_child_destroy(struct wm_view_xwayland_child* child){
    wl_list_remove(&child->request_configure.link);
    wl_list_remove(&child->map.link);
    wl_list_remove(&child->unmap.link);
    wl_list_remove(&child->destroy.link);

    wl_list_remove(&child->link);
}

void wm_view_xwayland_init(struct wm_view_xwayland* view, struct wm_server* server, struct wlr_xwayland_surface* surface){
    wm_view_base_init(&view->super, server);

    view->super.vtable = &wm_view_xwayland_vtable;

    view->parent = NULL;
    view->floating = false;
    wl_list_init(&view->children);

    view->wlr_xwayland_surface = surface;

    view->request_configure.notify = &handle_request_configure;
    wl_signal_add(&surface->events.request_configure, &view->request_configure);

    view->set_parent.notify = &handle_set_parent;
    wl_signal_add(&surface->events.set_parent, &view->set_parent);

    view->set_pid.notify = &handle_set_pid;
    wl_signal_add(&surface->events.set_pid, &view->set_pid);

    view->map.notify = &handle_map;
    wl_signal_add(&surface->events.map, &view->map);

    view->unmap.notify = &handle_unmap;
    wl_signal_add(&surface->events.unmap, &view->unmap);

    view->destroy.notify = &handle_destroy;
    wl_signal_add(&surface->events.destroy, &view->destroy);

}

static void wm_view_xwayland_destroy(struct wm_view* super){
    struct wm_view_xwayland* view = wm_cast(wm_view_xwayland, super);

    wl_list_remove(&view->request_configure.link);
    wl_list_remove(&view->set_pid.link);
    wl_list_remove(&view->set_parent.link);
    wl_list_remove(&view->map.link);
    wl_list_remove(&view->unmap.link);
    wl_list_remove(&view->destroy.link);
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

static struct wlr_surface* wm_view_xwayland_surface_at(struct wm_view* super, double at_x, double at_y, double* sx, double* sy){
    struct wm_view_xwayland* view = wm_cast(wm_view_xwayland, super);

    struct wm_view_xwayland_child* child;
    wl_list_for_each(child, &view->children, link){
        if(!child->wlr_xwayland_surface->surface) continue;

        int child_x = child->wlr_xwayland_surface->x;
        int child_y = child->wlr_xwayland_surface->y;
        struct wlr_surface* surface = wlr_surface_surface_at(child->wlr_xwayland_surface->surface, 
                at_x - child_x, at_y - child_y, sx, sy);

        if(surface){
            return surface;
        }
    }

    struct wlr_surface* surface = wlr_surface_surface_at(view->wlr_xwayland_surface->surface, at_x, at_y, sx, sy);
    if(surface){
        return surface;
    }

    return NULL;
}


struct child_iterator_data {
    void* user_data;
    struct wm_view_xwayland_child* child;
    wlr_surface_iterator_func_t iterator;
};

static void child_iterator(struct wlr_surface* surface, int sx, int sy, void* _data){
    struct child_iterator_data* data = _data;

    int child_x = data->child->wlr_xwayland_surface->x;
    int child_y = data->child->wlr_xwayland_surface->y;

    data->iterator(surface, child_x + sx, child_y + sy, data->user_data); 
}

static void wm_view_xwayland_for_each_surface(struct wm_view* super, wlr_surface_iterator_func_t iterator, void* user_data){
    struct wm_view_xwayland* view = wm_cast(wm_view_xwayland, super);
    if(!view->wlr_xwayland_surface->surface){
        return;
    }
    wlr_surface_for_each_surface(view->wlr_xwayland_surface->surface, iterator, user_data);

    struct wm_view_xwayland_child* child;
    wl_list_for_each_reverse(child, &view->children, link){
        if(!child->mapped) continue;

        struct child_iterator_data data = {
            .user_data = user_data,
            .child = child,
            .iterator = iterator
        };

        wlr_surface_for_each_surface(child->wlr_xwayland_surface->surface, child_iterator, &data);
    }
}

static bool wm_view_xwayland_is_floating(struct wm_view* super){
    struct wm_view_xwayland* view = wm_cast(wm_view_xwayland, super);
    return view->floating;
}

static struct wm_view* wm_view_xwayland_get_parent(struct wm_view* super){
    struct wm_view_xwayland* view = wm_cast(wm_view_xwayland, super);
    return view->parent ? &view->parent->super : NULL;
}

struct wm_view_vtable wm_view_xwayland_vtable = {
    .destroy = wm_view_xwayland_destroy,
    .get_info = wm_view_xwayland_get_info,
    .request_size = wm_view_xwayland_request_size,
    .get_size = wm_view_xwayland_get_size,
    .get_size_constraints = wm_view_xwayland_get_size_constraints,
    .focus = wm_view_xwayland_focus,
    .surface_at = wm_view_xwayland_surface_at,
    .for_each_surface = wm_view_xwayland_for_each_surface,
    .is_floating = wm_view_xwayland_is_floating,
    .get_parent = wm_view_xwayland_get_parent
};

int wm_view_is_xwayland(struct wm_view* view){
    return view->vtable == &wm_view_xwayland_vtable;
}
