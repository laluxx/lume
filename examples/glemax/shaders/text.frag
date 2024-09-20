// NOTE LCD 
// #version 330 core
// in vec2 TexCoord;
// in vec4 ourColor;  // Receive color from vertex shader
// out vec4 FragColor;
// uniform sampler2D ourTexture;

// void main() {
//     vec3 textColor = texture(ourTexture, TexCoord).rgb;  // Get the RGB subpixel values
//     FragColor = vec4(textColor * ourColor.rgb, ourColor.a);  // Multiply by the vertex color for any additional tinting
// }



// NOTE Grayscale
#version 330 core
in vec2 TexCoord;
in vec4 ourColor;  // Receive color from vertex shader

out vec4 FragColor;

uniform sampler2D ourTexture;

void main() {
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(ourTexture, TexCoord).r);
    FragColor = ourColor * sampled;  // Apply vertex color to texture
}
