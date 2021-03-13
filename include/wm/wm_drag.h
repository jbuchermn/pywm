#ifndef WM_DRAG_H
#define WM_DRAG_H

#include <wlr/types/wlr_data_device.h>

struct wm_drag {
    struct wm_seat* wm_seat;
    struct wlr_drag* wlr_drag;

    struct wl_listener destroy;

    struct {
        struct wlr_drag_icon* wlr_drag_icon;
        double x;
        double y;

    } icon;

    struct wl_listener icon_surface_commit;
    struct wl_listener icon_map;
    struct wl_listener icon_unmap;
    struct wl_listener icon_destroy;
};

void wm_drag_init(struct wm_drag* drag, struct wm_seat* seat, struct wlr_drag* wlr_drag);
void wm_drag_destroy(struct wm_drag* drag);


#endif // WM_DRAG_H
