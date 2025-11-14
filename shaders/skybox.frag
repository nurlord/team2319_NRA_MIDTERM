#version 430 core
out vec4 FragColor;

in vec3 vDirection;

uniform float time;
uniform samplerCube skyboxMap;

mat3 rotationX(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat3(1.0, 0.0, 0.0,
                0.0, c,   -s,
                0.0, s,    c);
}

mat3 rotationY(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat3(c,   0.0, s,
                0.0, 1.0, 0.0,
               -s,   0.0, c);
}

mat3 rotationZ(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat3(c,  -s, 0.0,
                s,   c, 0.0,
                0.0, 0.0, 1.0);
}

vec3 rotateDirection(vec3 dir, float t) {
    float yaw = t * 0.045;
    float pitch = sin(t * 0.18) * 0.12;
    float roll = cos(t * 0.11) * 0.05;
    mat3 rot = rotationY(yaw) * rotationX(pitch) * rotationZ(roll);
    return rot * dir;
}

void main() {
    vec3 dir = normalize(vDirection);
    vec3 sampleDir = rotateDirection(dir, time);
    vec3 color = texture(skyboxMap, sampleDir).rgb;

    float shimmer = sin(dot(sampleDir.xz, vec2(3.1, 4.3)) + time * 0.6) * 0.025;
    color += shimmer;

    FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
