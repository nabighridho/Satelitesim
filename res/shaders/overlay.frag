#version 460 core

in vec2 TexCoord;

uniform vec4 u_color;
uniform int u_useTexture;
uniform sampler2D u_fontAtlas;

out vec4 FragColor;

void main()
{
    if (u_useTexture == 1)
    {
        float alpha = texture(u_fontAtlas, TexCoord).r;
        if (alpha < 0.1)
            discard;
        FragColor = vec4(u_color.rgb, u_color.a * alpha);
    }
    else
    {
        FragColor = u_color;
    }
}
