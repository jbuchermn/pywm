uniform mat3 proj;
uniform bool invert_y;
attribute vec2 pos;
attribute vec2 texcoord;
varying vec2 v_texcoord;

void main() {
    gl_Position = vec4(proj * vec3(pos, 1.0), 1.0);
    if (invert_y) {
        v_texcoord = vec2(texcoord.x, 1.0 - texcoord.y);
    } else {
        v_texcoord = texcoord;
    }
}
