#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in float aBrightness;
layout(location = 2) in float aSize;

uniform mat4 u_view;
uniform mat4 u_projection;
uniform float u_time;

out float vBrightness;

void main()
{
    // Twinkle effect: modulate brightness with sine wave
    float twinkle = 0.6 + 0.4 * sin(u_time * 1.5 + aBrightness * 31.415);
    vBrightness = aBrightness * twinkle;

    gl_Position = u_projection * u_view * vec4(aPos, 1.0);
    gl_PointSize = aSize * (1.0 + 0.3 * sin(u_time * 2.0 + aBrightness * 17.3));
}
