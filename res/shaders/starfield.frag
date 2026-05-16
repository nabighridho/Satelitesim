#version 460 core

in float vBrightness;

out vec4 FragColor;

void main()
{
    // Circular point: discard corners to make round stars
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = length(coord);
    if (dist > 0.5)
        discard;

    // Soft glow falloff from center
    float alpha = smoothstep(0.5, 0.1, dist);

    // Warm/cool color variation based on brightness
    vec3 coolStar = vec3(0.7, 0.8, 1.0);   // blue-white
    vec3 warmStar = vec3(1.0, 0.9, 0.7);   // warm yellow
    vec3 color = mix(coolStar, warmStar, vBrightness);

    FragColor = vec4(color * vBrightness, alpha * vBrightness);
}
