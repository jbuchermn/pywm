#ifndef WM_RENDERER_H
#define WM_RENDERER_H

#include <stdbool.h>
#include <wlr/types/wlr_surface.h>
#include <wlr/render/wlr_renderer.h>
#include <pixman.h>


#ifdef WM_CUSTOM_RENDERER

struct wm_output;
struct wm_renderer;

#include <GLES3/gl32.h>
struct wm_renderer_texture_shader {
    GLuint shader;

    /* Basic parameters */
    GLint proj;
    GLint tex;
    GLint alpha;
    GLint pos_attrib;
    GLint tex_attrib;

    GLint offset_x;
    GLint offset_y;
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

#define WM_RENDERER_DOWNSAMPLE_BUFFERS 3

struct wm_renderer_buffers {
    int width;
    int height;
    struct wm_renderer* parent;

    GLuint frame_buffer;
    GLuint frame_buffer_rbo;
    GLuint frame_buffer_tex;

    GLuint downsample_buffers[WM_RENDERER_DOWNSAMPLE_BUFFERS];
    GLuint downsample_buffers_rbo[WM_RENDERER_DOWNSAMPLE_BUFFERS];
    GLuint downsample_buffers_tex[WM_RENDERER_DOWNSAMPLE_BUFFERS];

    int downsample_buffers_width[WM_RENDERER_DOWNSAMPLE_BUFFERS];
    int downsample_buffers_height[WM_RENDERER_DOWNSAMPLE_BUFFERS];
};

void wm_renderer_buffers_init(struct wm_renderer_buffers* buffers, struct wm_renderer* renderer, int width, int height);
void wm_renderer_buffers_destroy(struct wm_renderer_buffers* buffers);
void wm_renderer_buffers_ensure(struct wm_renderer* renderer, struct wm_output* output);

#endif

struct wm_renderer {
    struct wm_server* wm_server;
    struct wlr_renderer* wlr_renderer;

    struct wm_output* current;

#ifdef WM_CUSTOM_RENDERER
    int n_texture_shaders;
    struct wm_renderer_texture_shaders* texture_shaders;

    struct {
        GLuint shader;
        GLint tex;
        GLint pos_attrib;
        GLint tex_attrib;
    } quad_shader;
    struct {
        GLuint shader;
        GLint tex;
        GLint pos_attrib;
        GLint tex_attrib;

        GLint halfpixel;
        GLint offset;
    } downsample_shader;
    struct {
        GLuint shader;
        GLint tex;
        GLint pos_attrib;
        GLint tex_attrib;

        GLint halfpixel;
        GLint offset;

        GLint width;
        GLint height;
        GLint padding_l;
        GLint padding_t;
        GLint padding_r;
        GLint padding_b;
        GLint cornerradius;
    } upsample_shader;

    int n_primitive_shaders;
    struct wm_renderer_primitive_shader* primitive_shaders;

    struct wm_renderer_texture_shaders* texture_shaders_selected;
    struct wm_renderer_primitive_shader* primitive_shader_selected;
#endif
};

void wm_renderer_init(struct wm_renderer *renderer, struct wm_server *server);
void wm_renderer_destroy(struct wm_renderer *renderer);
int wm_renderer_init_output(struct wm_renderer* renderer, struct wm_output* output);

#ifdef WM_CUSTOM_RENDERER

void wm_renderer_init_texture_shaders(struct wm_renderer* renderer, int n_shaders);
void wm_renderer_add_texture_shaders(struct wm_renderer* renderer, const char* name,
        const GLchar* vert_src,
        const GLchar* frag_src_rgba,
        const GLchar* frag_src_rgbx,
        const GLchar* frag_src_ext,
        const GLchar* frag_src_lock_rgba,
        const GLchar* frag_src_lock_rgbx,
        const GLchar* frag_src_lock_ext);


void wm_renderer_init_primitive_shaders(struct wm_renderer* renderer, int n_shaders);
void wm_renderer_add_primitive_shader(struct wm_renderer* renderer, const char* name,
        const GLchar* vert_src, const GLchar* frag_src, int n_params_int, int n_params_float);

#endif

void wm_renderer_select_texture_shaders(struct wm_renderer* renderer, const char* name);
void wm_renderer_select_primitive_shader(struct wm_renderer* renderer, const char* name);

void wm_renderer_begin(struct wm_renderer *renderer, struct wm_output *output);
void wm_renderer_end(struct wm_renderer *renderer, pixman_region32_t *damage,
                     struct wm_output *output, bool clear_before);
void wm_renderer_render_texture_at(struct wm_renderer *renderer,
                                   pixman_region32_t *damage,
                                   struct wlr_surface* surface,
                                   struct wlr_texture *texture,
                                   struct wlr_box *box, double opacity,
                                   struct wlr_box *mask,
                                   double corner_radius, double lock_perc);

void wm_renderer_render_primitive(struct wm_renderer* renderer,
                                  pixman_region32_t* damage,
                                  struct wlr_box* box,
                                  double opacity, int* params_int, float* params_float);

void wm_renderer_apply_blur(struct wm_renderer* renderer,
                            pixman_region32_t* damage,
                            struct wlr_box* box,
                            int radius,
                            double cornerradius);

void wm_renderer_clear(struct wm_renderer* renderer,
                       pixman_region32_t* damage,
                       float* color);

#endif
