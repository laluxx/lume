#version 330 core

out vec4 FragColor;

in vec2 TexCoord;
in vec3 ourColor;

void main()
{
    vec2 normalizedPos = TexCoord - vec2(0.5, 0.5);

    float dist = length(normalizedPos / vec2(0.5, 0.5));

    if (dist < 1.0)
        FragColor = vec4(ourColor, 1.0);
    else
        discard;
}


