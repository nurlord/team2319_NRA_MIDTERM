#version 430 core
out vec4 FragColor;

in vec3 vDirection;

uniform float time;

float hash(vec3 p) {
    p = fract(p * 0.3183099 + vec3(0.1, 0.3, 0.7));
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

void main() {
    vec3 dir = normalize(vDirection);

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

    vec3 color = mix(groundColor, baseSky + auroraColor, blend) + stars;

    FragColor = vec4(color, 1.0);
}
