precision mediump float;
uniform float params_float[5];
uniform float alpha;
varying vec2 v_texcoord;

uniform float width;
uniform float height;

void main() {
    if(v_texcoord.x*width > width - params_float[4] &&  v_texcoord.y*height > height - params_float[4]){
        if(length(vec2(v_texcoord.x*width, v_texcoord.y*height) - 
                    vec2(width - params_float[4], height - params_float[4])) > 
                params_float[4]) discard;
    }
    if(v_texcoord.x*width > width - params_float[4] &&  v_texcoord.y*height < params_float[4]){
        if(length(vec2(v_texcoord.x*width, v_texcoord.y*height) - 
                    vec2(width - params_float[4], params_float[4])) > 
                params_float[4]) discard;
    }
    if(v_texcoord.x*width < params_float[4] &&  v_texcoord.y*height > height - params_float[4]){
        if(length(vec2(v_texcoord.x*width, v_texcoord.y*height) - 
                    vec2(params_float[4], height - params_float[4])) > 
                params_float[4]) discard;
    }
    if(v_texcoord.x*width < params_float[4] &&  v_texcoord.y*height < params_float[4]){
        if(length(vec2(v_texcoord.x*width, v_texcoord.y*height) - 
                    vec2(params_float[4], params_float[4])) > 
                params_float[4]) discard;
    }
	gl_FragColor = vec4(
            params_float[0],
            params_float[1],
            params_float[2],
            1.
    ) * params_float[3] * alpha;
}

