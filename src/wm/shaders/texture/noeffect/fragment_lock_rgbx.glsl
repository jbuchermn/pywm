precision mediump float;

varying vec2 v_texcoord;
uniform sampler2D tex;
uniform float alpha;

uniform float offset_x;
uniform float offset_y;
uniform float width;
uniform float height;

uniform float padding_l;
uniform float padding_t;
uniform float padding_r;
uniform float padding_b;
uniform float cornerradius;
uniform float lock_perc;

void main() {
    gl_FragColor = vec4(texture2D(tex, v_texcoord).rgb, 1.0) * alpha;
};
