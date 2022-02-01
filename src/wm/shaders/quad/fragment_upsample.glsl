precision mediump float;
varying vec2 v_texcoord;
uniform sampler2D tex;

uniform float width;
uniform float height;
uniform float padding_l;
uniform float padding_t;
uniform float padding_r;
uniform float padding_b;
uniform float cornerradius;

uniform vec2 halfpixel;
uniform float offset;

void main() {
    float x = gl_FragCoord.x;
    float y = gl_FragCoord.y;

    if(x < padding_l) discard;
    if(y < padding_t) discard;
    if(x > width - padding_r) discard;
    if(y > height - padding_b) discard;
    if(x < cornerradius + padding_l && y < cornerradius + padding_t){
        if(length(vec2(x, y) -
                vec2(cornerradius + padding_l, cornerradius + padding_t)) > cornerradius)
            discard;
    }
    if(x > width - cornerradius - padding_r && y < cornerradius + padding_t){
        if(length(vec2(x, y) -
                vec2(width - cornerradius - padding_r, cornerradius + padding_t)) > cornerradius)
            discard;
    }
    if(x < cornerradius + padding_l && y > height - cornerradius - padding_b){
        if(length(vec2(x, y) -
                vec2(cornerradius + padding_l, height - cornerradius - padding_b)) > cornerradius)
            discard;
    }
    if(x > width - cornerradius - padding_r && y > height - cornerradius - padding_b){
        if(length(vec2(x, y) - 
                    vec2(width - cornerradius - padding_r, height - cornerradius - padding_b)) > cornerradius)
            discard;
    }

    vec4 sum = texture2D(tex, v_texcoord + vec2(-halfpixel.x * 2.0, 0.0) * offset);
    sum += texture2D(tex, v_texcoord + vec2(-halfpixel.x, halfpixel.y) * offset) * 2.0;
    sum += texture2D(tex, v_texcoord + vec2(0.0, halfpixel.y * 2.0) * offset);
    sum += texture2D(tex, v_texcoord + vec2(halfpixel.x, halfpixel.y) * offset) * 2.0;
    sum += texture2D(tex, v_texcoord + vec2(halfpixel.x * 2.0, 0.0) * offset);
    sum += texture2D(tex, v_texcoord + vec2(halfpixel.x, -halfpixel.y) * offset) * 2.0;
    sum += texture2D(tex, v_texcoord + vec2(0.0, -halfpixel.y * 2.0) * offset);
    sum += texture2D(tex, v_texcoord + vec2(-halfpixel.x, -halfpixel.y) * offset) * 2.0;
    gl_FragColor = sum / 12.;
}
