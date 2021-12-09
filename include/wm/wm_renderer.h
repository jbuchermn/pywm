#ifndef WM_RENDERER_H
#define WM_RENDERER_H

#include <stdbool.h>
#include <wlr/render/wlr_renderer.h>
#include <pixman.h>


// TODO: Enable
/* #define WM_CUSTOM_RENDERER */

#ifdef WM_CUSTOM_RENDERER

// TODO: Generate
#define WM_CUSTOM_RENDERER_N_TEXTURE_SHADERS 10
#define WM_CUSTOM_RENDERER_N_PRIMITIVE_SHADERS 10

struct wm_output;

#include <GLES2/gl2.h>
struct wm_renderer_texture_shader {
    GLuint shader;

    /* Basic parameters */
    GLint proj;
    GLint tex;
    GLint alpha;
    GLint pos_attrib;
    GLint tex_attrib;

    GLint width;
    GLint height;

    /* pywm-defined additional parameters */
    GLint padding_l;
    GLint padding_t;
    GLint padding_r;
    GLint padding_b;
    GLint cornerradius;
    GLint lock_perc;
};

struct wm_renderer_texture_shaders {
    const char* name;

    struct wm_renderer_texture_shader rgba;
    struct wm_renderer_texture_shader rgbx;
    struct wm_renderer_texture_shader ext;

    struct wm_renderer_texture_shader lock_rgba;
    struct wm_renderer_texture_shader lock_rgbx;
    struct wm_renderer_texture_shader lock_ext;
};

struct wm_renderer_primitive_shader {
    GLuint shader;
    const char* name;

    /* Basic parameters */
    GLint proj;
    GLint alpha;
    GLint pos_attrib;
    GLint tex_attrib;

    GLint width;
    GLint height;

    /* Additional parameters */
    int n_params_int;
    int n_params_float;
    GLint params_float;
    GLint params_int;
};

#endif

struct wm_renderer {
    struct wm_server* wm_server;
    struct wlr_renderer* wlr_renderer;

#ifdef WM_CUSTOM_RENDERER
    struct wm_renderer_texture_shaders texture_shaders[WM_CUSTOM_RENDERER_N_TEXTURE_SHADERS];
    struct wm_renderer_primitive_shader primitive_shaders[WM_CUSTOM_RENDERER_N_PRIMITIVE_SHADERS];
#endif

    struct wm_renderer_texture_shaders* texture_shaders_selected;
    struct wm_renderer_primitive_shader* primitive_shader_selected;

    struct wm_output* current;
};

void wm_renderer_init(struct wm_renderer *renderer, struct wm_server *server);
void wm_renderer_destroy(struct wm_renderer *renderer);

#ifdef WM_CUSTOM_RENDERER

void wm_renderer_add_texture_shaders(struct wm_renderer* renderer, const char* name,
        const GLchar* vert_src,
        const GLchar* frag_src_rgba,
        const GLchar* frag_src_rgbx,
        const GLchar* frag_src_ext,
        const GLchar* frag_src_lock_rgba,
        const GLchar* frag_src_lock_rgbx,
        const GLchar* frag_src_lock_ext);


void wm_renderer_add_primitive_shader(struct wm_renderer* renderer, const char* name,
        const GLchar* vert_src, const GLchar* frag_src, int n_params_int, int n_params_float);

#endif

void wm_renderer_select_texture_shaders(struct wm_renderer* renderer, const char* name);
void wm_renderer_select_primitive_shader(struct wm_renderer* renderer, const char* name);

void wm_renderer_begin(struct wm_renderer *renderer, struct wm_output *output);
void wm_renderer_end(struct wm_renderer *renderer, pixman_region32_t *damage,
                     struct wm_output *output);
void wm_renderer_render_texture_at(struct wm_renderer *renderer,
                                   pixman_region32_t *damage,
                                   struct wlr_texture *texture,
                                   struct wlr_box *box, double opacity,
                                   struct wlr_box *mask,
                                   double corner_radius, double lock_perc);

void wm_renderer_render_primitive(struct wm_renderer* renderer,
                                  pixman_region32_t* damage,
                                  struct wlr_box* box,
                                  double opacity, int* params_int, float* params_float);


#endif
