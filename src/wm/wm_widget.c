#include "wm/wm_widget.h"
#include "wm/wm_server.h"
#include "wm/wm_output.h"
#include "wm/wm_renderer.h"
#include "wm/wm_layout.h"

#include "wm/wm_util.h"

struct wm_content_vtable wm_widget_vtable;

void wm_widget_init(struct wm_widget* widget, struct wm_server* server){
    wm_content_init(&widget->super, server);
    widget->super.vtable = &wm_widget_vtable;

    widget->wlr_texture = NULL;
    widget->wm_output = NULL;
}

static void wm_widget_destroy(struct wm_content* super){
    struct wm_widget* widget = wm_cast(wm_widget, super);
    wlr_texture_destroy(widget->wlr_texture);

    wm_content_base_destroy(super);
}

void wm_widget_set_pixels(struct wm_widget* widget, uint32_t format, uint32_t stride, uint32_t width, uint32_t height, const void* data){
    if(widget->wlr_texture){
        wlr_texture_write_pixels(widget->wlr_texture, stride, width, height, 0, 0, 0, 0, data);
    }else{
        widget->wlr_texture = wlr_texture_from_pixels(widget->super.wm_server->wm_renderer->wlr_renderer,
                format, stride, width, height, data);
    }
    wm_layout_damage_from(widget->super.wm_server->wm_layout, &widget->super, NULL);
}

void wm_widget_set_output(struct wm_widget* widget, char* name){
    widget->wm_output = NULL;
    if(!name || !strcmp(name, "")) return;

    struct wm_output* output;
    wl_list_for_each(output, &widget->super.wm_server->wm_layout->wm_outputs, link){
        if(!strcmp(output->wlr_output->name, name)) widget->wm_output = output;
    }
}

static void wm_widget_render(struct wm_content* super, struct wm_output* output, pixman_region32_t* output_damage, struct timespec now){
    struct wm_widget* widget = wm_cast(wm_widget, super);
    if(widget->wm_output && output != widget->wm_output) return;

    if (!widget->wlr_texture)
        return;

    struct wlr_box box = {
        .x = round((widget->super.display_x - output->layout_x) * output->wlr_output->scale),
        .y = round((widget->super.display_y - output->layout_y) * output->wlr_output->scale),
        .width = round(widget->super.display_width * output->wlr_output->scale),
        .height =
            round(widget->super.display_height * output->wlr_output->scale)};
    double corner_radius =
        wm_content_get_corner_radius(&widget->super) * output->wlr_output->scale;

    double mask_x, mask_y, mask_w, mask_h;
    wm_content_get_mask(super, &mask_x, &mask_y, &mask_w, &mask_h);

    double mask_l = fmax(0., mask_x * output->wlr_output->scale);
    double mask_t = fmax(0., mask_y * output->wlr_output->scale);
    double mask_r = fmax(0., box.width - (mask_x + mask_w) * output->wlr_output->scale);
    double mask_b = fmax(0., box.height - (mask_y + mask_h) * output->wlr_output->scale);

    wm_renderer_render_texture_at(
            output->wm_server->wm_renderer, output_damage,
            widget->wlr_texture, &box,
            wm_content_get_opacity(super), mask_l, mask_t, mask_r, mask_b, corner_radius,
            super->lock_enabled ? 0.0 : super->wm_server->lock_perc);

}

static void wm_widget_damage_output(struct wm_content* super, struct wm_output* output, struct wlr_surface* origin){

    pixman_region32_t region;
    pixman_region32_init(&region);

    double x, y, w, h;
    wm_content_get_box(super, &x, &y, &w, &h);
    x -= output->layout_x;
    y -= output->layout_y;

    x *= output->wlr_output->scale;
    y *= output->wlr_output->scale;
    w *= output->wlr_output->scale;
    h *= output->wlr_output->scale;
    pixman_region32_union_rect(&region, &region,
            floor(x), floor(y),
            ceil(x + w) - floor(x), ceil(y + h) - floor(y));

    wlr_output_damage_add(output->wlr_output_damage, &region);
    pixman_region32_fini(&region);
}

static void wm_widget_printf(FILE* file, struct wm_content* super){
    struct wm_widget* widget = wm_cast(wm_widget, super);
    fprintf(file, "wm_widget (%f, %f - %f, %f)\n", widget->super.display_x, widget->super.display_y, widget->super.display_width, widget->super.display_height);
}

struct wm_content_vtable wm_widget_vtable = {
    .destroy = &wm_widget_destroy,
    .render = &wm_widget_render,
    .damage_output = &wm_widget_damage_output,
    .printf = &wm_widget_printf
};
