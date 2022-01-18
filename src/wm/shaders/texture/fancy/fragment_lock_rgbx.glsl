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

    float r = sqrt((v_texcoord.x - 0.5) * (v_texcoord.x - 0.5) + (v_texcoord.y - 0.5) * (v_texcoord.y - 0.5));
    float a = atan(v_texcoord.y - 0.5, v_texcoord.x - 0.5);
	gl_FragColor = vec4(texture2D(tex, vec2(0.5 + r*cos(a + lock_perc * 10.0 * (0.5 - r)), 0.5 + r*sin(a + lock_perc * 10.0 * (0.5 - r)))).rgb, 1.0) * alpha;
};
