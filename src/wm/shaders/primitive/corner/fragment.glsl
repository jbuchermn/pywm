precision mediump float;
uniform float params_float[4];
uniform int params_int[1];
uniform float alpha;
varying vec2 v_texcoord;


uniform float width;
uniform float height;

void main() {
    if(params_int[0] == 0){
        if(length(vec2(v_texcoord.x*width, v_texcoord.y*height) - vec2(0, 0)) <
                params_float[0]) discard;
    }else if(params_int[0] == 1){
        if(length(vec2(v_texcoord.x*width, v_texcoord.y*height) - vec2(width, 0)) <
                params_float[0]) discard;
    }else if(params_int[0] == 2){
        if(length(vec2(v_texcoord.x*width, v_texcoord.y*height) - vec2(0, height)) <
                params_float[0]) discard;
    }else if(params_int[0] == 3){
        if(length(vec2(v_texcoord.x*width, v_texcoord.y*height) - vec2(width, height)) <
                params_float[0]) discard;
    }
	gl_FragColor = vec4(params_float[1], params_float[2], params_float[3], 1.) * alpha;
}

