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
    // Nop
}

static int blur_extend(int passes, int radius){
    return radius * pow(2., passes);
}

static int wm_composite_extend(struct wm_composite* comp){
    if(comp->type == WM_COMPOSITE_BLUR){
        int radius = comp->params.n_params_int >= 1 ? comp->params.params_int[0] : 1;
        int passes = comp->params.n_params_int >= 2 ? comp->params.params_int[1] : 2;
        return blur_extend(passes, radius);
    }
    return 0;
}

static void wm_composite_extend_box(struct wlr_box* box, struct wlr_box* comp_box, int extend){
    struct wlr_box extended_comp_box = {
        .x = comp_box->x - extend,
        .y = comp_box->y - extend,
        .width = comp_box->width + 2*extend,
        .height = comp_box->height + 2*extend,
    };

    struct wlr_box extended_box = {
        .x = box->x - extend,
        .y = box->y - extend,
        .width = box->width + 2*extend,
        .height = box->height + 2*extend,
    };

    wlr_box_intersection(box, &extended_comp_box, &extended_box);
}

static void wm_composite_get_effective_box(struct wm_composite* composite, struct wm_output* output, struct wlr_box* box){
    double display_x, display_y, display_w, display_h;
    wm_content_get_box(&composite->super, &display_x, &display_y, &display_w, &display_h);

    box->x = round((display_x - output->layout_x) * output->wlr_output->scale);
    box->y = round((display_y - output->layout_y) * output->wlr_output->scale);
    box->width = round(display_w * output->wlr_output->scale);
    box->height = round(display_h * output->wlr_output->scale);

    if(wm_content_has_workspace(&composite->super)){
        double ws_x, ws_y, ws_w, ws_h;
        wm_content_get_workspace(&composite->super, &ws_x, &ws_y, &ws_w, &ws_h);

        struct wlr_box workspace_box = {
            .x = round((ws_x - output->layout_x) * output->wlr_output->scale),
            .y = round((ws_y - output->layout_y) * output->wlr_output->scale),
            .width = round(ws_w * output->wlr_output->scale),
            .height = round(ws_h * output->wlr_output->scale)};
        wlr_box_intersection(box, box, &workspace_box);
    }
}

void wm_composite_on_damage_below(struct wm_composite* comp, struct wm_output* output, struct wm_content* from, pixman_region32_t* damage){
    struct wlr_box box;
    wm_composite_get_effective_box(comp, output, &box);

    int extend = wm_composite_extend(comp);
    int nrects;
    pixman_box32_t* rects = pixman_region32_rectangles(damage, &nrects);
    for(int i = 0; i < nrects; i++){
        struct wlr_box damage_box = {.x = rects[i].x1 - extend,
                                     .y = rects[i].y1 - extend,
                                     .width = rects[i].x2 - rects[i].x1 + 2*extend,
                                     .height = rects[i].y2 - rects[i].y1 + 2*extend};

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

void wm_composite_apply(struct wm_composite* composite, struct wm_output* output, pixman_region32_t* damage, struct timespec now){
    struct wlr_box box;
    wm_composite_get_effective_box(composite, output, &box);

    if(composite->type == WM_COMPOSITE_BLUR){
        int radius = composite->params.n_params_int >= 1 ? composite->params.params_int[0] : 1;
        int passes = composite->params.n_params_int >= 2 ? composite->params.params_int[1] : 2;
        wm_renderer_apply_blur(composite->super.wm_server->wm_renderer, damage, blur_extend(passes, radius), &box,
                radius, passes,
                output->wlr_output->scale * composite->super.corner_radius);
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


struct wm_compose_chain* wm_compose_chain_from_damage(struct wm_server* server, struct wm_output* output, pixman_region32_t* damage){

    struct wm_compose_chain* result = calloc(1, sizeof(struct wm_compose_chain));
    pixman_region32_init(&result->damage);
    pixman_region32_union(&result->damage, &result->damage, damage);

    pixman_region32_init(&result->composite_output);

    struct wm_compose_chain* at = result;

    struct wm_content* content;
    bool initial = true;

    wl_list_for_each(content, &server->wm_contents, link){
        if(initial) result->z_index = wm_content_get_z_index(content) + 1.;

        if(wm_content_is_composite(content)){
            at->lower = calloc(1, sizeof(struct wm_compose_chain));
            at->lower->higher = at;
            at = at->lower;

            at->composite = wm_cast(wm_composite, content);
            at->z_index = wm_content_get_z_index(content);
            pixman_region32_init(&at->damage);
            pixman_region32_init(&at->composite_output);


            int extend = wm_composite_extend(at->composite);
            struct wlr_box box;
            wm_composite_get_effective_box(at->composite, output, &box);

            int nrects;
            pixman_box32_t* rects = pixman_region32_rectangles(&at->higher->damage, &nrects);
            for(int i=0; i<nrects; i++){
                struct wlr_box damage_box = {
                    .x = rects[i].x1,
                    .y = rects[i].y1,
                    .width = rects[i].x2 - rects[i].x1,
                    .height = rects[i].y2 - rects[i].y1
                };

                struct wlr_box composite_output;
                wlr_box_intersection(&composite_output, &box, &damage_box);
                pixman_region32_union_rect(&at->composite_output, &at->composite_output, 
                        composite_output.x, composite_output.y, composite_output.width, composite_output.height);


                pixman_region32_union_rect(&at->damage, &at->damage,
                        damage_box.x, damage_box.y, damage_box.width, damage_box.height);

                wm_composite_extend_box(&damage_box, &box, extend);
                pixman_region32_union_rect(&at->damage, &at->damage,
                        damage_box.x, damage_box.y, damage_box.width, damage_box.height);

            }

            if(!pixman_region32_not_empty(&at->composite_output)){
                at = at->higher;
                wm_compose_chain_free(at->lower);
                at->lower = NULL;
            }
        }

        initial = false;
    }

    return result;
}

void wm_compose_chain_free(struct wm_compose_chain* chain){
    if(chain->lower){
        wm_compose_chain_free(chain->lower);
    }
    pixman_region32_fini(&chain->damage);
    pixman_region32_fini(&chain->composite_output);
    free(chain);
}
