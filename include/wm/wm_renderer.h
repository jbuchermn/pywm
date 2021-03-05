#ifndef WM_RENDERER_H
#define WM_RENDERER_H

#include <stdbool.h>
#include <wlr/render/wlr_renderer.h>

#define WM_CUSTOM_RENDERER

struct wm_output;

#ifdef WM_CUSTOM_RENDERER

#include <GLES2/gl2.h>
struct wm_renderer_shader {
    GLuint shader;
    GLint proj;
    GLint invert_y;
    GLint tex;
    GLint alpha;
    GLint pos_attrib;
    GLint tex_attrib;
    GLint width;
    GLint height;

    GLint cornerradius;

    GLint blur_offset;
    GLint blur_weight;
};

#endif

struct wm_renderer {
    struct wm_server* wm_server;
    struct wlr_renderer* wlr_renderer;

    struct wm_output* current;

#ifdef WM_CUSTOM_RENDERER
    /* Custom shaders */
    struct wm_renderer_shader shader_rgba;
    struct wm_renderer_shader shader_rgbx;

    struct wm_renderer_shader shader_blurred_rgba;
    struct wm_renderer_shader shader_blurred_rgbx;
#endif
};

void wm_renderer_init(struct wm_renderer *renderer, struct wm_server *server);
void wm_renderer_destroy(struct wm_renderer *renderer);

void wm_renderer_begin(struct wm_renderer *renderer, struct wm_output *output);
void wm_renderer_end(struct wm_renderer *renderer, pixman_region32_t *damage,
                     struct wm_output *output);
void wm_renderer_render_texture_at(struct wm_renderer *renderer,
                                   pixman_region32_t *damage,
                                   struct wlr_texture *texture,
                                   struct wlr_box *box, double opacity,
                                   double corner_radius, bool blurred);

#endif
