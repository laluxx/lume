#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord; // UV coordinates

uniform mat4 projectionMatrix;
uniform float time;

out vec3 ourColor;
out vec2 TexCoord;

void main()
{
    vec4 projectedPos = projectionMatrix * vec4(aPos, 1.0);

    vec3 animatedPos = projectedPos.xyz;
    animatedPos.y += sin(time + aPos.x * 1.0) * 0.1;

    gl_Position = vec4(animatedPos, projectedPos.w);
    
    ourColor = aColor;
    TexCoord = aTexCoord;
}
