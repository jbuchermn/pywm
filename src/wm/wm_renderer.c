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
    shader->offset_x = glGetUniformLocation(shader->shader, "offset_x");
    shader->offset_y = glGetUniformLocation(shader->shader, "offset_y");
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

void wm_renderer_init_texture_shaders(struct wm_renderer* renderer, int n_shaders){
    renderer->texture_shaders = calloc(n_shaders, sizeof(struct wm_renderer_texture_shaders));
    renderer->n_texture_shaders = n_shaders;

    const GLchar quad_vertex_src[] =
    "attribute vec2 pos;\n"
    "attribute vec2 texcoord;\n"
    "varying vec2 v_texcoord;\n"
    "\n"
    "void main() {\n"
    "	gl_Position = vec4(pos, 1.0, 1.0);\n"
    "	v_texcoord = texcoord;\n"
    "}\n";

    const GLchar quad_fragment_src[] =
    "precision mediump float;\n"
    "varying vec2 v_texcoord;\n"
    "uniform sampler2D tex;\n"
    "uniform float alpha;\n"
    "\n"
    "void main() {\n"
    "	gl_FragColor = texture2D(tex, v_texcoord) * alpha;\n"
    "}\n";

    wm_renderer_link_texture_shader(renderer, &renderer->quad_shader, quad_vertex_src, quad_fragment_src);

    const GLchar downsample_fragment_src[] =
    "precision mediump float;\n"
    "varying vec2 v_texcoord;\n"
    "uniform sampler2D tex;\n"
    "uniform float width;\n"
    "uniform float height;\n"
    "uniform float alpha;\n"
    "\n"
    "void main() {\n"
    "   float offset = alpha;\n"
    "   vec2 halfpixel = vec2(width, height);\n"
    "   vec4 sum = texture2D(tex, v_texcoord) * 4.0;\n"
    "   sum += texture2D(tex, v_texcoord - halfpixel.xy * offset);\n"
    "   sum += texture2D(tex, v_texcoord + halfpixel.xy * offset);\n"
    "   sum += texture2D(tex, v_texcoord + vec2(halfpixel.x, -halfpixel.y) * offset);\n"
    "   sum += texture2D(tex, v_texcoord - vec2(halfpixel.x, -halfpixel.y) * offset);\n"
    "	gl_FragColor = sum / 8.;\n"
    "}\n";

    wm_renderer_link_texture_shader(renderer, &renderer->downsample_shader, quad_vertex_src, downsample_fragment_src);

    const GLchar upsample_fragment_src[] =
    "precision mediump float;\n"
    "varying vec2 v_texcoord;\n"
    "uniform sampler2D tex;\n"
    "uniform float width;\n"
    "uniform float height;\n"
    "uniform float alpha;\n"
    "\n"
    "void main() {\n"
    "   float offset = alpha;\n"
    "   vec2 halfpixel = vec2(width, height);\n"
    "   vec4 sum = texture2D(tex, v_texcoord + vec2(-halfpixel.x * 2.0, 0.0) * offset);\n"
    "   sum += texture2D(tex, v_texcoord + vec2(-halfpixel.x, halfpixel.y) * offset) * 2.0;\n"
    "   sum += texture2D(tex, v_texcoord + vec2(0.0, halfpixel.y * 2.0) * offset);\n"
    "   sum += texture2D(tex, v_texcoord + vec2(halfpixel.x, halfpixel.y) * offset) * 2.0;\n"
    "   sum += texture2D(tex, v_texcoord + vec2(halfpixel.x * 2.0, 0.0) * offset);\n"
    "   sum += texture2D(tex, v_texcoord + vec2(halfpixel.x, -halfpixel.y) * offset) * 2.0;\n"
    "   sum += texture2D(tex, v_texcoord + vec2(0.0, -halfpixel.y * 2.0) * offset);\n"
    "   sum += texture2D(tex, v_texcoord + vec2(-halfpixel.x, -halfpixel.y) * offset) * 2.0;\n"
    "	gl_FragColor = sum / 12.;\n"
    "}\n";

    wm_renderer_link_texture_shader(renderer, &renderer->upsample_shader, quad_vertex_src, upsample_fragment_src);
}

void wm_renderer_add_texture_shaders(
    struct wm_renderer *renderer, const char *name, const GLchar *vert_src,
    const GLchar *frag_src_rgba, const GLchar *frag_src_rgbx,
    const GLchar *frag_src_ext, const GLchar *frag_src_lock_rgba,
    const GLchar *frag_src_lock_rgbx, const GLchar *frag_src_lock_ext) {

    struct wlr_gles2_renderer *gles2_renderer =
        gles2_get_renderer(renderer->wlr_renderer);

    int i = 0;
    for (; i < renderer->n_texture_shaders; i++) {
        if (!renderer->texture_shaders[i].name)
            break;
    }
    assert(i < renderer->n_texture_shaders);

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

void wm_renderer_init_primitive_shaders(struct wm_renderer* renderer, int n_shaders){
    renderer->primitive_shaders = calloc(n_shaders, sizeof(struct wm_renderer_primitive_shader));
    renderer->n_primitive_shaders = n_shaders;
}

void wm_renderer_add_primitive_shader(struct wm_renderer *renderer,
                                      const char *name, const GLchar *vert_src,
                                      const GLchar *frag_src, int n_params_int, int n_params_float) {

    int i = 0;
    for (; i < renderer->n_primitive_shaders; i++) {
        if (!renderer->primitive_shaders[i].name)
            break;
    }
    assert(i < renderer->n_primitive_shaders);

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

    if(n_params_float){
        renderer->primitive_shaders[i].params_float = glGetUniformLocation(renderer->primitive_shaders[i].shader, "params_float");
    }
    if(n_params_int){
        renderer->primitive_shaders[i].params_int = glGetUniformLocation(renderer->primitive_shaders[i].shader, "params_int");
    }

}

#endif

void wm_renderer_select_texture_shaders(struct wm_renderer *renderer,
                                        const char *name) {
#ifdef WM_CUSTOM_RENDERER
    int i = 0;
    for (; i < renderer->n_texture_shaders; i++) {
        if (renderer->texture_shaders[i].name && !strcmp(renderer->texture_shaders[i].name, name))
            break;
    }

    if (i < renderer->n_texture_shaders){
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
    for (; i < renderer->n_primitive_shaders; i++) {
        if (renderer->primitive_shaders[i].name && !strcmp(renderer->primitive_shaders[i].name, name))
            break;
    }
    if (i < renderer->n_primitive_shaders){
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
    glUniform1f(shader->offset_x, display_box->x);
    glUniform1f(shader->offset_y, display_box->y);
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

    glBindFramebuffer(GL_FRAMEBUFFER, renderer->current->renderer_buffers->frame_buffer);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

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

    renderer->n_primitive_shaders = 0;
    renderer->n_texture_shaders = 0;
    renderer->texture_shaders_selected = NULL;
    renderer->primitive_shader_selected = NULL;

    assert(wlr_egl_make_current(r->egl));
    wm_texture_shaders_init(renderer);
    wm_primitive_shaders_init(renderer);

    wm_renderer_select_texture_shaders(renderer, server->wm_config->texture_shaders);
    renderer->primitive_shader_selected = renderer->primitive_shaders;

    wlr_egl_unset_current(r->egl);
#endif

}

#ifdef WM_CUSTOM_RENDERER
void wm_renderer_buffers_init(struct wm_renderer_buffers* buffers, struct wm_renderer* renderer, int width, int height){
    buffers->width = width;
    buffers->height = height;
    buffers->parent = renderer;
    wlr_log(WLR_DEBUG, "Initialising renderer buffers for output: %dx%d", width, height);

    struct wlr_gles2_renderer *gles2_renderer = gles2_get_renderer(renderer->wlr_renderer);
    assert(wlr_egl_make_current(gles2_renderer->egl));

    glGenFramebuffers(1, &buffers->frame_buffer);
    glGenTextures(1, &buffers->frame_buffer_tex);

    glBindTexture(GL_TEXTURE_2D, buffers->frame_buffer_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, buffers->frame_buffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffers->frame_buffer_tex, 0);

    glGenRenderbuffers(1, &buffers->frame_buffer_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, buffers->frame_buffer_rbo); 
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);  
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, buffers->frame_buffer_rbo);

    wlr_log(WLR_ERROR, "%d", glCheckFramebufferStatus(GL_FRAMEBUFFER));
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    int ds_width = width;
    int ds_height = height;
    for(int i=0; i<WM_RENDERER_DOWNSAMPLE_BUFFERS; i++){
        ds_width /= 2;
        ds_height /= 2;
        buffers->downsample_buffers_width[i] = ds_width;
        buffers->downsample_buffers_height[i] = ds_height;
        glGenFramebuffers(1, &buffers->downsample_buffers[i]);
        glGenTextures(1, &buffers->downsample_buffers_tex[i]);

        glBindTexture(GL_TEXTURE_2D, buffers->downsample_buffers_tex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ds_width, ds_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, buffers->downsample_buffers[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffers->downsample_buffers_tex[i], 0);

        glGenRenderbuffers(1, &buffers->downsample_buffers_rbo[i]);
        glBindRenderbuffer(GL_RENDERBUFFER, buffers->downsample_buffers_rbo[i]); 
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, ds_width, ds_height);  
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, buffers->downsample_buffers_rbo[i]);

        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    wlr_egl_unset_current(gles2_renderer->egl);
}

void wm_renderer_buffers_destroy(struct wm_renderer_buffers* buffers){
    if(!buffers) return;

    wlr_log(WLR_DEBUG, "Freeing renderer buffers for output: %dx%d", buffers->width, buffers->height);

    struct wlr_gles2_renderer *r = gles2_get_renderer(buffers->parent->wlr_renderer);
    assert(r);
    assert(wlr_egl_make_current(r->egl));

    glDeleteFramebuffers(1, &buffers->frame_buffer);
    glDeleteRenderbuffers(1, &buffers->frame_buffer_rbo);
    glDeleteTextures(1, &buffers->frame_buffer_tex);

    for(int i=0; i<WM_RENDERER_DOWNSAMPLE_BUFFERS; i++){
        glDeleteFramebuffers(1, &buffers->downsample_buffers[i]);
        glDeleteRenderbuffers(1, &buffers->downsample_buffers[i]);
        glDeleteTextures(1, &buffers->downsample_buffers_tex[i]);
    }

    wlr_egl_unset_current(r->egl);
}

void wm_renderer_buffers_ensure(struct wm_renderer* renderer, struct wm_output* output){
    if(output->renderer_buffers && output->renderer_buffers->width == output->wlr_output->width && output->renderer_buffers->height == output->wlr_output->height){
        return;
    }

    if(!output->renderer_buffers){
        output->renderer_buffers = calloc(1, sizeof(struct wm_renderer_buffers));
    }else{
        wm_renderer_buffers_destroy(output->renderer_buffers);
    }
    wm_renderer_buffers_init(output->renderer_buffers, renderer, output->wlr_output->width, output->wlr_output->height);
}

#endif

int wm_renderer_init_output(struct wm_renderer* renderer, struct wm_output* output){
    return wlr_output_init_render(output->wlr_output, renderer->wm_server->wlr_allocator,
            renderer->wlr_renderer);
}

void wm_renderer_destroy(struct wm_renderer *renderer) {
    wlr_renderer_destroy(renderer->wlr_renderer);
}

static void wm_renderer_scissor(struct wm_renderer* renderer, struct wlr_box* box){
    struct wlr_gles2_renderer *gles2_renderer =
        gles2_get_renderer(renderer->wlr_renderer);

    push_gles2_debug(gles2_renderer);
    if (box != NULL) {
        glScissor(box->x, box->y, box->width, box->height);
        glEnable(GL_SCISSOR_TEST);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }
    pop_gles2_debug(gles2_renderer);
}

void wm_renderer_begin(struct wm_renderer *renderer, struct wm_output *output) {
    renderer->current = output;
#ifdef WM_CUSTOM_RENDERER
    wm_renderer_buffers_ensure(renderer, output);
    struct wlr_gles2_renderer *gles2_renderer = gles2_get_renderer(renderer->wlr_renderer);
    assert(wlr_egl_make_current(gles2_renderer->egl));
#else
    wlr_renderer_begin(renderer->wlr_renderer, output->wlr_output->width, output->wlr_output->height);
#endif
}

void wm_renderer_end(struct wm_renderer *renderer, pixman_region32_t *damage,
                     struct wm_output *output) {

    // DEBUG
    struct wlr_box debug_box = {
        .x = 0,
        .y = 0,
        .width = 200,
        .height = 200
    };
    wm_renderer_apply_blur(renderer, damage, &debug_box);

#ifdef WM_CUSTOM_RENDERER
    struct wlr_gles2_renderer *gles2_renderer = gles2_get_renderer(renderer->wlr_renderer);
    push_gles2_debug(gles2_renderer);

    wlr_renderer_begin(renderer->wlr_renderer, output->wlr_output->width, output->wlr_output->height);
    glBindFramebuffer(GL_FRAMEBUFFER, gles2_renderer->current_buffer->fbo);

    struct wm_renderer_texture_shader* shader = &renderer->quad_shader;

    const GLfloat texcoord[] = {
        1, 0, // top right
        0, 0, // top left
        1, 1, // bottom right
        0, 1, // bottom left
    };

    const GLfloat verts[] = {
        1, -1, // top right
        -1, -1, // top left
        1, 1, // bottom right
        -1, 1, // bottom left
    };

    /* TODO For each damage --> wlr_scissor */
    wlr_renderer_scissor(renderer->wlr_renderer, NULL);

    glUseProgram(shader->shader);

    glDisable(GL_BLEND);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->current->renderer_buffers->frame_buffer_tex);
    glUniform1i(shader->tex, 0);

    glUniform1f(shader->alpha, 1.);

    glVertexAttribPointer(shader->pos_attrib, 2, GL_FLOAT, GL_FALSE, 0, verts);
    glVertexAttribPointer(shader->tex_attrib, 2, GL_FLOAT, GL_FALSE, 0, texcoord);

    glEnableVertexAttribArray(shader->pos_attrib);
    glEnableVertexAttribArray(shader->tex_attrib);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(shader->pos_attrib);
    glDisableVertexAttribArray(shader->tex_attrib);

    glBindTexture(GL_TEXTURE_2D, 0);

    wlr_renderer_scissor(renderer->wlr_renderer, NULL);
    wlr_output_render_software_cursors(output->wlr_output, damage);
    wlr_renderer_end(renderer->wlr_renderer);

    pop_gles2_debug(gles2_renderer);

#else
    wlr_renderer_scissor(renderer->wlr_renderer, NULL);
    wlr_output_render_software_cursors(output->wlr_output, damage);
    wlr_renderer_end(renderer->wlr_renderer);
#endif

    renderer->current = NULL;
}

void wm_renderer_render_texture_at(struct wm_renderer *renderer,
                                   pixman_region32_t *damage,
                                   struct wlr_surface* surface,
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

    struct wlr_fbox fbox;
    if(surface){
        wlr_surface_get_buffer_source_box(surface, &fbox);
    }else{
        fbox.x = 0;
        fbox.y = 0;
        fbox.width = texture->width;
        fbox.height = texture->height;
    }

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
        wm_renderer_scissor(renderer, &inters);

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
        wm_renderer_scissor(renderer, &inters);

#ifdef WM_CUSTOM_RENDERER
        render_primitive_with_matrix(
            renderer, &fbox, matrix, opacity, params_int, params_float);
#else
        wlr_render_rect(renderer->wlr_renderer, box, (float[]){1.0, 1.0, 0.0, 1.0}, renderer->current->wlr_output->transform_matrix);
#endif
    }
}

void wm_renderer_apply_blur(struct wm_renderer* renderer, pixman_region32_t* damage, struct wlr_box* box){
#ifdef WM_CUSTOM_RENDERER
    struct wlr_gles2_renderer *gles2_renderer = gles2_get_renderer(renderer->wlr_renderer);
    push_gles2_debug(gles2_renderer);

    wm_renderer_scissor(renderer, box);

    int offset = 1;

    /*
     * Downsample
     */
    {
        const GLfloat texcoord[] = {
            1, 0, // top right
            0, 0, // top left
            1, 1, // bottom right
            0, 1, // bottom left
        };

        const GLfloat verts[] = {
            0, -1, // top right
            -1, -1, // top left
            0, 0, // bottom right
            -1, 0, // bottom left
        };

        glBindFramebuffer(GL_FRAMEBUFFER, renderer->current->renderer_buffers->downsample_buffers[0]);
        struct wm_renderer_texture_shader* shader = &renderer->downsample_shader;

        glUseProgram(shader->shader);

        glDisable(GL_BLEND);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, renderer->current->renderer_buffers->frame_buffer_tex);
        glUniform1i(shader->tex, 0);

        // Misuse attributes
        glUniform1f(shader->width, 0.5 / renderer->current->renderer_buffers->width);
        glUniform1f(shader->height, 0.5 / renderer->current->renderer_buffers->height);
        glUniform1f(shader->alpha, offset);

        glVertexAttribPointer(shader->pos_attrib, 2, GL_FLOAT, GL_FALSE, 0, verts);
        glVertexAttribPointer(shader->tex_attrib, 2, GL_FLOAT, GL_FALSE, 0, texcoord);

        glEnableVertexAttribArray(shader->pos_attrib);
        glEnableVertexAttribArray(shader->tex_attrib);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glDisableVertexAttribArray(shader->pos_attrib);
        glDisableVertexAttribArray(shader->tex_attrib);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    {
        const GLfloat texcoord[] = {
            1, 0, // top right
            0, 0, // top left
            1, 1, // bottom right
            0, 1, // bottom left
        };

        const GLfloat verts[] = {
            -0.75, -1, // top right
            -1, -1, // top left
            -0.75, -0.75, // bottom right
            -1, -0.75, // bottom left
        };
        glBindFramebuffer(GL_FRAMEBUFFER, renderer->current->renderer_buffers->downsample_buffers[1]);
        struct wm_renderer_texture_shader* shader = &renderer->downsample_shader;

        glUseProgram(shader->shader);

        glDisable(GL_BLEND);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, renderer->current->renderer_buffers->downsample_buffers_tex[0]);
        glUniform1i(shader->tex, 0);

        // Misuse attributes
        glUniform1f(shader->width, 0.5 / renderer->current->renderer_buffers->downsample_buffers_width[0]);
        glUniform1f(shader->height, 0.5 / renderer->current->renderer_buffers->downsample_buffers_height[0]);
        glUniform1f(shader->alpha, offset);

        glVertexAttribPointer(shader->pos_attrib, 2, GL_FLOAT, GL_FALSE, 0, verts);
        glVertexAttribPointer(shader->tex_attrib, 2, GL_FLOAT, GL_FALSE, 0, texcoord);

        glEnableVertexAttribArray(shader->pos_attrib);
        glEnableVertexAttribArray(shader->tex_attrib);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glDisableVertexAttribArray(shader->pos_attrib);
        glDisableVertexAttribArray(shader->tex_attrib);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    /*
     * Upsample
     */
    {
        const GLfloat texcoord[] = {
            1, 0, // top right
            0, 0, // top left
            1, 1, // bottom right
            0, 1, // bottom left
        };

        const GLfloat verts[] = {
            1, -1, // top right
            -1, -1, // top left
            1, 1, // bottom right
            -1, 1, // bottom left
        };

        glBindFramebuffer(GL_FRAMEBUFFER, renderer->current->renderer_buffers->downsample_buffers[0]);
        struct wm_renderer_texture_shader* shader = &renderer->upsample_shader;

        glUseProgram(shader->shader);

        glDisable(GL_BLEND);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, renderer->current->renderer_buffers->downsample_buffers_tex[1]);
        glUniform1i(shader->tex, 0);

        glVertexAttribPointer(shader->pos_attrib, 2, GL_FLOAT, GL_FALSE, 0, verts);
        glVertexAttribPointer(shader->tex_attrib, 2, GL_FLOAT, GL_FALSE, 0, texcoord);

        // Misuse attributes
        glUniform1f(shader->width, 0.5 / renderer->current->renderer_buffers->downsample_buffers_width[1]);
        glUniform1f(shader->height, 0.5 / renderer->current->renderer_buffers->downsample_buffers_height[1]);
        glUniform1f(shader->alpha, offset);

        glEnableVertexAttribArray(shader->pos_attrib);
        glEnableVertexAttribArray(shader->tex_attrib);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glDisableVertexAttribArray(shader->pos_attrib);
        glDisableVertexAttribArray(shader->tex_attrib);

        glBindTexture(GL_TEXTURE_2D, 0);
    }
    {
        const GLfloat texcoord[] = {
            1, 0, // top right
            0, 0, // top left
            1, 1, // bottom right
            0, 1, // bottom left
        };

        const GLfloat verts[] = {
            1, -1, // top right
            -1, -1, // top left
            1, 1, // bottom right
            -1, 1, // bottom left
        };

        glBindFramebuffer(GL_FRAMEBUFFER, renderer->current->renderer_buffers->frame_buffer);
        struct wm_renderer_texture_shader* shader = &renderer->upsample_shader;

        glUseProgram(shader->shader);

        glDisable(GL_BLEND);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, renderer->current->renderer_buffers->downsample_buffers_tex[0]);
        glUniform1i(shader->tex, 0);

        glVertexAttribPointer(shader->pos_attrib, 2, GL_FLOAT, GL_FALSE, 0, verts);
        glVertexAttribPointer(shader->tex_attrib, 2, GL_FLOAT, GL_FALSE, 0, texcoord);

        // Misuse attributes
        glUniform1f(shader->width, 0.5 / renderer->current->renderer_buffers->downsample_buffers_width[0]);
        glUniform1f(shader->height, 0.5 / renderer->current->renderer_buffers->downsample_buffers_height[0]);
        glUniform1f(shader->alpha, offset);

        glEnableVertexAttribArray(shader->pos_attrib);
        glEnableVertexAttribArray(shader->tex_attrib);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glDisableVertexAttribArray(shader->pos_attrib);
        glDisableVertexAttribArray(shader->tex_attrib);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    wm_renderer_scissor(renderer, NULL);

    pop_gles2_debug(gles2_renderer);
#else
    // noop
#endif
}
