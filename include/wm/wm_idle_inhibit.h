#ifndef WM_IDLE_INHIBIT_H
#define WM_IDLE_INHIBIT_H

#include <wlr/types/wlr_surface.h>
#include <wlr/types/wlr_idle_inhibit_v1.h>

struct wm_server;
struct wm_idle_inhibit;

struct wm_idle_inhibitor {
    struct wlr_idle_inhibitor_v1* wlr_inhibitor;
    struct wm_idle_inhibit* parent;
    struct wm_view* view;

    struct wl_listener destroy;
};

void wm_idle_inhibitor_init(struct wm_idle_inhibitor* inhibitor, struct wm_idle_inhibit* parent, struct wlr_idle_inhibitor_v1* wlr_inhibitor);
void wm_idle_inhibitor_destroy(struct wm_idle_inhibitor* inhibitor);

struct wm_idle_inhibit {
    struct wm_server* wm_server;

    struct wlr_idle_inhibit_manager_v1* wlr_idle_inhibit_manager;
    struct wl_listener new_idle_inhibitor;
};

void wm_idle_inhibit_init(struct wm_idle_inhibit* inhibit, struct wm_server* server);
void wm_idle_inhibit_destroy(struct wm_idle_inhibit* inhibit);

#endif
