#ifndef WM_DRAG_H
#define WM_DRAG_H

#include <wlr/types/wlr_data_device.h>

#include "wm_content.h"

struct wm_drag {
    struct wm_content super;

    struct wm_seat* wm_seat;
    struct wlr_drag* wlr_drag;

    struct wl_listener destroy;

    struct wlr_drag_icon* wlr_drag_icon;
    struct wl_listener icon_surface_commit;
    struct wl_listener icon_map;
    struct wl_listener icon_unmap;
    struct wl_listener icon_destroy;
};

void wm_drag_init(struct wm_drag* drag, struct wm_seat* seat, struct wlr_drag* wlr_drag);
void wm_drag_update_position(struct wm_drag* drag);
bool wm_content_is_drag(struct wm_content* content);


#endif // WM_DRAG_H
