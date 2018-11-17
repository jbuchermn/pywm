#define _POSIX_C_SOURCE 200112L

#include <wayland-server.h>
#include <wlr/util/log.h>
#include <wlr/types/wlr_input_device.h>
#include <libinput.h>
#include <assert.h>
#include "wm/wm_pointer.h"
#include "wm/wm_seat.h"

/*
 * Callbacks
 */
static void handle_destroy(struct wl_listener* listener, void* data){
    struct wm_pointer* pointer = wl_container_of(listener, pointer, destroy);
    wm_pointer_destroy(pointer);
}

/*
 * Class implementation
 */
void wm_pointer_init(struct wm_pointer* pointer, struct wm_seat* seat, struct wlr_input_device* input_device){
    pointer->wm_seat = seat;
    pointer->wlr_input_device = input_device;

    /* segfaults */
    /* if(wlr_input_device_is_libinput(pointer->wlr_input_device)){ */
    /*     struct libinput_device* device = wlr_libinput_get_device_handle(pointer->wlr_input_device); */
    /*     assert(device); */
    /*     libinput_device_config_scroll_set_natural_scroll_enabled(device, false); */
    /* } */

    pointer->destroy.notify = handle_destroy;
    wl_signal_add(&pointer->wlr_input_device->events.destroy, &pointer->destroy);
}

void wm_pointer_destroy(struct wm_pointer* pointer){
    wl_list_remove(&pointer->link);
}
