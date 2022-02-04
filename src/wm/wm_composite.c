#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <assert.h>

#include "wm/wm_composite.h"
#include "wm/wm_server.h"
#include "wm/wm_output.h"
#include "wm/wm_renderer.h"
#include "wm/wm_layout.h"

#include "wm/wm_util.h"

struct wm_content_vtable wm_composite_vtable;

void wm_composite_init(struct wm_composite* comp, struct wm_server* server){
    wm_content_init(&comp->super, server);
    comp->super.vtable = &wm_composite_vtable;
    comp->type = WM_COMPOSITE_BLUR;

    comp->params.n_params_float = 0;
    comp->params.n_params_int = 0;
    comp->params.params_float = NULL;
    comp->params.params_int = NULL;
}

static void wm_composite_destroy(struct wm_content* super){
    wm_content_base_destroy(super);
    struct wm_composite* comp = wm_cast(wm_composite, super);

    free(comp->params.params_float);
    free(comp->params.params_int);
}

void wm_composite_set_type(struct wm_composite* comp, const char* type, int n_params_int, int* params_int, int n_params_float, float* params_float){
    assert(!strcmp(type, "blur"));
    comp->type = WM_COMPOSITE_BLUR;

    comp->params.n_params_int = n_params_int;
    comp->params.params_int = params_int;
    comp->params.n_params_float = n_params_float;
    comp->params.params_float = params_float;

    wm_layout_damage_from(comp->super.wm_server->wm_layout, &comp->super, NULL);
}

static void wm_composite_render(struct wm_content* super, struct wm_output* output, pixman_region32_t* output_damage, struct timespec now){
    struct wm_composite* comp = wm_cast(wm_composite, super);

    double display_x, display_y, display_w, display_h;
    wm_content_get_box(&comp->super, &display_x, &display_y, &display_w, &display_h);

    struct wlr_box box = {
        .x = round((display_x - output->layout_x) * output->wlr_output->scale),
        .y = round((display_y - output->layout_y) * output->wlr_output->scale),
        .width = round(display_w * output->wlr_output->scale),
        .height = round(display_h * output->wlr_output->scale)};

    if(comp->type == WM_COMPOSITE_BLUR){
        wm_renderer_apply_blur(super->wm_server->wm_renderer, output_damage, &box,
                comp->params.n_params_int >= 1 ? comp->params.params_int[0] : 1,
                comp->params.n_params_int >= 2 ? comp->params.params_int[1] : 2,
                output->wlr_output->scale * super->corner_radius);
    }

}

/* TODO */
#define WM_COMPOSITE_EXTEND 10

void wm_composite_on_damage_below(struct wm_composite* comp, struct wm_output* output, pixman_region32_t* damage){

    double x, y, w, h;
    wm_content_get_box(&comp->super, &x, &y, &w, &h);
    x -= output->layout_x;
    y -= output->layout_y;

    x *= output->wlr_output->scale;
    y *= output->wlr_output->scale;
    w *= output->wlr_output->scale;
    h *= output->wlr_output->scale;

    struct wlr_box box = {
        .x = x - WM_COMPOSITE_EXTEND,
        .y = y - WM_COMPOSITE_EXTEND,
        .width = w + 2*WM_COMPOSITE_EXTEND,
        .height = h + 2*WM_COMPOSITE_EXTEND,
    };

    int nrects;
    pixman_box32_t* rects = pixman_region32_rectangles(damage, &nrects);
    for(int i = 0; i < nrects; i++){
        struct wlr_box damage_box = {.x = rects[i].x1 - WM_COMPOSITE_EXTEND,
                                     .y = rects[i].y1 - WM_COMPOSITE_EXTEND,
                                     .width = rects[i].x2 - rects[i].x1 + 2*WM_COMPOSITE_EXTEND,
                                     .height = rects[i].y2 - rects[i].y1 + 2*WM_COMPOSITE_EXTEND};

        struct wlr_box inters;
        wlr_box_intersection(&inters, &damage_box, &box);
        if (wlr_box_empty(&inters))
            continue;

        pixman_region32_t region;
        pixman_region32_init(&region);
        pixman_region32_union_rect(&region, &region, inters.x, inters.y, inters.width, inters.height);
        wm_layout_damage_output(output->wm_layout, output, &region, &comp->super);
        pixman_region32_fini(&region);

    }
}

bool wm_content_is_composite(struct wm_content* content){
    return content->vtable == &wm_composite_vtable;
}

static void wm_composite_printf(FILE* file, struct wm_content* super){
    struct wm_composite* comp = wm_cast(wm_composite, super);
    fprintf(file, "wm_composite (%f, %f - %f, %f)\n", comp->super.display_x, comp->super.display_y, comp->super.display_width, comp->super.display_height);
}

struct wm_content_vtable wm_composite_vtable = {
    .destroy = &wm_composite_destroy,
    .render = &wm_composite_render,
    .damage_output = NULL,
    .printf = &wm_composite_printf
};
