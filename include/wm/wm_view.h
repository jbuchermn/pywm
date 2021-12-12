#ifndef WM_VIEW_H
#define WM_VIEW_H

#include <stdbool.h>
#include <wayland-server.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/util/box.h>

#include "wm_content.h"

struct wm_seat;
struct wm_view_vtable;

typedef void (*wm_surface_iterator_func_t)(struct wlr_surface *surface,
	int sx, int sy, bool constrained, void *data);

struct wm_view {
    struct wm_content super;

    struct wm_view_vtable* vtable;
    bool mapped;
    bool inhibiting_idle;

    bool accepts_input;

    /* Server-side determined states - stored from setter */
    bool floating;
    bool focused;
    bool fullscreen;
    bool maximized;
    bool resizing;
};

void wm_view_base_init(struct wm_view* view, struct wm_server* server);

void wm_view_set_inhibiting_idle(struct wm_view* view, bool inhibiting_idle);
bool wm_view_is_inhibiting_idle(struct wm_view* view);

bool wm_content_is_view(struct wm_content* content);

struct wm_view_vtable {
    void (*destroy)(struct wm_view* view);

    struct wlr_surface* (*surface_at)(struct wm_view* view, double at_x, double at_y, double* sx, double* sy);
    void (*for_each_surface)(struct wm_view* view, wm_surface_iterator_func_t iterator, void* user_data);
    void (*set_activated)(struct wm_view* view, bool activated);

    void (*get_credentials)(struct wm_view* view, pid_t* pid, uid_t* uid, gid_t* gid);
    void (*get_info)(struct wm_view* view, const char** title, const char** app_id, const char** role);
    void (*get_size)(struct wm_view* view, int* width, int* height);
    void (*get_offset)(struct wm_view* view, int* offset_x, int* offset_y);
    void (*get_size_constraints)(struct wm_view* view, int** constraints, int* n_constraints);
    struct wm_view* (*get_parent)(struct wm_view* view);

    void (*request_size)(struct wm_view* view, int width, int height);
    void (*set_floating)(struct wm_view* view, bool floating);
    void (*set_resizing)(struct wm_view* view, bool resizing);
    void (*set_fullscreen)(struct wm_view* view, bool fullscreen);
    void (*set_maximized)(struct wm_view* view, bool maximized);
    void (*focus)(struct wm_view* view, struct wm_seat* seat);
    void (*request_close)(struct wm_view* view);

    void (*structure_printf)(FILE* file, struct wm_view* view);
};

static inline void wm_view_get_credentials(struct wm_view* view, pid_t* pid, uid_t* uid, gid_t* gid){
    (*view->vtable->get_credentials)(view, pid, uid, gid);
}

static inline void wm_view_get_info(struct wm_view* view, const char** title, const char** app_id, const char** role){
    (*view->vtable->get_info)(view, title, app_id, role);
}

static inline void wm_view_request_size(struct wm_view* view, int width, int height){
    (*view->vtable->request_size)(view, width, height);
}

static inline void wm_view_get_size_constraints(struct wm_view* view, int** constraints, int* n_constraints){
    (*view->vtable->get_size_constraints)(view, constraints, n_constraints);
}

static inline void wm_view_get_size(struct wm_view* view, int* width, int* height){
    (*view->vtable->get_size)(view, width, height);
}

static inline void wm_view_get_offset(struct wm_view* view, int* offset_x, int* offset_y){
    (*view->vtable->get_offset)(view, offset_x, offset_y);
}

static inline void wm_view_focus(struct wm_view* view, struct wm_seat* seat){
    (*view->vtable->focus)(view, seat);
}

static inline void wm_view_set_floating(struct wm_view* view, bool floating){
    view->floating = floating;
    (*view->vtable->set_floating)(view, floating);
}

static inline void wm_view_set_resizing(struct wm_view* view, bool resizing){
    view->resizing = resizing;
    (*view->vtable->set_resizing)(view, resizing);
}

static inline void wm_view_set_fullscreen(struct wm_view* view, bool fullscreen){
    view->fullscreen = fullscreen;
    (*view->vtable->set_fullscreen)(view, fullscreen);
}

static inline void wm_view_set_maximized(struct wm_view* view, bool maximized){
    view->maximized = maximized;
    (*view->vtable->set_maximized)(view, maximized);
}

static inline void wm_view_set_activated(struct wm_view* view, bool activated){
    view->focused = activated;
    (*view->vtable->set_activated)(view, activated);
}

static inline bool wm_view_is_floating(struct wm_view* view){
    return view->floating;
}

static inline bool wm_view_is_focused(struct wm_view* view){
    return view->focused;
}

static inline bool wm_view_is_fullscreen(struct wm_view* view){
    return view->fullscreen;
}

static inline bool wm_view_is_maximized(struct wm_view* view){
    return view->maximized;
}

static inline bool wm_view_is_resizing(struct wm_view* view){
    return view->resizing;
}

static inline void wm_view_request_close(struct wm_view* view){
    (*view->vtable->request_close)(view);
}

static inline struct wlr_surface* wm_view_surface_at(struct wm_view* view, double at_x, double at_y, double* sx, double* sy){
    return (*view->vtable->surface_at)(view, at_x, at_y, sx, sy);
}

static inline void wm_view_for_each_surface(struct wm_view* view, wm_surface_iterator_func_t iterator, void* user_data){
    (*view->vtable->for_each_surface)(view, iterator, user_data);
}


static inline struct wm_view* wm_view_get_parent(struct wm_view* view){
    return (*view->vtable->get_parent)(view);
}

static inline void wm_view_structure_printf(FILE* file, struct wm_view* view){
    (*view->vtable->structure_printf)(file, view);
}



#endif
