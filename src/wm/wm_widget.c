#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>

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

    widget->primitive.name = NULL;
    widget->primitive.params_int = NULL;
    widget->primitive.params_float = NULL;
}

static void wm_widget_destroy(struct wm_content* super){
    struct wm_widget* widget = wm_cast(wm_widget, super);
    wlr_texture_destroy(widget->wlr_texture);

    free(widget->primitive.name);
    free(widget->primitive.params_int);
    free(widget->primitive.params_float);

    wm_content_base_destroy(super);
}

void wm_widget_set_pixels(struct wm_widget* widget, uint32_t format, uint32_t stride, uint32_t width, uint32_t height, const void* data){
    if(widget->wlr_texture){
        wlr_texture_write_pixels(widget->wlr_texture, stride, width, height, 0, 0, 0, 0, data);
    }else{
        widget->wlr_texture = wlr_texture_from_pixels(widget->super.wm_server->wm_renderer->wlr_renderer,
                format, stride, width, height, data);
    }
    wm_widget_set_primitive(widget, NULL, 0, NULL, 0, NULL);

    wm_layout_damage_from(widget->super.wm_server->wm_layout, &widget->super, NULL);
}

void wm_widget_set_primitive(struct wm_widget* widget, char* name, int n_params_int, int* params_int, int n_params_float, float* params_float){
    if(widget->primitive.name) free(widget->primitive.name);
    if(widget->primitive.params_int) free(widget->primitive.params_int);
    if(widget->primitive.params_float) free(widget->primitive.params_float);

    widget->primitive.name = NULL;
    widget->primitive.params_int = NULL;
    widget->primitive.params_float = NULL;

    widget->primitive.name = name;
    widget->primitive.params_int = params_int;
    widget->primitive.params_float = params_float;

    widget->primitive.n_params_int = n_params_int;
    widget->primitive.n_params_float = n_params_float;

    if(name && widget->wlr_texture){
        wlr_texture_destroy(widget->wlr_texture);
        widget->wlr_texture = NULL;
    }

    wm_layout_damage_from(widget->super.wm_server->wm_layout, &widget->super, NULL);
}


static void wm_widget_render(struct wm_content* super, struct wm_output* output, pixman_region32_t* output_damage, struct timespec now){
    struct wm_widget* widget = wm_cast(wm_widget, super);

    double display_x, display_y, display_w, display_h;
    wm_content_get_box(&widget->super, &display_x, &display_y, &display_w, &display_h);

    struct wlr_box box = {
        .x = round((display_x - output->layout_x) * output->wlr_output->scale),
        .y = round((display_y - output->layout_y) * output->wlr_output->scale),
        .width = round(display_w * output->wlr_output->scale),
        .height = round(display_h * output->wlr_output->scale)};

    if (widget->wlr_texture){

        double mask_x, mask_y, mask_w, mask_h;
        wm_content_get_mask(&widget->super, &mask_x, &mask_y, &mask_w, &mask_h);

        struct wlr_box mask = {
            .x = round((display_x - output->layout_x + mask_x) * output->wlr_output->scale),
            .y = round((display_y - output->layout_y + mask_y) * output->wlr_output->scale),
            .width = round(mask_w * output->wlr_output->scale),
            .height = round(mask_h * output->wlr_output->scale)};

        double corner_radius =
            wm_content_get_corner_radius(&widget->super) * output->wlr_output->scale;

        wm_renderer_render_texture_at(
                output->wm_server->wm_renderer, output_damage,
                NULL, widget->wlr_texture, &box,
                wm_content_get_opacity(super), &mask, corner_radius,
                super->lock_enabled ? 0.0 : super->wm_server->lock_perc);
    }else if(widget->primitive.name){
#ifdef WM_CUSTOM_RENDERER
        if(widget->primitive.n_params_int < output->wm_server->wm_renderer->primitive_shader_selected->n_params_int){
            wlr_log(WLR_ERROR, "Not enough int parameters (%d) for shader %s", widget->primitive.n_params_int, widget->primitive.name);
            return;
        }
        if(widget->primitive.n_params_float < output->wm_server->wm_renderer->primitive_shader_selected->n_params_float){
            wlr_log(WLR_ERROR, "Not enough float parameters (%d) for shader %s", widget->primitive.n_params_float, widget->primitive.name);
            return;
        }
        wm_renderer_select_primitive_shader(output->wm_server->wm_renderer, widget->primitive.name);
        wm_renderer_render_primitive(output->wm_server->wm_renderer, output_damage, &box, wm_content_get_opacity(super) * (1. - (super->lock_enabled ? 0.0 : super->wm_server->lock_perc)),
                                     widget->primitive.params_int, widget->primitive.params_float);
#endif
    }
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

    if(wm_content_has_workspace(super)){
        double workspace_x, workspace_y, workspace_w, workspace_h;
        wm_content_get_workspace(super, &workspace_x, &workspace_y,
                                 &workspace_w, &workspace_h);
        workspace_x = (workspace_x - output->layout_x) * output->wlr_output->scale;
        workspace_y = (workspace_y - output->layout_y) * output->wlr_output->scale;
        workspace_w *= output->wlr_output->scale;
        workspace_h *= output->wlr_output->scale;
        pixman_region32_intersect_rect(
            &region, &region,
            floor(workspace_x),
            floor(workspace_y),
            ceil(workspace_x + workspace_w) - floor(workspace_x),
            ceil(workspace_y + workspace_h) - floor(workspace_y));
    }

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
