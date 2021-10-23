#ifndef WM_CURSOR_H
#define WM_CURSOR_H

#include <wayland-server.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_xcursor_manager.h>

struct wm_cursor;
struct wm_seat;
struct wm_layout;
struct wm_output;
struct wm_pointer;

struct wm_cursor {
    struct wm_seat* wm_seat;

    struct wlr_cursor* wlr_cursor;
    struct wlr_xcursor_manager* wlr_xcursor_manager;

    struct wl_listener motion;
    struct wl_listener motion_absolute;
    struct wl_listener button;
    struct wl_listener axis;
    struct wl_listener frame;
    struct wl_listener surface_destroy;


    uint32_t msec_delta;

    /* Set from python - final say about whether a cursor is displayed */
    int cursor_visible;

    struct {
        struct wlr_surface* surface;
        int32_t hotspot_x;
        int32_t hotspot_y;
    } client_image;
};

void wm_cursor_init(struct wm_cursor* cursor, struct wm_seat* seat, struct wm_layout* layout);
void wm_cursor_ensure_loaded_for_scale(struct wm_cursor* cursor, double scale);

void wm_cursor_set_visible(struct wm_cursor* cursor, int visible);
void wm_cursor_set_image(struct wm_cursor* cursor, const char* image);
void wm_cursor_set_image_surface(struct wm_cursor* cursor, struct wlr_surface* surface, int32_t hotspot_x, int32_t hotspot_y);

void wm_cursor_destroy(struct wm_cursor* cursor);
void wm_cursor_add_pointer(struct wm_cursor* cursor, struct wm_pointer* pointer);
void wm_cursor_update(struct wm_cursor* cursor);


#endif
