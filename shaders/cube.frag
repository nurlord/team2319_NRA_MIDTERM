#version 430 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 color;
uniform vec3 cameraPos;
uniform float time;
uniform float reflectivity;
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

void main()
{
    vec3 N = normalize(Normal);
    vec3 V = normalize(cameraPos - FragPos);
    vec3 R = reflect(-V, N);

    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.0);
    float mixAmount = clamp(reflectivity + fresnel * 0.5, 0.0, 1.0);

    vec3 rotatedR = rotateDirection(R, time);
    vec3 envColor = texture(skyboxMap, rotatedR).rgb;
    vec3 base = color;

    vec3 lightDir = normalize(vec3(0.45, 0.8, 0.35));
    vec3 halfVector = normalize(lightDir + V);
    float diffuse = max(dot(N, lightDir), 0.0);
    float specularStrength = pow(max(dot(N, halfVector), 0.0), 32.0);

    vec3 diffuseColor = base * diffuse * 0.45;
    vec3 ambient = base * 0.55;
    vec3 specular = envColor * specularStrength * 0.35;

    vec3 shaded = ambient + diffuseColor + specular;
    vec3 finalColor = mix(shaded, envColor, mixAmount);

    FragColor = vec4(finalColor, 1.0);
}
