#version 430 core
out vec4 FragColor;

in vec2 vUV;

uniform float time;

float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

void main()
{
    vec2 uv = vUV;
    vec3 baseTop = vec3(0.09, 0.13, 0.21);
    vec3 baseBottom = vec3(0.03, 0.05, 0.08);
    vec3 baseColor = mix(baseBottom, baseTop, uv.y);

    vec2 cell = uv * 14.0;
    vec2 grid = abs(fract(cell - 0.5) - 0.5);
    float line = 1.0 - smoothstep(0.0, 0.015, min(grid.x, grid.y));

    float noise = hash(floor(cell));
    float twinkle = 0.04 * sin((uv.x + uv.y) * 18.0 + time * 0.35 + noise * 6.2831);

    float vignette = smoothstep(1.2, 0.35, distance(uv, vec2(0.5)));

    vec3 color = baseColor;
    color += twinkle;
    color = mix(color, color * 0.65, line * 0.4);
    color *= vignette;

    FragColor = vec4(color, 1.0);
}
