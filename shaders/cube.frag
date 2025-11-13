#version 430 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 color;
uniform vec3 cameraPos;
uniform float time;
uniform float reflectivity;

float hash(vec3 p) {
    p = fract(p * 0.3183099 + vec3(0.1, 0.3, 0.7));
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

vec3 evaluateSky(vec3 dir) {
    dir = normalize(dir);
    float horizon = smoothstep(-0.2, 0.6, dir.y);
    vec3 skyTop = vec3(0.05, 0.15, 0.35);
    vec3 skyMid = vec3(0.15, 0.25, 0.55);
    vec3 baseSky = mix(skyMid, skyTop, horizon);

    float wavePhase = time * 0.15;
    float band = sin((dir.x + dir.z) * 4.0 + wavePhase) * 0.5 + 0.5;
    float swirl = sin(dir.y * 12.0 + time * 0.2) * 0.5 + 0.5;
    vec3 clouds = mix(vec3(0.12, 0.18, 0.28), vec3(0.45, 0.55, 0.75), band * swirl);
    baseSky = mix(baseSky, clouds, 0.35 * horizon);

    float aurora = pow(max(dir.y, 0.0), 3.0) * (sin(dir.x * 10.0 + time * 0.6) * 0.5 + 0.5);
    vec3 auroraColor = vec3(0.1, 0.7, 0.55) * aurora * 0.35;

    float starLayer = hash(floor(dir * 40.0 + time * 2.0));
    float stars = smoothstep(0.98, 1.0, starLayer) * (1.0 - horizon) * 0.7;

    vec3 groundColor = vec3(0.02, 0.04, 0.08);
    float blend = smoothstep(-0.4, 0.1, dir.y);

    return mix(groundColor, baseSky + auroraColor, blend) + stars;
}

void main()
{
    vec3 N = normalize(Normal);
    vec3 V = normalize(cameraPos - FragPos);
    vec3 R = reflect(-V, N);

    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.0);
    float mixAmount = clamp(reflectivity + fresnel * 0.5, 0.0, 1.0);

    vec3 envColor = evaluateSky(R);
    vec3 base = color;

    vec3 lightDir = normalize(vec3(0.45, 0.8, 0.35));
    float diffuse = max(dot(N, lightDir), 0.0);
    vec3 diffuseColor = base * diffuse * 0.45;
    vec3 ambient = base * 0.55;

    vec3 shaded = ambient + diffuseColor;
    vec3 finalColor = mix(shaded, envColor, mixAmount);

    FragColor = vec4(finalColor, 1.0);
}
