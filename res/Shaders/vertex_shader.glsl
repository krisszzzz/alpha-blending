#version 460
layout (location = 0) in vec2  in_position;
layout (location = 1) in vec4  color;

out vec4 vertex_color;

void main() {
     vertex_color = vec4(color);
     gl_Position = vec4(in_position, 0.0, 1.0);
}
