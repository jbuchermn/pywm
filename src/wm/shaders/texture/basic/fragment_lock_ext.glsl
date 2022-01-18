#extension GL_OES_EGL_image_external : require
precision mediump float;

varying vec2 v_texcoord;
uniform samplerExternalOES texture0;
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
    float x = gl_FragCoord.x - offset_x;
    float y = gl_FragCoord.y - offset_y;

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

    gl_FragColor = texture2D(texture0, v_texcoord) * alpha * (1. - lock_perc);
};
