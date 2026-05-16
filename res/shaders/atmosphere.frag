#version 460 core

in vec3 vWorldPos;
in vec3 vWorldNormal;

uniform vec3 u_viewPos;
uniform vec3 u_sunDir;   // direction FROM sun (normalized)
uniform float u_time;

out vec4 FragColor;

void main()
{
    vec3 viewDir = normalize(u_viewPos - vWorldPos);
    vec3 normal = normalize(vWorldNormal);

    // Fresnel: bright at edges, transparent at center
    float fresnel = 1.0 - abs(dot(viewDir, normal));
    fresnel = pow(fresnel, 2.5);

    // Day/night: brighter on sun-lit side
    float sunFactor = dot(normal, u_sunDir);
    sunFactor = smoothstep(-0.3, 0.5, sunFactor);

    // Atmosphere color: blue with slight green on terminator
    vec3 dayColor = vec3(0.3, 0.6, 1.0);
    vec3 twilightColor = vec3(0.8, 0.4, 0.2);
    vec3 atmoColor = mix(twilightColor, dayColor, smoothstep(0.0, 0.4, sunFactor));

    // Subtle breathing animation
    float breathe = 0.9 + 0.1 * sin(u_time * 0.5);

    float alpha = fresnel * (0.3 + 0.7 * sunFactor) * breathe;
    alpha = clamp(alpha, 0.0, 0.85);

    FragColor = vec4(atmoColor, alpha * 0.6);
}
