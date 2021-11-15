#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <stdlib.h>
#include <wayland-server.h>
#include <wlr/util/log.h>

#include "wm/wm_content.h"
#include "wm/wm_output.h"
#include "wm/wm_server.h"
#include "wm/wm_layout.h"

struct wm_content_vtable wm_content_base_vtable;

void wm_content_init(struct wm_content* content, struct wm_server* server) {
    content->vtable = &wm_content_base_vtable;

    content->wm_server = server;
    content->display_x = 0.;
    content->display_y = 0.;
    content->display_width = 0.;
    content->display_height = 0.;
    content->corner_radius = 0.;

    content->fixed_output = NULL;
    content->workspace_x = 0.;
    content->workspace_y = 0.;
    content->workspace_width = -1.;
    content->workspace_height = -1.;


    content->z_index = 0;
    wl_list_insert(&content->wm_server->wm_contents, &content->link);

    content->lock_enabled = false;
}

void wm_content_base_destroy(struct wm_content* content) {
    wl_list_remove(&content->link);
}

void wm_content_set_output(struct wm_content* content, char* name){
    struct wm_output* res = NULL;
    if(name && strcmp(name, "")){
        struct wm_output* output;
        wl_list_for_each(output, &content->wm_server->wm_layout->wm_outputs, link){
            if(!strcmp(output->wlr_output->name, name)){
                res = output;
                break;
            }
        }
    }

    if(res != content->fixed_output){
        wm_layout_damage_from(content->wm_server->wm_layout, content, NULL);
        content->fixed_output = res;
        wm_layout_damage_from(content->wm_server->wm_layout, content, NULL);
    }
}

void wm_content_set_workspace(struct wm_content* content, double x, double y, double width, double height){
    if(fabs(content->workspace_x - x) +
            fabs(content->workspace_y - y) +
            fabs(content->workspace_width - width) +
            fabs(content->workspace_height - height) < 0.01) return;

    wm_layout_update_content_outputs(content->wm_server->wm_layout, content);

    wm_layout_damage_from(content->wm_server->wm_layout, content, NULL);
    content->workspace_x = x;
    content->workspace_y = y;
    content->workspace_width = width;
    content->workspace_height = height;
    wm_layout_damage_from(content->wm_server->wm_layout, content, NULL);
}

void wm_content_get_workspace(struct wm_content* content, double* workspace_x, double* workspace_y, double* workspace_width, double* workspace_height){
    *workspace_x = content->workspace_x;
    *workspace_y = content->workspace_y;
    *workspace_width = content->workspace_width;
    *workspace_height = content->workspace_height;
}

bool wm_content_has_workspace(struct wm_content* content){
    return !(content->workspace_width < 0 || content->workspace_height < 0);
}

void wm_content_set_box(struct wm_content* content, double x, double y, double width, double height) {
    if(fabs(content->display_x - x) +
            fabs(content->display_y - y) + 
            fabs(content->display_width - width) +
            fabs(content->display_height - height) < 0.01) return;

    wm_layout_update_content_outputs(content->wm_server->wm_layout, content);

    wm_layout_damage_from(content->wm_server->wm_layout, content, NULL);
    content->display_x = x;
    content->display_y = y;
    content->display_width = width;
    content->display_height = height;
    wm_layout_damage_from(content->wm_server->wm_layout, content, NULL);
}

void wm_content_get_box(struct wm_content* content, double* display_x, double* display_y, double* display_width, double* display_height){
    *display_x = content->display_x;
    *display_y = content->display_y;
    *display_width = content->display_width;
    *display_height = content->display_height;
}

void wm_content_set_z_index(struct wm_content* content, int z_index){
    if(z_index == content->z_index) return;

    content->z_index = z_index;
    wm_layout_damage_from(content->wm_server->wm_layout, content, NULL);
}
int wm_content_get_z_index(struct wm_content* content){
    return content->z_index;
}

void wm_content_set_opacity(struct wm_content* content, double opacity){
    if(fabs(content->opacity - opacity) < 0.01) return;

    content->opacity = opacity;
    wm_layout_damage_from(content->wm_server->wm_layout, content, NULL);
}

double wm_content_get_opacity(struct wm_content* content){
   return content->opacity;
}

void wm_content_set_mask(struct wm_content* content, double mask_x, double mask_y, double mask_w, double mask_h){
    if(fabs(content->mask_x - mask_x) +
            fabs(content->mask_y - mask_y) + 
            fabs(content->mask_w - mask_w) +
            fabs(content->mask_h - mask_h) < 0.01) return;

    content->mask_x = mask_x;
    content->mask_y = mask_y;
    content->mask_w = mask_w;
    content->mask_h = mask_h;

    wm_layout_damage_from(content->wm_server->wm_layout, content, NULL);
}
void wm_content_get_mask(struct wm_content* content, double* mask_x, double* mask_y, double* mask_w, double* mask_h){
    *mask_x = content->mask_x;
    *mask_y = content->mask_y;
    *mask_w = content->mask_w;
    *mask_h = content->mask_h;

    if(*mask_w < 0) *mask_w = -*mask_x + content->display_width + 1;
    if(*mask_h < 0) *mask_h = -*mask_h + content->display_height + 1;
}

void wm_content_set_corner_radius(struct wm_content* content, double corner_radius){
    if(fabs(content->corner_radius - corner_radius) < 0.01) return;

    content->corner_radius = corner_radius;
    wm_layout_damage_from(content->wm_server->wm_layout, content, NULL);
}

void wm_content_set_lock_enabled(struct wm_content* content, bool lock_enabled){
    if(lock_enabled == content->lock_enabled) return;

    content->lock_enabled = lock_enabled;
    wm_layout_damage_from(content->wm_server->wm_layout, content, NULL);
}

double wm_content_get_corner_radius(struct wm_content* content){
    return content->corner_radius;
}

void wm_content_render(struct wm_content* content, struct wm_output* output, pixman_region32_t* output_damage, struct timespec now){
    pixman_region32_t damage_on_workspace;
    pixman_region32_init(&damage_on_workspace);
    pixman_region32_copy(&damage_on_workspace, output_damage);
    if(wm_content_has_workspace(content)){
        int x = round((content->workspace_x - output->layout_x) * output->wlr_output->scale);
        int y = round((content->workspace_y - output->layout_y) * output->wlr_output->scale);
        int w = round(content->workspace_width * output->wlr_output->scale);
        int h = round(content->workspace_height * output->wlr_output->scale);
        pixman_region32_intersect_rect(&damage_on_workspace, &damage_on_workspace, x, y, w, h);
    }

    (*content->vtable->render)(content, output, &damage_on_workspace, now);

    pixman_region32_fini(&damage_on_workspace);
}

struct wm_content_vtable wm_content_base_vtable = {
    .destroy = wm_content_base_destroy,
};
