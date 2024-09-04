#version 330 core
in vec2 TexCoord;
in vec3 ourColor;

out vec4 FragColor;

uniform sampler2D ourTexture;
// uniform vec3 textColor;       // TODO Pass the color as a uniform

void main() {
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(ourTexture, TexCoord).r);
    FragColor = vec4(255.0, 255.0, 255.0, 1.0) * sampled;
}
