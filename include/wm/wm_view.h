#ifndef WM_VIEW_H
#define WM_VIEW_H

#include <stdbool.h>
#include <wayland-server.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/types/wlr_box.h>

struct wm_seat;
struct wm_view_vtable;

struct wm_view {
    struct wl_list link;  // wm_server::wm_views
    struct wm_server* wm_server;

    struct wm_view_vtable* vtable;

    bool mapped;

    double display_x;
    double display_y;
    double display_width;
    double display_height;
};

void wm_view_base_init(struct wm_view* view, struct wm_server* server);
void wm_view_base_destroy(struct wm_view* view);
void wm_view_base_get_box(struct wm_view* view, double* x, double* y, double* width, double* height);
void wm_view_base_set_box(struct wm_view* view, double x, double y, double width, double height);

struct wm_view_vtable {
    void (*destroy)(struct wm_view* view);
    void (*get_info)(struct wm_view* view, const char** title, const char** app_id, const char** role);
    void (*set_box)(struct wm_view* view, double x, double y, double width, double height);
    void (*get_box)(struct wm_view* view, double* x, double* y, double* width, double* height);
    void (*request_size)(struct wm_view* view, int width, int height);
    void (*get_size)(struct wm_view* view, int* width, int* height);
    void (*get_size_constraints)(struct wm_view* view, int* min_width, int* max_width, int* min_height, int* max_height);
    void (*focus)(struct wm_view* view, struct wm_seat* seat);
    void (*set_activated)(struct wm_view* view, bool activated);
    struct wlr_surface* (*surface_at)(struct wm_view* view, double at_x, double at_y, double* sx, double* sy);
    void (*for_each_surface)(struct wm_view* view, wlr_surface_iterator_func_t iterator, void* user_data);
};

static inline void wm_view_destroy(struct wm_view* view){
    (*view->vtable->destroy)(view);
}

static inline void wm_view_get_info(struct wm_view* view, const char** title, const char** app_id, const char** role){
    (*view->vtable->get_info)(view, title, app_id, role);
}

static inline void wm_view_set_box(struct wm_view* view, double x, double y, double width, double height){
    (*view->vtable->set_box)(view, x, y, width, height);
}

static inline void wm_view_get_box(struct wm_view* view, double* x, double* y, double* width, double* height){
    (*view->vtable->get_box)(view, x, y, width, height);
}

static inline void wm_view_request_size(struct wm_view* view, int width, int height){
    (*view->vtable->request_size)(view, width, height);
}

static inline void wm_view_get_size_constraints(struct wm_view* view, int* min_width, int* max_width, int* min_height, int* max_height){
    (*view->vtable->get_size_constraints)(view, min_width, max_width, min_height, max_height);
}

static inline void wm_view_get_size(struct wm_view* view, int* width, int* height){
    (*view->vtable->get_size)(view, width, height);
}

static inline void wm_view_focus(struct wm_view* view, struct wm_seat* seat){
    (*view->vtable->focus)(view, seat);
}

static inline void wm_view_set_activated(struct wm_view* view, bool activated){
    (*view->vtable->set_activated)(view, activated);
}

static inline struct wlr_surface* wm_view_surface_at(struct wm_view* view, double at_x, double at_y, double* sx, double* sy){
    return (*view->vtable->surface_at)(view, at_x, at_y, sx, sy);
}

static inline void wm_view_for_each_surface(struct wm_view* view, wlr_surface_iterator_func_t iterator, void* user_data){
    (*view->vtable->for_each_surface)(view, iterator, user_data);
}

struct wm_view_vtable wm_view_base_vtable;


#endif
