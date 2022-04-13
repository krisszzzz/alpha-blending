#version 460

in  vec4 vertex_color;
out vec4 frag_color;

void main() {

    frag_color = vec4(vertex_color);

}
