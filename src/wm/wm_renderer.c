#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-server.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_matrix.h>

#include "wm/wm_output.h"
#include "wm/wm_renderer.h"
#include "wm/wm_server.h"
#include "wm/wm_config.h"

#ifdef WM_CUSTOM_RENDERER

#include <render/gles2.h>
#include "wm/shaders/wm_shaders.h"

static const GLfloat verts[] = {
    1, 0, // top right
    0, 0, // top left
    1, 1, // bottom right
    0, 1, // bottom left
};

static const float flip_180[9] = {
    1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
};

static GLuint compile_shader(struct wlr_gles2_renderer *renderer, GLuint type,
                             const GLchar *src) {
    push_gles2_debug(renderer);

    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok == GL_FALSE) {
        glDeleteShader(shader);
        shader = 0;
    }

    pop_gles2_debug(renderer);
    return shader;
}

static GLuint wm_renderer_link_program(struct wm_renderer *renderer,
                                const GLchar *vert_src,
                                const GLchar *frag_src) {
    struct wlr_gles2_renderer *gles2_renderer =
        gles2_get_renderer(renderer->wlr_renderer);
    push_gles2_debug(gles2_renderer);

    GLuint vert = compile_shader(gles2_renderer, GL_VERTEX_SHADER,
                                 vert_src);
    if (!vert) {
        goto error;
    }

    GLuint frag = compile_shader(gles2_renderer,
                                 GL_FRAGMENT_SHADER, frag_src);
    if (!frag) {
        glDeleteShader(vert);
        goto error;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);

    glDetachShader(prog, vert);
    glDetachShader(prog, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);

    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (ok == GL_FALSE) {
        glDeleteProgram(prog);
        goto error;
    }

    pop_gles2_debug(gles2_renderer);
    return prog;

error:
    return 0;
}

static void wm_renderer_link_texture_shader(struct wm_renderer *renderer,
                                     struct wm_renderer_texture_shader *shader,
                                     const GLchar *vert_src,
                                     const GLchar *frag_src) {
    shader->shader = wm_renderer_link_program(renderer, vert_src, frag_src);
    assert(shader->shader);

    shader->proj = glGetUniformLocation(shader->shader, "proj");
    shader->tex = glGetUniformLocation(shader->shader, "tex");
    shader->alpha = glGetUniformLocation(shader->shader, "alpha");
    shader->width = glGetUniformLocation(shader->shader, "width");
    shader->height = glGetUniformLocation(shader->shader, "height");
    shader->padding_l = glGetUniformLocation(shader->shader, "padding_l");
    shader->padding_t = glGetUniformLocation(shader->shader, "padding_t");
    shader->padding_r = glGetUniformLocation(shader->shader, "padding_r");
    shader->padding_b = glGetUniformLocation(shader->shader, "padding_b");
    shader->cornerradius = glGetUniformLocation(shader->shader, "cornerradius");
    shader->lock_perc = glGetUniformLocation(shader->shader, "lock_perc");

    shader->pos_attrib = glGetAttribLocation(shader->shader, "pos");
    shader->tex_attrib = glGetAttribLocation(shader->shader, "texcoord");
}

void wm_renderer_add_texture_shaders(
    struct wm_renderer *renderer, const char *name, const GLchar *vert_src,
    const GLchar *frag_src_rgba, const GLchar *frag_src_rgbx,
    const GLchar *frag_src_ext, const GLchar *frag_src_lock_rgba,
    const GLchar *frag_src_lock_rgbx, const GLchar *frag_src_lock_ext) {

    struct wlr_gles2_renderer *gles2_renderer =
        gles2_get_renderer(renderer->wlr_renderer);

    int i = 0;
    for (; i < WM_CUSTOM_RENDERER_N_TEXTURE_SHADERS; i++) {
        if (!renderer->texture_shaders[i].name)
            break;
    }
    assert(i < WM_CUSTOM_RENDERER_N_TEXTURE_SHADERS);

    renderer->texture_shaders[i].name = strdup(name);

    wm_renderer_link_texture_shader(
        renderer, &renderer->texture_shaders[i].rgba, vert_src, frag_src_rgba);
    wm_renderer_link_texture_shader(renderer,
                                    &renderer->texture_shaders[i].lock_rgba,
                                    vert_src, frag_src_lock_rgba);

    wm_renderer_link_texture_shader(
        renderer, &renderer->texture_shaders[i].rgbx, vert_src, frag_src_rgbx);
    wm_renderer_link_texture_shader(renderer,
                                    &renderer->texture_shaders[i].lock_rgbx,
                                    vert_src, frag_src_lock_rgbx);

    if (gles2_renderer->exts.OES_egl_image_external) {
        wm_renderer_link_texture_shader(renderer,
                                        &renderer->texture_shaders[i].ext,
                                        vert_src, frag_src_ext);
        wm_renderer_link_texture_shader(renderer,
                                        &renderer->texture_shaders[i].lock_ext,
                                        vert_src, frag_src_lock_ext);
    }
}

void wm_renderer_add_primitive_shader(struct wm_renderer *renderer,
                                      const char *name, const GLchar *vert_src,
                                      const GLchar *frag_src, int n_params_int, int n_params_float) {

    int i = 0;
    for (; i < WM_CUSTOM_RENDERER_N_PRIMITIVE_SHADERS; i++) {
        if (!renderer->primitive_shaders[i].name)
            break;
    }
    assert(i < WM_CUSTOM_RENDERER_N_PRIMITIVE_SHADERS);

    renderer->primitive_shaders[i].name = strdup(name);
    renderer->primitive_shaders[i].n_params_float = n_params_float;
    renderer->primitive_shaders[i].n_params_int = n_params_int;

    renderer->primitive_shaders[i].shader = wm_renderer_link_program(renderer, vert_src, frag_src);
    assert(renderer->primitive_shaders[i].shader);

    renderer->primitive_shaders[i].proj = glGetUniformLocation(renderer->primitive_shaders[i].shader, "proj");
    renderer->primitive_shaders[i].alpha = glGetUniformLocation(renderer->primitive_shaders[i].shader, "alpha");
    renderer->primitive_shaders[i].width = glGetUniformLocation(renderer->primitive_shaders[i].shader, "width");
    renderer->primitive_shaders[i].height = glGetUniformLocation(renderer->primitive_shaders[i].shader, "height");
    renderer->primitive_shaders[i].pos_attrib = glGetAttribLocation(renderer->primitive_shaders[i].shader, "pos");
    renderer->primitive_shaders[i].tex_attrib = glGetAttribLocation(renderer->primitive_shaders[i].shader, "texcoord");

    if(renderer->primitive_shader_selected->n_params_float){
        renderer->primitive_shaders[i].params_float = glGetUniformLocation(renderer->primitive_shaders[i].shader, "params_float");
    }
    if(renderer->primitive_shader_selected->n_params_int){
        renderer->primitive_shaders[i].params_int = glGetUniformLocation(renderer->primitive_shaders[i].shader, "params_int");
    }
}

#endif

void wm_renderer_select_texture_shaders(struct wm_renderer *renderer,
                                        const char *name) {
#ifdef WM_CUSTOM_RENDERER
    int i = 0;
    for (; i < WM_CUSTOM_RENDERER_N_TEXTURE_SHADERS; i++) {
        if (renderer->texture_shaders[i].name && !strcmp(renderer->texture_shaders[i].name, name))
            break;
    }

    if (i < WM_CUSTOM_RENDERER_N_TEXTURE_SHADERS){
        renderer->texture_shaders_selected = renderer->texture_shaders + i;
    }else{
        wlr_log(WLR_INFO, "Could not find texture shaders '%s' - defaulting", name);
        renderer->texture_shaders_selected = renderer->texture_shaders;
    }
#else
    // noop
#endif
}

void wm_renderer_select_primitive_shader(struct wm_renderer *renderer,
                                         const char *name) {
#ifdef WM_CUSTOM_RENDERER
    int i = 0;
    for (; i < WM_CUSTOM_RENDERER_N_PRIMITIVE_SHADERS; i++) {
        if (renderer->primitive_shaders[i].name && !strcmp(renderer->primitive_shaders[i].name, name))
            break;
    }
    if (i < WM_CUSTOM_RENDERER_N_PRIMITIVE_SHADERS){
        renderer->primitive_shader_selected = renderer->primitive_shaders + i;
    }else{
        wlr_log(WLR_INFO, "Could not find primitive shader '%s' - defaulting", name);
        renderer->primitive_shader_selected = renderer->primitive_shaders;
    }
#else
    // noop
#endif
}

#ifdef WM_CUSTOM_RENDERER

static bool render_subtexture_with_matrix(
    struct wm_renderer *renderer, struct wlr_texture *wlr_texture,
    const struct wlr_fbox *box, const float matrix[static 9], float alpha,
    const struct wlr_box *display_box, double padding_l, double padding_t,
    double padding_r, double padding_b, float corner_radius, double lock_perc) {

    struct wlr_gles2_renderer *gles2_renderer =
        gles2_get_renderer(renderer->wlr_renderer);
    struct wlr_gles2_texture *texture = gles2_get_texture(wlr_texture);

    struct wm_renderer_texture_shader *shader = NULL;

    switch (texture->target) {
    case GL_TEXTURE_2D:
        if (texture->has_alpha) {
            shader = lock_perc > 0.001 ? &renderer->texture_shaders_selected->lock_rgba
                                       : &renderer->texture_shaders_selected->rgba;
        } else {
            shader = lock_perc > 0.001 ? &renderer->texture_shaders_selected->lock_rgbx
                                       : &renderer->texture_shaders_selected->rgbx;
        }
        break;
    case GL_TEXTURE_EXTERNAL_OES:
        shader = lock_perc > 0.001 ? &renderer->texture_shaders_selected->lock_ext
                                   : &renderer->texture_shaders_selected->ext;

        if (!gles2_renderer->exts.OES_egl_image_external) {
            wlr_log(WLR_ERROR, "Failed to render texture: "
                               "GL_TEXTURE_EXTERNAL_OES not supported");
            return false;
        }
        break;
    default:
        abort();
    }

    float gl_matrix[9];
    wlr_matrix_multiply(gl_matrix, gles2_renderer->projection, matrix);
    wlr_matrix_multiply(gl_matrix, flip_180, gl_matrix);

    // OpenGL ES 2 requires the glUniformMatrix3fv transpose parameter to be set
    // to GL_FALSE
    wlr_matrix_transpose(gl_matrix, gl_matrix);

    push_gles2_debug(gles2_renderer);

    if (!texture->has_alpha && alpha == 1.0) {
        glDisable(GL_BLEND);
    } else {
        glEnable(GL_BLEND);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(texture->target, texture->tex);

    glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glUseProgram(shader->shader);

    glUniformMatrix3fv(shader->proj, 1, GL_FALSE, gl_matrix);
    glUniform1i(shader->tex, 0);
    glUniform1f(shader->alpha, alpha);
    glUniform1f(shader->width, display_box->width);
    glUniform1f(shader->height, display_box->height);
    glUniform1f(shader->padding_l, padding_l);
    glUniform1f(shader->padding_t, padding_t);
    glUniform1f(shader->padding_r, padding_r);
    glUniform1f(shader->padding_b, padding_b);
    glUniform1f(shader->cornerradius, corner_radius);
    glUniform1f(shader->lock_perc, lock_perc);

    const GLfloat x1 = box->x / wlr_texture->width;
    const GLfloat y1 = box->y / wlr_texture->height;
    const GLfloat x2 = (box->x + box->width) / wlr_texture->width;
    const GLfloat y2 = (box->y + box->height) / wlr_texture->height;
    const GLfloat texcoord[] = {
        x2, y1, // top right
        x1, y1, // top left
        x2, y2, // bottom right
        x1, y2, // bottom left
    };

    glVertexAttribPointer(shader->pos_attrib, 2, GL_FLOAT, GL_FALSE, 0, verts);
    glVertexAttribPointer(shader->tex_attrib, 2, GL_FLOAT, GL_FALSE, 0,
                          texcoord);

    glEnableVertexAttribArray(shader->pos_attrib);
    glEnableVertexAttribArray(shader->tex_attrib);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(shader->pos_attrib);
    glDisableVertexAttribArray(shader->tex_attrib);

    glBindTexture(texture->target, 0);

    pop_gles2_debug(gles2_renderer);
    return true;
}

static void render_primitive_with_matrix(
    struct wm_renderer *renderer,
    const struct wlr_fbox *box, const float matrix[static 9], float alpha, const GLint* params_int, const GLfloat* params_float){

    struct wlr_gles2_renderer *gles2_renderer =
        gles2_get_renderer(renderer->wlr_renderer);

    float gl_matrix[9];
    wlr_matrix_multiply(gl_matrix, gles2_renderer->projection, matrix);
    wlr_matrix_multiply(gl_matrix, flip_180, gl_matrix);

    // OpenGL ES 2 requires the glUniformMatrix3fv transpose parameter to be set
    // to GL_FALSE
    wlr_matrix_transpose(gl_matrix, gl_matrix);

    push_gles2_debug(gles2_renderer);

    glEnable(GL_BLEND);

    glUseProgram(renderer->primitive_shader_selected->shader);

    glUniformMatrix3fv(renderer->primitive_shader_selected->proj, 1, GL_FALSE, gl_matrix);
    glUniform1f(renderer->primitive_shader_selected->alpha, alpha);
    glUniform1f(renderer->primitive_shader_selected->width, box->width);
    glUniform1f(renderer->primitive_shader_selected->height, box->height);

    if(renderer->primitive_shader_selected->n_params_int){
        glUniform1iv(renderer->primitive_shader_selected->params_int, renderer->primitive_shader_selected->n_params_int, params_int);
    }
    if(renderer->primitive_shader_selected->n_params_float){
        glUniform1fv(renderer->primitive_shader_selected->params_float, renderer->primitive_shader_selected->n_params_float, params_float);
    }

    const GLfloat texcoord[] = {
        1, 0, // top right
        0, 0, // top left
        1, 1, // bottom right
        0, 1, // bottom left
    };

    glVertexAttribPointer(renderer->primitive_shader_selected->pos_attrib, 2, GL_FLOAT,
                          GL_FALSE, 0, verts);
    glVertexAttribPointer(renderer->primitive_shader_selected->tex_attrib, 2, GL_FLOAT, GL_FALSE, 0,
                          texcoord);
    glEnableVertexAttribArray(renderer->primitive_shader_selected->pos_attrib);
    glEnableVertexAttribArray(renderer->primitive_shader_selected->tex_attrib);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(renderer->primitive_shader_selected->pos_attrib);
    glDisableVertexAttribArray(renderer->primitive_shader_selected->tex_attrib);

    pop_gles2_debug(gles2_renderer);
}

#endif


void wm_renderer_init(struct wm_renderer *renderer, struct wm_server *server) {
    renderer->wm_server = server;

    renderer->wlr_renderer = wlr_renderer_autocreate(server->wlr_backend);
    assert(renderer->wlr_renderer);

    wlr_renderer_init_wl_display(renderer->wlr_renderer, server->wl_display);

    renderer->current = NULL;

#ifdef WM_CUSTOM_RENDERER
    struct wlr_gles2_renderer *r = gles2_get_renderer(renderer->wlr_renderer);
    assert(r);

    for (int i = 0; i < WM_CUSTOM_RENDERER_N_TEXTURE_SHADERS; i++)
        renderer->texture_shaders[i].name = 0;
    for (int i = 0; i < WM_CUSTOM_RENDERER_N_PRIMITIVE_SHADERS; i++)
        renderer->primitive_shaders[i].name = 0;
    renderer->texture_shaders_selected = renderer->texture_shaders;
    renderer->primitive_shader_selected = renderer->primitive_shaders;

    assert(wlr_egl_make_current(r->egl));
    wm_texture_shaders_init(renderer);
    wm_primitive_shaders_init(renderer);
    wlr_egl_unset_current(r->egl);

    wm_renderer_select_texture_shaders(renderer, server->wm_config->texture_shaders);
#endif

}

void wm_renderer_destroy(struct wm_renderer *renderer) {
    wlr_renderer_destroy(renderer->wlr_renderer);
}

void wm_renderer_begin(struct wm_renderer *renderer, struct wm_output *output) {
    wlr_renderer_begin(renderer->wlr_renderer, output->wlr_output->width,
                       output->wlr_output->height);
    renderer->current = output;
}

void wm_renderer_end(struct wm_renderer *renderer, pixman_region32_t *damage,
                     struct wm_output *output) {
    wlr_renderer_scissor(renderer->wlr_renderer, NULL);
    wlr_output_render_software_cursors(output->wlr_output, damage);
    wlr_renderer_end(renderer->wlr_renderer);

    renderer->current = NULL;
}

void wm_renderer_render_texture_at(struct wm_renderer *renderer,
                                   pixman_region32_t *damage,
                                   struct wlr_texture *texture,
                                   struct wlr_box *box, double opacity,
                                   struct wlr_box *mask, double corner_radius,
                                   double lock_perc) {

    int ow, oh;
    wlr_output_transformed_resolution(renderer->current->wlr_output, &ow, &oh);

    enum wl_output_transform transform =
        wlr_output_transform_invert(renderer->current->wlr_output->transform);

    float matrix[9];
    wlr_matrix_project_box(matrix, box, WL_OUTPUT_TRANSFORM_NORMAL, 0,
                           renderer->current->wlr_output->transform_matrix);

    struct wlr_fbox fbox = {
        .x = 0,
        .y = 0,
        .width = texture->width,
        .height = texture->height,
    };

    int nrects;
    pixman_box32_t *rects = pixman_region32_rectangles(damage, &nrects);
    for (int i = 0; i < nrects; i++) {
        struct wlr_box damage_box = {.x = rects[i].x1,
                                     .y = rects[i].y1,
                                     .width = rects[i].x2 - rects[i].x1,
                                     .height = rects[i].y2 - rects[i].y1};
        struct wlr_box inters;
        wlr_box_intersection(&inters, box, &damage_box);
        if (wlr_box_empty(&inters))
            continue;

        wlr_box_transform(&inters, &inters, transform, ow, oh);
        wlr_renderer_scissor(renderer->wlr_renderer, &inters);

#ifdef WM_CUSTOM_RENDERER
        render_subtexture_with_matrix(
            renderer, texture, &fbox, matrix, opacity, box, mask->x - box->x,
            mask->y - box->y, box->x + box->width - mask->x - mask->width,
            box->y + box->height - mask->y - mask->height, corner_radius,
            lock_perc);
#else

        wlr_render_subtexture_with_matrix(renderer->wlr_renderer, texture,
                                          &fbox, matrix, opacity);
#endif
    }
}

void wm_renderer_render_primitive(struct wm_renderer* renderer,
                                  pixman_region32_t* damage,
                                  struct wlr_box* box,
                                  double opacity, int* params_int, float* params_float){

    int ow, oh;
    wlr_output_transformed_resolution(renderer->current->wlr_output, &ow, &oh);

    enum wl_output_transform transform =
        wlr_output_transform_invert(renderer->current->wlr_output->transform);

#ifdef WM_CUSTOM_RENDERER
    float matrix[9];
    wlr_matrix_project_box(matrix, box, WL_OUTPUT_TRANSFORM_NORMAL, 0,
                           renderer->current->wlr_output->transform_matrix);

    struct wlr_fbox fbox = {
        .x = 0,
        .y = 0,
        .width = box->width,
        .height = box->height,
    };
#endif

    int nrects;
    pixman_box32_t *rects = pixman_region32_rectangles(damage, &nrects);
    for (int i = 0; i < nrects; i++) {
        struct wlr_box damage_box = {.x = rects[i].x1,
                                     .y = rects[i].y1,
                                     .width = rects[i].x2 - rects[i].x1,
                                     .height = rects[i].y2 - rects[i].y1};
        struct wlr_box inters;
        wlr_box_intersection(&inters, box, &damage_box);
        if (wlr_box_empty(&inters))
            continue;

        wlr_box_transform(&inters, &inters, transform, ow, oh);
        wlr_renderer_scissor(renderer->wlr_renderer, &inters);

#ifdef WM_CUSTOM_RENDERER
        render_primitive_with_matrix(
            renderer, &fbox, matrix, opacity, params_int, params_float);
#else
        wlr_render_rect(renderer->wlr_renderer, box, (float[]){1.0, 1.0, 0.0, 1.0}, renderer->current->wlr_output->transform_matrix);
#endif
    }
}
