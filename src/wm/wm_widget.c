#include "wm/wm_widget.h"
#include "wm/wm_server.h"

#include "wm/wm_util.h"

void wm_widget_init(struct wm_widget* widget, struct wm_server* server){
    wm_content_init(&widget->super, server);

    widget->wlr_texture = NULL;
}

static void wm_widget_destroy(struct wm_content* super){
    struct wm_widget* widget = wm_cast(wm_widget, super);
    wlr_texture_destroy(widget->wlr_texture);

    wm_content_destroy(super);
}

void wm_widget_set_pixels(struct wm_widget* widget, enum wl_shm_format format, uint32_t stride, uint32_t width, uint32_t height, const void* data){
    if(widget->wlr_texture){
        wlr_texture_write_pixels(widget->wlr_texture, stride, width, height, 0, 0, 0, 0, data);
    }else{
        widget->wlr_texture = wlr_texture_from_pixels(widget->super.wm_server->wlr_renderer, format, stride, width, height, data);
    }
}

struct wm_content_vtable wm_widget_vtable = {
    .destroy = &wm_widget_destroy,
};
