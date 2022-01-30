precision mediump float;
uniform float params_float[4];
uniform float alpha;
varying vec2 v_texcoord;

uniform float width;
uniform float height;

void main() {
	gl_FragColor = vec4(
            params_float[0],
            params_float[1],
            params_float[2],
            1.
    ) * params_float[3] * alpha;
}

