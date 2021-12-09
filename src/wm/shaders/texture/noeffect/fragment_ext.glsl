#extension GL_OES_EGL_image_external : require
precision mediump float;

varying vec2 v_texcoord;
uniform samplerExternalOES texture0;
uniform float alpha;

uniform float width;
uniform float height;

uniform float padding_l;
uniform float padding_t;
uniform float padding_r;
uniform float padding_b;
uniform float cornerradius;
uniform float lock_perc;

void main() {
    gl_FragColor = texture2D(texture0, v_texcoord) * alpha;
};
