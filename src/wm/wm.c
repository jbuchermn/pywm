#define _POSIX_C_SOURCE 200112L

#include "wm/wm.h"

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <wayland-server.h>
#include <wlr/backend.h>
#include <wlr/util/log.h>
#include <wlr/xwayland.h>

#include "wm/wm_cursor.h"
#include "wm/wm_layout.h"
#include "wm/wm_seat.h"
#include "wm/wm_server.h"
#include "wm/wm_view.h"
#include "wm/wm_widget.h"

struct wm wm = {0};

void wm_init(struct wm_config *config) {
    if (wm.server)
        return;

    wlr_log_init(WLR_DEBUG, NULL);
    wm.server = calloc(1, sizeof(struct wm_server));
    wm_server_init(wm.server, config);
}

void wm_destroy() {
    if (!wm.server)
        return;

    wm_server_destroy(wm.server);
    free(wm.server);
    wm.server = 0;
}

void *run() {
    if (!wm.server || !wm.server->wl_display)
        return NULL;

    /* Setup socket and set env */
    const char *socket = wl_display_add_socket_auto(wm.server->wl_display);
    if (!socket) {
        wlr_log_errno(WLR_ERROR, "Unable to open wayland socket");
        wlr_backend_destroy(wm.server->wlr_backend);
        return NULL;
    }

    wlr_log(WLR_INFO, "Running compositor on wayland display '%s'", socket);
    setenv("_WAYLAND_DISPLAY", socket, true);

    wlr_log(WLR_INFO, "Starting backend...");
    if (!wlr_backend_start(wm.server->wlr_backend)) {
        wlr_log(WLR_ERROR, "Failed to start backend");
        wlr_backend_destroy(wm.server->wlr_backend);
        wl_display_destroy(wm.server->wl_display);
        return NULL;
    }

    setenv("WAYLAND_DISPLAY", socket, true);
    if (wm.server->wlr_xwayland) {
        setenv("DISPLAY", wm.server->wlr_xwayland->display_name, true);
    }else{
        wm_callback_ready();
    }

    /* Main */
    wlr_log(WLR_INFO, "Main...");
    wl_display_run(wm.server->wl_display);

    unsetenv("_WAYLAND_DISPLAY");
    unsetenv("WAYLAND_DISPLAY");
    unsetenv("DISPLAY");

    return NULL;
}

int wm_run() {
    if (!wm.server)
        return 2;

    run();

    return 0;
}

void wm_terminate() {
    if (!wm.server)
        return;

    wl_display_terminate(wm.server->wl_display);
}

void wm_focus_view(struct wm_view *view) {
    if (!wm.server)
        return;

    wm_view_focus(view, wm.server->wm_seat);
}

void wm_update_cursor(int cursor_visible) {
    if (!wm.server)
        return;

    wm_cursor_set_visible(wm.server->wm_seat->wm_cursor, cursor_visible);
    wm_cursor_update(wm.server->wm_seat->wm_cursor);
}

void wm_set_locked(double locked) {
    if (!wm.server)
        return;

    wm_server_set_locked(wm.server, locked);
}

void wm_open_virtual_output(const char* name){
   wm_server_open_virtual_output(wm.server, name);
}

void wm_close_virtual_output(const char* name){
   wm_server_close_virtual_output(wm.server, name);
}

struct wm_widget *wm_create_widget() {
    if (!wm.server)
        return NULL;

    return wm_server_create_widget(wm.server);
}

void wm_destroy_widget(struct wm_widget *widget) {
    wm_content_destroy(&widget->super);
    free(widget);
}

struct wm *get_wm() {
    return &wm;
}

/*
 * Callbacks
 */
void wm_callback_layout_change(struct wm_layout *layout) {
    if (!wm.callback_layout_change) {
        return;
    }

    return (*wm.callback_layout_change)(layout);
}

bool wm_callback_key(struct wlr_event_keyboard_key *event,
                     const char *keysyms) {
    if (!wm.callback_key) {
        return false;
    }

    return (*wm.callback_key)(event, keysyms);
}

bool wm_callback_modifiers(struct wlr_keyboard_modifiers *modifiers) {
    if (!wm.callback_modifiers) {
        return false;
    }

    return (*wm.callback_modifiers)(modifiers);
}

bool wm_callback_motion(double delta_x, double delta_y, double abs_x, double abs_y, uint32_t time_msec) {
    if (!wm.callback_motion) {
        return false;
    }

    return (*wm.callback_motion)(delta_x, delta_y, abs_x, abs_y, time_msec);
}

bool wm_callback_button(struct wlr_event_pointer_button *event) {
    if (!wm.callback_button) {
        return false;
    }

    return (*wm.callback_button)(event);
}

bool wm_callback_axis(struct wlr_event_pointer_axis *event) {
    if (!wm.callback_axis) {
        return false;
    }

    return (*wm.callback_axis)(event);
}

void wm_callback_init_view(struct wm_view *view) {
    if (!wm.callback_init_view) {
        return;
    }

    return (*wm.callback_init_view)(view);
}

void wm_callback_destroy_view(struct wm_view *view) {
    if (!wm.callback_destroy_view) {
        return;
    }

    return (*wm.callback_destroy_view)(view);
}

void wm_callback_view_event(struct wm_view *view, const char *event) {
    if (!wm.callback_view_event) {
        return;
    }

    return (*wm.callback_view_event)(view, event);
}

void wm_callback_view_resize(struct wm_view *view, int width, int height) {
    if (!wm.callback_view_resize) {
        return;
    }

    return (*wm.callback_view_resize)(view, width, height);
}

void wm_callback_update() {
    if (!wm.callback_update) {
        return;
    }

    return (*wm.callback_update)();
}

void wm_callback_ready() {
    if (!wm.callback_ready) {
        return;
    }

    return (*wm.callback_ready)();
}
