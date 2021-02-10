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

#include "wm/wm_util.h"

struct wm_content_vtable wm_view_vtable;

void wm_view_base_init(struct wm_view* view, struct wm_server* server){
    wm_content_init(&view->super, server);

    view->super.vtable = &wm_view_vtable;

    /* Abstract class */
    view->vtable = NULL;

    view->mapped = false;
    view->accepts_input = true;
}

static void wm_view_base_destroy(struct wm_content* super){
    struct wm_view* view = wm_cast(wm_view, super);

    (view->vtable->destroy)(view);
    wm_content_base_destroy(super);
}

int wm_content_is_view(struct wm_content* content){
    return content->vtable == &wm_view_vtable;
}

struct wm_content_vtable wm_view_vtable = {
    .destroy = &wm_view_base_destroy
};
