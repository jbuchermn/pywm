#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <stdlib.h>
#include <wayland-server.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_matrix.h>
#include <render/gles2.h>

#include "wm/wm_server.h"
#include "wm/wm_renderer.h"
#include "wm/wm_output.h"

#ifdef WM_CUSTOM_RENDERER
static const GLfloat verts[] = {
	1, 0, // top right
	0, 0, // top left
	1, 1, // bottom right
	0, 1, // bottom left
};

static const float flip_180[9] = {
	1.0f, 0.0f, 0.0f,
	0.0f, -1.0f, 0.0f,
	0.0f, 0.0f, 1.0f,
};

static GLuint compile_shader(struct wlr_gles2_renderer *renderer,
		GLuint type, const GLchar *src) {

	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);

	GLint ok;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
	if (ok == GL_FALSE) {
		glDeleteShader(shader);
		shader = 0;
	}

	return shader;
}

static GLuint link_program(struct wlr_gles2_renderer *renderer,
		const GLchar *vert_src, const GLchar *frag_src) {

	GLuint vert = compile_shader(renderer, GL_VERTEX_SHADER, vert_src);
	if (!vert) {
		goto error;
	}

	GLuint frag = compile_shader(renderer, GL_FRAGMENT_SHADER, frag_src);
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

	return prog;

error:
	return 0;
}

static bool render_subtexture_with_matrix(
		struct wm_renderer *renderer, struct wlr_texture *wlr_texture,
		const struct wlr_fbox *box, const float matrix[static 9],
		float alpha,
        const struct wlr_box *display_box, float corner_radius, bool blurred
        ) {
	struct wlr_gles2_renderer *gles2_renderer =
		gles2_get_renderer(renderer->wlr_renderer);
	struct wlr_gles2_texture *texture =
		gles2_get_texture(wlr_texture);
	assert(wlr_egl_is_current(gles2_renderer->egl));

	struct wm_renderer_shader *shader = NULL;

	switch (texture->target) {
	case GL_TEXTURE_2D:
		if (texture->has_alpha) {
			shader = blurred ? &renderer->shader_blurred_rgba : &renderer->shader_rgba;
		} else {
			shader = blurred ? &renderer->shader_blurred_rgbx : &renderer->shader_rgbx;
		}
		break;
	case GL_TEXTURE_EXTERNAL_OES:
        wlr_log(WLR_ERROR, "Failed to render texture: "
            "GL_TEXTURE_EXTERNAL_OES not supported");
        return false;
	default:
		abort();
	}

	float gl_matrix[9];
	if (gles2_renderer->current_buffer != NULL) {
		wlr_matrix_multiply(gl_matrix, flip_180, matrix);
	} else {
		memcpy(gl_matrix, matrix, sizeof(gl_matrix));
	}
	// OpenGL ES 2 requires the glUniformMatrix3fv transpose parameter to be set
	// to GL_FALSE
	wlr_matrix_transpose(gl_matrix, gl_matrix);


	glActiveTexture(GL_TEXTURE0);
	glBindTexture(texture->target, texture->tex);

	glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glUseProgram(shader->shader);

	glUniformMatrix3fv(shader->proj, 1, GL_FALSE, gl_matrix);
	glUniform1i(shader->invert_y, texture->inverted_y);
	glUniform1i(shader->tex, 0);
	glUniform1f(shader->alpha, alpha);
	glUniform1f(shader->width, display_box->width);
	glUniform1f(shader->height, display_box->height);
	glUniform1f(shader->cornerradius, corner_radius);

    if(blurred){
        float x = 2. / display_box->width;
        float y = 2. / display_box->height;
        glUniform2fv(shader->blur_offset, 25, (GLfloat[]){
                -2*x, -2*y, // 5
                -2*x, -1*y, // 4
                -2*x,  0*y, // 3
                -2*x,  1*y, // 4 
                -2*x,  2*y, // 5
                -1*x, -2*y, // 4
                -1*x, -1*y, // 3
                -1*x,  0*y, // 2
                -1*x,  1*y, // 3
                -1*x,  2*y, // 4
                 0*x, -2*y, // 3
                 0*x, -1*y, // 2
                 0*x,  0*y, // 1
                 0*x,  1*y, // 2
                 0*x,  2*y, // 3
                 1*x, -2*y, // 4
                 1*x, -1*y, // 3
                 1*x,  0*y, // 2
                 1*x,  1*y, // 3
                 1*x,  2*y, // 4
                 2*x, -2*y, // 5
                 2*x, -1*y, // 4
                 2*x,  0*y, // 3
                 2*x,  1*y, // 4
                 2*x,  2*y, // 5
                });
        glUniform1fv(shader->blur_weight, 25, (GLfloat[]){
                5. / 85.,
                4. / 85.,
                3. / 85.,
                4. / 85.,
                5. / 85.,
                4. / 85.,
                3. / 85.,
                2. / 85.,
                3. / 85.,
                4. / 85.,
                3. / 85.,
                2. / 85.,
                1. / 85.,
                2. / 85.,
                3. / 85.,
                4. / 85.,
                3. / 85.,
                2. / 85.,
                3. / 85.,
                4. / 85.,
                5. / 85.,
                4. / 85.,
                3. / 85.,
                4. / 85.,
                5. / 85.
                });
    }

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
	glVertexAttribPointer(shader->tex_attrib, 2, GL_FLOAT, GL_FALSE, 0, texcoord);

	glEnableVertexAttribArray(shader->pos_attrib);
	glEnableVertexAttribArray(shader->tex_attrib);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableVertexAttribArray(shader->pos_attrib);
	glDisableVertexAttribArray(shader->tex_attrib);

	glBindTexture(texture->target, 0);

	return true;
}

const GLchar custom_tex_vertex_src[] =
"uniform mat3 proj;\n"
"uniform bool invert_y;\n"
"attribute vec2 pos;\n"
"attribute vec2 texcoord;\n"
"varying vec2 v_texcoord;\n"
"\n"
"void main() {\n"
"	gl_Position = vec4(proj * vec3(pos, 1.0), 1.0);\n"
"	if (invert_y) {\n"
"		v_texcoord = vec2(texcoord.x, 1.0 - texcoord.y);\n"
"	} else {\n"
"		v_texcoord = texcoord;\n"
"	}\n"
"}\n";


const GLchar custom_tex_fragment_src_rgba[] =
"precision mediump float;\n"
"varying vec2 v_texcoord;\n"
"uniform sampler2D tex;\n"
"uniform float alpha;\n"
"\n"
"uniform float width;\n"
"uniform float height;\n"
"uniform float cornerradius;\n"
"\n"
"void main() {\n"
"   if(v_texcoord.x*width < cornerradius && v_texcoord.y*height < cornerradius){\n"
"       if(length(vec2(v_texcoord.x*width, v_texcoord.y*height) - vec2(cornerradius, cornerradius)) > cornerradius) discard;\n"
"   }\n"
"   if(v_texcoord.x*width > width - cornerradius && v_texcoord.y*height < cornerradius){\n"
"       if(length(vec2(v_texcoord.x*width, v_texcoord.y*height) - vec2(width - cornerradius, cornerradius)) > cornerradius) discard;\n"
"   }\n"
"   if(v_texcoord.x*width < cornerradius && v_texcoord.y*height > height - cornerradius){\n"
"       if(length(vec2(v_texcoord.x*width, v_texcoord.y*height) - vec2(cornerradius, height - cornerradius)) > cornerradius) discard;\n"
"   }\n"
"   if(v_texcoord.x*width > width - cornerradius && v_texcoord.y*height > height - cornerradius){\n"
"       if(length(vec2(v_texcoord.x*width, v_texcoord.y*height) - vec2(width - cornerradius, height - cornerradius)) > cornerradius) discard;\n"
"   }\n"
"	gl_FragColor = texture2D(tex, v_texcoord) * alpha;\n"
"}\n";



const GLchar custom_tex_fragment_src_rgbx[] =
"precision mediump float;\n"
"varying vec2 v_texcoord;\n"
"uniform sampler2D tex;\n"
"uniform float alpha;\n"
"\n"
"uniform float width;\n"
"uniform float height;\n"
"uniform float cornerradius;\n"
"\n"
"void main() {\n"
"   if(v_texcoord.x*width < cornerradius && v_texcoord.y*height < cornerradius){\n"
"       if(length(vec2(v_texcoord.x*width, v_texcoord.y*height) - vec2(cornerradius, cornerradius)) > cornerradius) discard;\n"
"   }\n"
"   if(v_texcoord.x*width > width - cornerradius && v_texcoord.y*height < cornerradius){\n"
"       if(length(vec2(v_texcoord.x*width, v_texcoord.y*height) - vec2(width - cornerradius, cornerradius)) > cornerradius) discard;\n"
"   }\n"
"   if(v_texcoord.x*width < cornerradius && v_texcoord.y*height > height - cornerradius){\n"
"       if(length(vec2(v_texcoord.x*width, v_texcoord.y*height) - vec2(cornerradius, height - cornerradius)) > cornerradius) discard;\n"
"   }\n"
"   if(v_texcoord.x*width > width - cornerradius && v_texcoord.y*height > height - cornerradius){\n"
"       if(length(vec2(v_texcoord.x*width, v_texcoord.y*height) - vec2(width - cornerradius, height - cornerradius)) > cornerradius) discard;\n"
"   }\n"
"	gl_FragColor = vec4(texture2D(tex, v_texcoord).rgb, 1.0) * alpha;\n"
"}\n";

const GLchar custom_tex_fragment_blurred_src_rgba[] =
"precision mediump float;\n"
"varying vec2 v_texcoord;\n"
"uniform sampler2D tex;\n"
"uniform float alpha;\n"
"\n"
"uniform float width;\n"
"uniform float height;\n"
"\n"
"uniform vec2 blur_offset[25];\n"
"uniform float blur_weight[25];\n"
"uniform float cornerradius;\n"
"\n"
"void main() {\n"
"   if(v_texcoord.x*width < cornerradius && v_texcoord.y*height < cornerradius){\n"
"       if(length(vec2(v_texcoord.x*width, v_texcoord.y*height) - vec2(cornerradius, cornerradius)) > cornerradius) discard;\n"
"   }\n"
"   if(v_texcoord.x*width > width - cornerradius && v_texcoord.y*height < cornerradius){\n"
"       if(length(vec2(v_texcoord.x*width, v_texcoord.y*height) - vec2(width - cornerradius, cornerradius)) > cornerradius) discard;\n"
"   }\n"
"   if(v_texcoord.x*width < cornerradius && v_texcoord.y*height > height - cornerradius){\n"
"       if(length(vec2(v_texcoord.x*width, v_texcoord.y*height) - vec2(cornerradius, height - cornerradius)) > cornerradius) discard;\n"
"   }\n"
"   if(v_texcoord.x*width > width - cornerradius && v_texcoord.y*height > height - cornerradius){\n"
"       if(length(vec2(v_texcoord.x*width, v_texcoord.y*height) - vec2(width - cornerradius, height - cornerradius)) > cornerradius) discard;\n"
"   }\n"
"   gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);\n"
"   for(int i=0; i<25; i++)\n"
"       gl_FragColor += texture2D(tex, v_texcoord + blur_offset[i]) * blur_weight[i] * alpha;\n"
"}\n";



const GLchar custom_tex_fragment_blurred_src_rgbx[] =
"precision mediump float;\n"
"varying vec2 v_texcoord;\n"
"uniform sampler2D tex;\n"
"uniform float alpha;\n"
"\n"
"uniform float width;\n"
"uniform float height;\n"
"\n"
"uniform vec2 blur_offset[25];\n"
"uniform float blur_weight[25];\n"
"uniform float cornerradius;\n"
"\n"
"void main() {\n"
"   if(v_texcoord.x*width < cornerradius && v_texcoord.y*height < cornerradius){\n"
"       if(length(vec2(v_texcoord.x*width, v_texcoord.y*height) - vec2(cornerradius, cornerradius)) > cornerradius) discard;\n"
"   }\n"
"   if(v_texcoord.x*width > width - cornerradius && v_texcoord.y*height < cornerradius){\n"
"       if(length(vec2(v_texcoord.x*width, v_texcoord.y*height) - vec2(width - cornerradius, cornerradius)) > cornerradius) discard;\n"
"   }\n"
"   if(v_texcoord.x*width < cornerradius && v_texcoord.y*height > height - cornerradius){\n"
"       if(length(vec2(v_texcoord.x*width, v_texcoord.y*height) - vec2(cornerradius, height - cornerradius)) > cornerradius) discard;\n"
"   }\n"
"   if(v_texcoord.x*width > width - cornerradius && v_texcoord.y*height > height - cornerradius){\n"
"       if(length(vec2(v_texcoord.x*width, v_texcoord.y*height) - vec2(width - cornerradius, height - cornerradius)) > cornerradius) discard;\n"
"   }\n"
"   gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);\n"
"   for(int i=0; i<25; i++)\n"
"       gl_FragColor += vec4(texture2D(tex, v_texcoord + blur_offset[i]).rgb, 1.0) * blur_weight[i] * alpha;\n"
"}\n";


#endif

void wm_renderer_init(struct wm_renderer* renderer, struct wm_server* server){
    renderer->wm_server = server;

    renderer->wlr_renderer = wlr_backend_get_renderer(server->wlr_backend);
    assert(renderer->wlr_renderer);

    wlr_renderer_init_wl_display(renderer->wlr_renderer, server->wl_display);

    renderer->current = NULL;

#ifdef WM_CUSTOM_RENDERER
	struct wlr_gles2_renderer *r =
		gles2_get_renderer(renderer->wlr_renderer);

	assert(wlr_egl_make_current(r->egl));

	renderer->shader_rgba.shader = 
		link_program(r, custom_tex_vertex_src, custom_tex_fragment_src_rgba);
	assert(renderer->shader_rgba.shader);

	renderer->shader_rgba.proj = glGetUniformLocation(renderer->shader_rgba.shader, "proj");
	renderer->shader_rgba.invert_y = glGetUniformLocation(renderer->shader_rgba.shader, "invert_y");
	renderer->shader_rgba.tex = glGetUniformLocation(renderer->shader_rgba.shader, "tex");
	renderer->shader_rgba.alpha = glGetUniformLocation(renderer->shader_rgba.shader, "alpha");
	renderer->shader_rgba.width = glGetUniformLocation(renderer->shader_rgba.shader, "width");
	renderer->shader_rgba.height = glGetUniformLocation(renderer->shader_rgba.shader, "height");
	renderer->shader_rgba.cornerradius = glGetUniformLocation(renderer->shader_rgba.shader, "cornerradius");

	renderer->shader_rgba.pos_attrib = glGetAttribLocation(renderer->shader_rgba.shader, "pos");
	renderer->shader_rgba.tex_attrib = glGetAttribLocation(renderer->shader_rgba.shader, "texcoord");

	renderer->shader_rgbx.shader = 
		link_program(r, custom_tex_vertex_src, custom_tex_fragment_src_rgbx);
	assert(renderer->shader_rgbx.shader);

	renderer->shader_rgbx.proj = glGetUniformLocation(renderer->shader_blurred_rgbx.shader, "proj");
	renderer->shader_rgbx.invert_y = glGetUniformLocation(renderer->shader_blurred_rgbx.shader, "invert_y");
	renderer->shader_rgbx.tex = glGetUniformLocation(renderer->shader_blurred_rgbx.shader, "tex");
	renderer->shader_rgbx.alpha = glGetUniformLocation(renderer->shader_blurred_rgbx.shader, "alpha");
	renderer->shader_rgbx.width = glGetUniformLocation(renderer->shader_blurred_rgbx.shader, "width");
	renderer->shader_rgbx.height = glGetUniformLocation(renderer->shader_blurred_rgbx.shader, "height");
	renderer->shader_rgbx.cornerradius = glGetUniformLocation(renderer->shader_blurred_rgbx.shader, "cornerradius");

	renderer->shader_rgbx.pos_attrib = glGetAttribLocation(renderer->shader_blurred_rgbx.shader, "pos");
	renderer->shader_rgbx.tex_attrib = glGetAttribLocation(renderer->shader_blurred_rgbx.shader, "texcoord");

	renderer->shader_blurred_rgba.shader =
		link_program(r, custom_tex_vertex_src, custom_tex_fragment_blurred_src_rgba);
	assert(renderer->shader_blurred_rgba.shader);

	renderer->shader_blurred_rgba.proj = glGetUniformLocation(renderer->shader_blurred_rgba.shader, "proj");
	renderer->shader_blurred_rgba.invert_y = glGetUniformLocation(renderer->shader_blurred_rgba.shader, "invert_y");
	renderer->shader_blurred_rgba.tex = glGetUniformLocation(renderer->shader_blurred_rgba.shader, "tex");
	renderer->shader_blurred_rgba.alpha = glGetUniformLocation(renderer->shader_blurred_rgba.shader, "alpha");
	renderer->shader_blurred_rgba.width = glGetUniformLocation(renderer->shader_blurred_rgba.shader, "width");
	renderer->shader_blurred_rgba.height = glGetUniformLocation(renderer->shader_blurred_rgba.shader, "height");

	renderer->shader_blurred_rgba.cornerradius = glGetUniformLocation(renderer->shader_blurred_rgba.shader, "cornerradius");
	renderer->shader_blurred_rgba.blur_offset = glGetUniformLocation(renderer->shader_blurred_rgba.shader, "blur_offset");
	renderer->shader_blurred_rgba.blur_weight = glGetUniformLocation(renderer->shader_blurred_rgba.shader, "blur_weight");

	renderer->shader_blurred_rgba.pos_attrib = glGetAttribLocation(renderer->shader_blurred_rgba.shader, "pos");
	renderer->shader_blurred_rgba.tex_attrib = glGetAttribLocation(renderer->shader_blurred_rgba.shader, "texcoord");

	renderer->shader_blurred_rgbx.shader =
		link_program(r, custom_tex_vertex_src, custom_tex_fragment_blurred_src_rgbx);
	assert(renderer->shader_blurred_rgbx.shader);

	renderer->shader_blurred_rgbx.proj = glGetUniformLocation(renderer->shader_rgbx.shader, "proj");
	renderer->shader_blurred_rgbx.invert_y = glGetUniformLocation(renderer->shader_rgbx.shader, "invert_y");
	renderer->shader_blurred_rgbx.tex = glGetUniformLocation(renderer->shader_rgbx.shader, "tex");
	renderer->shader_blurred_rgbx.alpha = glGetUniformLocation(renderer->shader_rgbx.shader, "alpha");
	renderer->shader_blurred_rgbx.width = glGetUniformLocation(renderer->shader_rgbx.shader, "width");
	renderer->shader_blurred_rgbx.height = glGetUniformLocation(renderer->shader_rgbx.shader, "height");

	renderer->shader_blurred_rgbx.cornerradius = glGetUniformLocation(renderer->shader_rgbx.shader, "cornerradius");
	renderer->shader_blurred_rgbx.blur_offset = glGetUniformLocation(renderer->shader_blurred_rgba.shader, "blur_offset");
	renderer->shader_blurred_rgbx.blur_weight = glGetUniformLocation(renderer->shader_blurred_rgba.shader, "blur_weight");

	renderer->shader_blurred_rgbx.pos_attrib = glGetAttribLocation(renderer->shader_rgbx.shader, "pos");
	renderer->shader_blurred_rgbx.tex_attrib = glGetAttribLocation(renderer->shader_rgbx.shader, "texcoord");

	wlr_egl_unset_current(r->egl);
#endif
}

void wm_renderer_destroy(struct wm_renderer* renderer){
    wlr_renderer_destroy(renderer->wlr_renderer);
}

void wm_renderer_begin(struct wm_renderer* renderer, struct wm_output* output){
	wlr_renderer_begin(renderer->wlr_renderer, output->wlr_output->width, output->wlr_output->height);
    renderer->current = output;
}

void wm_renderer_end(struct wm_renderer* renderer, pixman_region32_t* damage, struct wm_output* output){
    wlr_renderer_scissor(renderer->wlr_renderer, NULL);
    wlr_output_render_software_cursors(output->wlr_output, damage);
	wlr_renderer_end(renderer->wlr_renderer);

    renderer->current = NULL;
}

void wm_renderer_render_texture_at(struct wm_renderer *renderer,
                                   pixman_region32_t *damage,
                                   struct wlr_texture *texture,
                                   struct wlr_box *box, double opacity,
                                   double corner_radius, bool blurred) {

    float matrix[9];
    wlr_matrix_project_box(matrix, box,
            WL_OUTPUT_TRANSFORM_NORMAL, 0,
            renderer->current->wlr_output->transform_matrix);

	struct wlr_fbox fbox = {
		.x = 0,
		.y = 0,
		.width = texture->width,
		.height = texture->height,
	};

	int nrects;
	pixman_box32_t* rects = pixman_region32_rectangles(damage, &nrects);
	for(int i=0; i<nrects; i++){
        struct wlr_box damage_box = {
            .x = rects[i].x1,
            .y = rects[i].y1,
            .width = rects[i].x2 - rects[i].x1,
            .height = rects[i].y2 - rects[i].y1
        };
		struct wlr_box inters;
		wlr_box_intersection(&inters, box, &damage_box);
		if(wlr_box_empty(&inters)) continue;

        wlr_renderer_scissor(renderer->wlr_renderer, &inters);

#ifdef WM_CUSTOM_RENDERER
		render_subtexture_with_matrix(
				renderer,
				texture,
				&fbox, matrix, opacity,

				box, corner_radius, blurred);
#else
		wlr_render_subtexture_with_matrix(
				renderer->wlr_renderer,
				texture,
				&fbox, matrix, 1.);
#endif
	}
}
