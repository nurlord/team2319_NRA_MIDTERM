#version 430 core
layout(location = 0) in vec2 aPos;

uniform mat4 projection;
uniform mat4 model;
uniform vec3 color;

out vec3 fragColor;

void main() {
    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
    fragColor = color;
}
