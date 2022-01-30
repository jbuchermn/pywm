precision mediump float;
varying vec2 v_texcoord;
uniform sampler2D tex;
uniform vec2 halfpixel;
uniform float offset;

void main() {
    vec4 sum = texture2D(tex, v_texcoord) * 4.0;
    sum += texture2D(tex, v_texcoord - halfpixel.xy * offset);
    sum += texture2D(tex, v_texcoord + halfpixel.xy * offset);
    sum += texture2D(tex, v_texcoord + vec2(halfpixel.x, -halfpixel.y) * offset);
    sum += texture2D(tex, v_texcoord - vec2(halfpixel.x, -halfpixel.y) * offset);
    gl_FragColor = sum / 8.;
}
