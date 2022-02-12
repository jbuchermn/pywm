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

/* #define DEBUG_COMPOSE_TREE */

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

void wm_composite_on_damage_below(struct wm_composite* comp, struct wm_output* output, struct wm_content* from, pixman_region32_t* damage){
    double x, y, w, h;
    wm_content_get_box(&comp->super, &x, &y, &w, &h);
    x -= output->layout_x;
    y -= output->layout_y;

    x *= output->wlr_output->scale;
    y *= output->wlr_output->scale;
    w *= output->wlr_output->scale;
    h *= output->wlr_output->scale;

    int extend = 0;
    if(comp->type == WM_COMPOSITE_BLUR){
        int radius = comp->params.n_params_int >= 1 ? comp->params.params_int[0] : 1;
        int passes = comp->params.n_params_int >= 2 ? comp->params.params_int[1] : 2;
        extend = blur_extend(passes, radius);
    }

    struct wlr_box box = {
        .x = x,
        .y = y,
        .width = w,
        .height = h,
    };

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

void wm_composite_apply(struct wm_composite* composite, struct wm_output* output, pixman_region32_t* damage, unsigned int from_buffer, struct timespec now){
    double display_x, display_y, display_w, display_h;
    wm_content_get_box(&composite->super, &display_x, &display_y, &display_w, &display_h);

    struct wlr_box box = {
        .x = round((display_x - output->layout_x) * output->wlr_output->scale),
        .y = round((display_y - output->layout_y) * output->wlr_output->scale),
        .width = round(display_w * output->wlr_output->scale),
        .height = round(display_h * output->wlr_output->scale)};

    if(composite->type == WM_COMPOSITE_BLUR){
        int radius = composite->params.n_params_int >= 1 ? composite->params.params_int[0] : 1;
        int passes = composite->params.n_params_int >= 2 ? composite->params.params_int[1] : 2;
        wm_renderer_apply_blur(composite->super.wm_server->wm_renderer, damage, blur_extend(passes, radius), &box, from_buffer,
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


//////////////////////////////////////////////////////////////////

static void wm_compose_insert(struct wm_output* output, struct wm_compose_tree* at, struct wm_composite* composite, struct wlr_box* composite_output_box){
    int extend = 0;
    if(composite->type == WM_COMPOSITE_BLUR){
        int radius = composite->params.n_params_int >= 1 ? composite->params.params_int[0] : 1;
        int passes = composite->params.n_params_int >= 2 ? composite->params.params_int[1] : 2;
        extend = blur_extend(passes, radius);
    }

    pixman_region32_t inters;
    pixman_region32_init(&inters);
    pixman_region32_intersect_rect(&inters, &at->leaf_input, composite_output_box->x, composite_output_box->y, composite_output_box->width, composite_output_box->height);
    if(pixman_region32_not_empty(&inters)){
        struct wm_compose_tree* new = calloc(1, sizeof(struct wm_compose_tree));
        new->parent = at;
        new->composite = composite;
        pixman_region32_init(&new->leaf_input);
        pixman_region32_init(&new->output);
        wl_list_init(&new->children);
        wl_list_insert(&at->children, &new->link);

        pixman_region32_union(&new->output, &new->output, &inters);
        pixman_region32_subtract(&at->leaf_input, &at->leaf_input, &inters);

        int nrects;
        /* Extend leaf input (at this point == total input) */
        pixman_box32_t *rects = pixman_region32_rectangles(&inters, &nrects);
        for (int i = 0; i < nrects; i++) {
            pixman_region32_union_rect(&new->leaf_input, &new->leaf_input,
                    rects[i].x1 - extend,
                    rects[i].y1 - extend,
                    rects[i].x2 - rects[i].x1 + 2*extend,
                    rects[i].y2 - rects[i].y1 + 2*extend);
        }
    }
    pixman_region32_fini(&inters);

    struct wm_compose_tree* it;
    wl_list_for_each(it, &at->children, link){
        if(it->composite == composite) continue;
        wm_compose_insert(output, it, composite, composite_output_box);
    }
}

void wm_compose_tree_destroy(struct wm_compose_tree* tree){
    wl_list_remove(&tree->link);
    struct wm_compose_tree* it, *tmp;
    wl_list_for_each_safe(it, tmp, &tree->children, link){
        wm_compose_tree_destroy(it);
        free(it);
    }

    pixman_region32_fini(&tree->output);
    pixman_region32_fini(&tree->leaf_input);
}

static void wm_compose_tree_printf(struct wm_compose_tree* tree, int indent){
    pixman_box32_t* leaf = pixman_region32_extents(&tree->leaf_input);
    pixman_box32_t* output = pixman_region32_extents(&tree->output);
    wlr_log(WLR_DEBUG, "%*stree %p (leaf: [%d %d %d %d]) -> [%d %d %d %d]", indent, "", tree->composite, leaf->x1, leaf->y1, leaf->x2, leaf->y2, output->x1, output->y1, output->x2, output->y2);
    struct wm_compose_tree* it;
    wl_list_for_each(it, &tree->children, link){
        wm_compose_tree_printf(it, indent + 2);
    }
}

struct wm_compose_tree* wm_compose_tree_from_damage(struct wm_server* server, struct wm_output* output, pixman_region32_t* damage, bool bypass){
    /* Ensure z-indes */
    wm_server_update_contents(server);

    struct wm_compose_tree* root = calloc(1, sizeof(struct wm_compose_tree));
    root->parent = NULL;
    root->composite = NULL;
    pixman_region32_init(&root->leaf_input);
    pixman_region32_init(&root->output);
    wl_list_init(&root->link);
    wl_list_init(&root->children);
    pixman_region32_union(&root->output, &root->output, damage);
    pixman_region32_union(&root->leaf_input, &root->leaf_input, damage);

    if(!bypass){
        struct wm_content* content;
        wl_list_for_each(content, &server->wm_contents, link) {
            if(!wm_content_is_composite(content)) continue;

            struct wm_composite* composite = wm_cast(wm_composite, content);

            double display_x, display_y, display_w, display_h;
            wm_content_get_box(&composite->super, &display_x, &display_y, &display_w, &display_h);
            struct wlr_box box = {
                .x = round((display_x - output->layout_x) * output->wlr_output->scale),
                .y = round((display_y - output->layout_y) * output->wlr_output->scale),
                .width = round(display_w * output->wlr_output->scale),
                .height = round(display_h * output->wlr_output->scale)};
            wm_compose_insert(output, root, composite, &box);
        }
    }
#ifdef DEBUG_COMPOSE_TREE
    wm_compose_tree_printf(root, 0);
#endif
    return root;
}
