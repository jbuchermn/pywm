#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <stdlib.h>
#include <wayland-server.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <wlr/xwayland.h>

#include "wm/wm_view.h"
#include "wm/wm_seat.h"
#include "wm/wm_server.h"
#include "wm/wm.h"

/*
 * Callbacks: xdg_surface
 */
static void handle_xdg_map(struct wl_listener* listener, void* data){
    struct wm_view* view = wl_container_of(listener, view, map);
    view->mapped = true;
}

static void handle_xdg_unmap(struct wl_listener* listener, void* data){
    struct wm_view* view = wl_container_of(listener, view, unmap);
    view->mapped = false;
    wm_callback_destroy_view(view);
}

static void handle_xdg_destroy(struct wl_listener* listener, void* data){
    struct wm_view* view = wl_container_of(listener, view, destroy);
    wm_view_destroy(view);
    free(view);
}


/*
 * Callbacks: xwayland_surface
 */
#ifdef PYWM_XWAYLAND
static void handle_xwayland_map(struct wl_listener* listener, void* data){
    struct wm_view* view = wl_container_of(listener, view, map);

    const char* title;
    const char* app_id; 
    const char* role;
    wm_view_get_info(view, &title, &app_id, &role);
    wlr_log(WLR_DEBUG, "New wm_view (xwayland): %s, %s, %s", title, app_id, role);

    pid_t pid = view->wlr_xwayland_surface->pid;
    struct wm_view* parent = wm_server_root_view_for_pid(view->wm_server, pid);
    if(parent){
        const char* title;
        const char* app_id; 
        const char* role;
        wm_view_get_info(parent, &title, &app_id, &role);
        wlr_log(WLR_DEBUG, "Is child of view %s, %s, %s", title, app_id, role);

        view->parent = parent;
    }else{
        wm_callback_init_view(view);
    }

    view->pid = pid;


    view->mapped = true;

}

static void handle_xwayland_unmap(struct wl_listener* listener, void* data){
    struct wm_view* view = wl_container_of(listener, view, unmap);
    view->mapped = false;
    wm_callback_destroy_view(view);
}

static void handle_xwayland_destroy(struct wl_listener* listener, void* data){
    struct wm_view* view = wl_container_of(listener, view, destroy);

    struct wm_view* child;
    wl_list_for_each(child, &view->wm_server->wm_views, link){
        if(child->parent == view){
            child->parent = NULL;
        }
    }

    wm_view_destroy(view);
    free(view);
}
#endif


/*
 * Class implementation
 */
void wm_view_init_xdg(struct wm_view* view, struct wm_server* server, struct wlr_xdg_surface* surface){
    view->kind = WM_VIEW_XDG;

    view->wm_server = server;
    view->wlr_xdg_surface = surface;

    const char* title;
    const char* app_id; 
    const char* role;
    wm_view_get_info(view, &title, &app_id, &role);
    wlr_log(WLR_DEBUG, "New wm_view (xdg): %s, %s, %s", title, app_id, role);

    view->mapped = false;
    view->pid = 0;
    view->parent = NULL;

    view->map.notify = &handle_xdg_map;
    wl_signal_add(&surface->events.map, &view->map);

    view->unmap.notify = &handle_xdg_unmap;
    wl_signal_add(&surface->events.unmap, &view->unmap);

    view->destroy.notify = &handle_xdg_destroy;
    wl_signal_add(&surface->events.destroy, &view->destroy);

    /*
     * XDG Views are initialized immediately to set the width/height before mapping
     */
    wm_callback_init_view(view);

    /* Get rid of white spaces around; therefore geometry.width/height should always equal current.width/height */
    wlr_xdg_toplevel_set_tiled(surface, 15);
}

#ifdef PYWM_XWAYLAND
void wm_view_init_xwayland(struct wm_view* view, struct wm_server* server, struct wlr_xwayland_surface* surface){
    view->kind = WM_VIEW_XWAYLAND;

    view->wm_server = server;
    view->wlr_xwayland_surface = surface;

    view->mapped = false;
    view->pid = 0;
    view->parent = NULL;

    /* If parent views are destroyed before children,
     * the children become orphans and use their own display_...
     * for positioning, therefore we initialise them to zero and do
     * not display the orphans
     */
    view->display_x = 0;
    view->display_y = 0;
    view->display_width = 0;
    view->display_height = 0;

    view->map.notify = &handle_xwayland_map;
    wl_signal_add(&surface->events.map, &view->map);

    view->unmap.notify = &handle_xwayland_unmap;
    wl_signal_add(&surface->events.unmap, &view->unmap);

    view->destroy.notify = &handle_xwayland_destroy;
    wl_signal_add(&surface->events.destroy, &view->destroy);

    /*
     * XWayland views are not initialised immediately, as there are a number of useless
     * surfaces, that never get mapped...
     */

}
#endif

void wm_view_destroy(struct wm_view* view){
    wl_list_remove(&view->map.link);
    wl_list_remove(&view->unmap.link);
    wl_list_remove(&view->destroy.link);
    wl_list_remove(&view->link);
}

void wm_view_set_box(struct wm_view* view, double x, double y, double width, double height){
    view->display_x = x;
    view->display_y = y;
    view->display_width = width;
    view->display_height = height;
}

void wm_view_get_info(struct wm_view* view, const char** title, const char** app_id, const char** role){
    switch(view->kind){
    case WM_VIEW_XDG:
        *title = view->wlr_xdg_surface->toplevel->title;
        *app_id = view->wlr_xdg_surface->toplevel->app_id;
        *role = "toplevel";
        break;
#ifdef PYWM_XWAYLAND
    case WM_VIEW_XWAYLAND:
        *title = view->wlr_xwayland_surface->title;
        *app_id = view->wlr_xwayland_surface->class;
        *role = view->wlr_xwayland_surface->instance;
        break;
#endif
    }

}

void wm_view_get_display_box(struct wm_view* view, double* display_x, double* display_y, double* display_width, double* display_height){
    if(view->parent){
        wm_view_get_display_box(view->parent, display_x, display_y, display_width, display_height);

        int pwidth, pheight;
        wm_view_get_size(view->parent, &pwidth, &pheight);

        double x_scale = pwidth / *display_width;
        double y_scale = pheight / *display_height;

        int width, height;
        wm_view_get_size(view, &width, &height);

        if(view->kind == WM_VIEW_XWAYLAND && view->wlr_xwayland_surface){
            *display_x += view->wlr_xwayland_surface->x / x_scale;
            *display_y += view->wlr_xwayland_surface->y / x_scale;
        }

        *display_width = width / x_scale;
        *display_height = height / y_scale;

    }else{
        *display_x = view->display_x;
        *display_y = view->display_y;
        *display_width = view->display_width;
        *display_height = view->display_height;
    }
}

void wm_view_request_size(struct wm_view* view, int width, int height){
    switch(view->kind){
    case WM_VIEW_XDG:
        if(!view->wlr_xdg_surface){
            wlr_log(WLR_DEBUG, "Warning: view with wlr_xdg_surface == 0");
            return;
        }

        if(view->wlr_xdg_surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL){
            wlr_xdg_toplevel_set_size(view->wlr_xdg_surface, width, height);
        }else{
            wlr_log(WLR_DEBUG, "Warning: Can only set size on toplevel");
        }
        break;
#ifdef PYWM_XWAYLAND
    case WM_VIEW_XWAYLAND:
        wlr_xwayland_surface_configure(view->wlr_xwayland_surface, 0, 0, width, height);
        break;
#endif
    }
}

void wm_view_get_size(struct wm_view* view, int* width, int* height){
    switch(view->kind){
    case WM_VIEW_XDG:
        /* Fixed by set_tiled */
        /* Although during updates not strictly equal? */
        /* assert(view->wlr_xdg_surface->geometry.width == view->wlr_xdg_surface->surface->current.width); */
        /* assert(view->wlr_xdg_surface->geometry.height == view->wlr_xdg_surface->surface->current.height); */

        if(!view->wlr_xdg_surface){
            *width = 0;
            *height = 0;

            wlr_log(WLR_DEBUG, "Warning: view with wlr_xdg_surface == 0");
            return;
        }

        *width = view->wlr_xdg_surface->geometry.width;
        *height = view->wlr_xdg_surface->geometry.height;
        break;
#ifdef PYWM_XWAYLAND
    case WM_VIEW_XWAYLAND:
        if(!view->wlr_xwayland_surface->surface){
            *width = 0;
            *height = 0;
            return;
        }

        *width = view->wlr_xwayland_surface->surface->current.width;
        *height = view->wlr_xwayland_surface->surface->current.height;
        break;
#endif
    }
}


void wm_view_focus(struct wm_view* view, struct wm_seat* seat){
    switch(view->kind){
    case WM_VIEW_XDG:
        wm_seat_focus_surface(seat, view->wlr_xdg_surface->surface);
        break;
#ifdef PYWM_XWAYLAND
    case WM_VIEW_XWAYLAND:
        if(!view->wlr_xwayland_surface->surface){
            return;
        }
        wm_seat_focus_surface(seat, view->wlr_xwayland_surface->surface);
        break;
#endif
    }
}

void wm_view_set_activated(struct wm_view* view, bool activated){
    switch(view->kind){
    case WM_VIEW_XDG:
        if(!view->wlr_xdg_surface){
            return;
        }
        wlr_xdg_toplevel_set_activated(view->wlr_xdg_surface, activated);
        break;
#ifdef PYWM_XWAYLAND
    case WM_VIEW_XWAYLAND:
        if(!view->wlr_xwayland_surface->surface){
            return;
        }
        wlr_xwayland_surface_activate(view->wlr_xwayland_surface, activated);
        break;
#endif
    }

}

struct wlr_surface* wm_view_surface_at(struct wm_view* view, double at_x, double at_y, double* sx, double* sy){
    switch(view->kind){
    case WM_VIEW_XDG:
        return wlr_xdg_surface_surface_at(view->wlr_xdg_surface, at_x, at_y, sx, sy);
#ifdef PYWM_XWAYLAND
    case WM_VIEW_XWAYLAND:
        if(!view->wlr_xwayland_surface->surface){
            return NULL;
        }

        return wlr_surface_surface_at(view->wlr_xwayland_surface->surface, at_x, at_y, sx, sy);
#endif
    }

    /* prevent warning */
    return NULL;
}

void wm_view_for_each_surface(struct wm_view* view, wlr_surface_iterator_func_t iterator, void* user_data){
    switch(view->kind){
    case WM_VIEW_XDG:
        wlr_xdg_surface_for_each_surface(view->wlr_xdg_surface, iterator, user_data);
        break;
#ifdef PYWM_XWAYLAND
    case WM_VIEW_XWAYLAND:
        if(!view->wlr_xwayland_surface->surface){
            return;
        }
        wlr_surface_for_each_surface(view->wlr_xwayland_surface->surface, iterator, user_data);
        break;
#endif
    }
}
