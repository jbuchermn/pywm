#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <stdlib.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_primary_selection.h>
#include <wlr/util/log.h>
#include "wm/wm.h"
#include "wm/wm_seat.h"
#include "wm/wm_view.h"
#include "wm/wm_server.h"
#include "wm/wm_keyboard.h"
#include "wm/wm_pointer.h"
#include "wm/wm_cursor.h"
#include "wm/wm_config.h"
#include "wm/wm_drag.h"

/*
 * Callbacks
 */
static void handle_request_start_drag(struct wl_listener* listener, void* data){
    wlr_log(WLR_DEBUG, "Handling request start drag");
    struct wm_seat* seat = wl_container_of(listener, seat, request_start_drag);
    struct wlr_seat_request_start_drag_event* event = data;

    if(wlr_seat_validate_pointer_grab_serial(seat->wlr_seat, event->origin, event->serial)){
        wlr_seat_start_pointer_drag(seat->wlr_seat, event->drag, event->serial);
    }else{
        wlr_log(WLR_DEBUG, "Failed to validate pointer grab");
        wlr_data_source_destroy(event->drag->source);
    }
}

static void handle_start_drag(struct wl_listener* listener, void* data){
    wlr_log(WLR_DEBUG, "Starting drag");
    struct wm_seat* seat = wl_container_of(listener, seat, start_drag);
    struct wlr_drag* wlr_drag = data;

    struct wm_drag* drag = calloc(1, sizeof(struct wm_drag));
    wm_drag_init(drag, seat, wlr_drag);

}

static void handle_request_set_selection(struct wl_listener* listener, void* data){
    struct wm_seat* seat = wl_container_of(listener, seat, request_set_selection);
    struct wlr_seat_request_set_selection_event* event = data;

    wlr_seat_set_selection(seat->wlr_seat, event->source, event->serial);
}

static void handle_request_set_primary_selection(struct wl_listener* listener, void* data){
    struct wm_seat* seat = wl_container_of(listener, seat, request_set_primary_selection);
    struct wlr_seat_request_set_primary_selection_event* event = data;

    wlr_seat_set_primary_selection(seat->wlr_seat, event->source, event->serial);
}

static void handle_request_set_cursor(struct wl_listener* listener, void* data){
    struct wm_seat* seat = wl_container_of(listener, seat, request_set_cursor);
    struct wlr_seat_pointer_request_set_cursor_event* event = data;

    /* Following sway */
	struct wl_client *focused_client = NULL;
	struct wlr_surface *focused_surface = seat->wlr_seat->pointer_state.focused_surface;
	if (focused_surface != NULL) {
		focused_client = wl_resource_get_client(focused_surface->resource);
	}

	if (focused_client == NULL ||
			event->seat_client->client != focused_client) {
		wlr_log(WLR_DEBUG, "Denying request to set cursor from unfocused client");
		return;
	}

    wm_cursor_set_image_surface(seat->wm_cursor, event->surface, event->hotspot_x, event->hotspot_y);

}

static void handle_destroy(struct wl_listener* listener, void* data){
	wlr_log(WLR_DEBUG, "Destroying seat...");
    struct wm_seat* seat = wl_container_of(listener, seat, destroy);
    wm_seat_destroy(seat);
}

/*
 * Class implementation
 */
void wm_seat_init(struct wm_seat* seat, struct wm_server* server, struct wm_layout* layout){
    seat->wm_server = server;
    wl_list_init(&seat->wm_keyboards);
    wl_list_init(&seat->wm_pointers);

    seat->wlr_seat = wlr_seat_create(server->wl_display, "default");
    assert(seat->wlr_seat);

    seat->wm_cursor = calloc(1, sizeof(struct wm_cursor));
    wm_cursor_init(seat->wm_cursor, seat, layout);

    seat->request_start_drag.notify = handle_request_start_drag;
    wl_signal_add(&seat->wlr_seat->events.request_start_drag, &seat->request_start_drag);

    seat->start_drag.notify = handle_start_drag;
    wl_signal_add(&seat->wlr_seat->events.start_drag, &seat->start_drag);

    seat->request_set_selection.notify = handle_request_set_selection;
    wl_signal_add(&seat->wlr_seat->events.request_set_selection, &seat->request_set_selection);

    seat->request_set_primary_selection.notify = handle_request_set_primary_selection;
    wl_signal_add(&seat->wlr_seat->events.request_set_primary_selection, &seat->request_set_primary_selection);

    seat->request_set_cursor.notify = handle_request_set_cursor;
    wl_signal_add(&seat->wlr_seat->events.request_set_cursor, &seat->request_set_cursor);

    seat->destroy.notify = handle_destroy;
    wl_signal_add(&seat->wlr_seat->events.destroy, &seat->destroy);
}

void wm_seat_destroy(struct wm_seat* seat) {
    wl_list_remove(&seat->request_start_drag.link);
    wl_list_remove(&seat->start_drag.link);
    wl_list_remove(&seat->request_set_selection.link);
    wl_list_remove(&seat->request_set_primary_selection.link);
    wl_list_remove(&seat->destroy.link);

    wm_cursor_destroy(seat->wm_cursor);
    free(seat->wm_cursor);

    wlr_seat_destroy(seat->wlr_seat);

    wlr_log(WLR_ERROR, "Seat destroy is not supported");
}

void wm_seat_add_input_device(struct wm_seat* seat, struct wlr_input_device* input_device){
    switch(input_device->type){
    case WLR_INPUT_DEVICE_KEYBOARD:
        wlr_log(WLR_DEBUG, "New keyboard");

        struct wm_keyboard* keyboard = calloc(1, sizeof(struct wm_keyboard));
        wm_keyboard_init(keyboard, seat, input_device);
        wl_list_insert(&seat->wm_keyboards, &keyboard->link);

        wlr_seat_set_keyboard(seat->wlr_seat, keyboard->wlr_input_device);
        break;

    case WLR_INPUT_DEVICE_POINTER:
        wlr_log(WLR_DEBUG, "New pointer");

        struct wm_pointer* pointer = calloc(1, sizeof(struct wm_pointer));
        wm_pointer_init(pointer, seat, input_device);
        wl_list_insert(&seat->wm_pointers, &pointer->link);

        wm_cursor_add_pointer(seat->wm_cursor, pointer);
        break;
    case WLR_INPUT_DEVICE_TOUCH:
    case WLR_INPUT_DEVICE_SWITCH:
    case WLR_INPUT_DEVICE_TABLET_TOOL:
    case WLR_INPUT_DEVICE_TABLET_PAD:
        wlr_log(WLR_DEBUG, "Unsupported input device");
    }

    uint32_t capabilities = 0;
    if(!wl_list_empty(&seat->wm_keyboards)){
        capabilities |= WL_SEAT_CAPABILITY_KEYBOARD;
    }
    if(!wl_list_empty(&seat->wm_pointers)){
        capabilities |= WL_SEAT_CAPABILITY_POINTER;
    }

    wlr_seat_set_capabilities(seat->wlr_seat, capabilities);
}

void wm_seat_clear_focus(struct wm_seat* seat){
    wm_seat_focus_surface(seat, NULL);
    wlr_seat_keyboard_clear_focus(seat->wlr_seat);
}

void wm_seat_focus_surface(struct wm_seat* seat, struct wlr_surface* surface){
    struct wlr_surface* prev = seat->wlr_seat->keyboard_state.focused_surface;
    if(prev == surface){
        return;
    }
    struct wm_view* prev_view = NULL;
    if(prev) prev_view = wm_server_view_for_surface(seat->wm_server, prev);

    struct wm_view* view = NULL;
    if(surface) view = wm_server_view_for_surface(seat->wm_server, surface);

    if(prev_view && view && prev_view == view){
        return;
    }

    if(prev_view) wm_view_set_activated(prev_view, false);

    /* Guard keyboard focus */
    if(wm_server_is_locked(seat->wm_server)){
        if(!view) return;
        if(!view->super.lock_enabled) return;
    }

    if(view) wm_view_set_activated(view, true);
    struct wlr_keyboard* keyboard = wlr_seat_get_keyboard(seat->wlr_seat);
    wlr_seat_keyboard_notify_enter(seat->wlr_seat, surface,
            keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);

}

void wm_seat_dispatch_key(struct wm_seat* seat, struct wlr_input_device* input_device, struct wlr_event_keyboard_key* event){
    wlr_seat_set_keyboard(seat->wlr_seat, input_device);
    wlr_seat_keyboard_notify_key(seat->wlr_seat, event->time_msec, event->keycode, event->state);
}

void wm_seat_dispatch_modifiers(struct wm_seat* seat, struct wlr_input_device* input_device){
    wlr_seat_set_keyboard(seat->wlr_seat, input_device);
    wlr_seat_keyboard_notify_modifiers(seat->wlr_seat, &input_device->keyboard->modifiers);
}


bool wm_seat_dispatch_motion(struct wm_seat* seat, double x, double y, uint32_t time_msec){
    struct wlr_surface* surface;
    double sx;
    double sy;

    wm_server_surface_at(seat->wm_server, x, y, &surface, &sx, &sy);
    if(!surface){
        wlr_seat_pointer_clear_focus(seat->wlr_seat);
        return false;
    }

    if(seat->wm_server->wm_config->focus_follows_mouse){
        /* Automatically focus surface on mouse enter  */
        wm_seat_focus_surface(seat, surface);
    }

    /* Guard mouse focus */
    if(wm_server_is_locked(seat->wm_server)){
        struct wm_view* view = wm_server_view_for_surface(seat->wm_server, surface);
        if(!view || !view->super.lock_enabled){
            goto Guard;
        }
    }
    wlr_seat_pointer_notify_enter(seat->wlr_seat, surface, sx, sy);

Guard:
    wlr_seat_pointer_notify_motion(seat->wlr_seat, time_msec, sx, sy);

    return true;
}

static void _handle_focus(struct wm_seat* seat, double x, double y){
    struct wlr_surface* surface;
    double sx;
    double sy;

    wm_server_surface_at(seat->wm_server, x, y, &surface, &sx, &sy);
    if(!surface){
        wlr_seat_pointer_clear_focus(seat->wlr_seat);
        return;
    }

    wm_seat_focus_surface(seat, surface);
}

void wm_seat_dispatch_button(struct wm_seat* seat, struct wlr_event_pointer_button* event){
    /* Focus clicked surface */
    _handle_focus(seat, seat->wm_cursor->wlr_cursor->x, seat->wm_cursor->wlr_cursor->y);

    wlr_seat_pointer_notify_button(seat->wlr_seat, event->time_msec, event->button, event->state);
}
void wm_seat_dispatch_axis(struct wm_seat* seat, struct wlr_event_pointer_axis* event){
    /* Focus acted on surface */
    _handle_focus(seat, seat->wm_cursor->wlr_cursor->x, seat->wm_cursor->wlr_cursor->y);

    wlr_seat_pointer_notify_axis(seat->wlr_seat,
            event->time_msec, event->orientation, event->delta, event->delta_discrete, event->source);
}
