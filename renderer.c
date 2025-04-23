#include "renderer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// TODO drawCricle()
// TODO drawRectangleEx() with rounded borders

Renderer renderer = {0};


GLuint fillTexture(int width, int height, unsigned char* data, int channels) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                 GL_UNSIGNED_BYTE, data);
    
    printf("[FILLED TEXTURE] %d x %d %d\n", width, height, channels);
    
    return textureID;
}

GLuint loadTexture(const char *filepath) {
    stbi_set_flip_vertically_on_load(1);
    int width, height, channels;
    unsigned char *data = stbi_load(filepath, &width, &height, &channels, 0);
    if (!data) {
        fprintf(stderr, "Failed to load texture: %s\n", filepath);
        return 0;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                 GL_UNSIGNED_BYTE, data);

    printf("[LOADED TEXTURE] %d x %d %d %s\n", width, height, channels, filepath);

    stbi_image_free(data);
    
    return textureID;
}

void getTextureSize(GLuint textureID, int* width, int* height) {
    glBindTexture(GL_TEXTURE_2D, textureID);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, height);
}

void drawTextureOriginal(Vec2f position, GLuint textureID) {
    drawTextureScaled(position, textureID, 1.0f);
}

void drawTextureScaled(Vec2f position, GLuint textureID, float scale) {
    int width, height;
    getTextureSize(textureID, &width, &height);

    // Apply scale to dimensions
    float scaledWidth = width * scale;
    float scaledHeight = height * scale;

    glBindTexture(GL_TEXTURE_2D, textureID);

    Vec2f p1 = {position.x, position.y};
    Vec2f p2 = {position.x + scaledWidth, position.y};
    Vec2f p3 = {position.x, position.y + scaledHeight};
    Vec2f p4 = {position.x + scaledWidth, position.y + scaledHeight};

    Vec2f uv1 = {0.0f, 0.0f};
    Vec2f uv2 = {1.0f, 0.0f};
    Vec2f uv3 = {0.0f, 1.0f};
    Vec2f uv4 = {1.0f, 1.0f};

    Color white = {255, 255, 255, 1.0};

    drawTriangleEx(p1, white, uv1,
                   p3, white, uv3,
                   p2, white, uv2);
    
    drawTriangleEx(p2, white, uv2,
                   p3, white, uv3,
                   p4, white, uv4);
}


void drawTexture(Vec2f position, Vec2f size, GLuint textureID) {

    /* glActiveTexture(GL_TEXTURE0); */

    glBindTexture(GL_TEXTURE_2D, textureID);

    Vec2f p1 = {position.x, position.y};
    Vec2f p2 = {position.x + size.x, position.y};
    Vec2f p3 = {position.x, position.y + size.y};
    Vec2f p4 = {position.x + size.x, position.y + size.y};

    Vec2f uv1 = {0.0f, 0.0f};
    Vec2f uv2 = {1.0f, 0.0f};
    Vec2f uv3 = {0.0f, 1.0f};
    Vec2f uv4 = {1.0f, 1.0f};

    drawTriangleEx(p1, (Color){255, 255, 255, 1.0}, uv1,
                       p3, (Color){255, 255, 255, 1.0}, uv3,
                       p2, (Color){255, 255, 255, 1.0}, uv2);
    
    drawTriangleEx(p2, (Color){255, 0, 0, 1.0}, uv2,
                       p3, (Color){255, 0, 0, 1.0}, uv3,
                       p4, (Color){255, 0, 0, 1.0}, uv4);

    /* glBindTexture(textureID, 0); */
}



static char *readFile(const char *filePath);
static GLuint compileShader(const char *source, GLenum shaderType);
static GLuint linkProgram(const char *simpleVertex, const char *simpleFragment);

void initRenderer(int screenWidth, int screenHeight) {
    // Initialize VAO and VBO
    glGenVertexArrays(1, &renderer.vao);
    glGenBuffers(1, &renderer.vbo);
    glBindVertexArray(renderer.vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(renderer.vertices), NULL, GL_DYNAMIC_DRAW);

    // Vertex attribute setup
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(7 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    // Shader initialization
    int initialShaderCapacity = 4;
    renderer.shaders = malloc(sizeof(Shader) * initialShaderCapacity);
    if (!renderer.shaders) {
        fprintf(stderr, "Failed to allocate memory for shaders\n");
        exit(1);
    }
    renderer.numShaders = 0;
    renderer.maxShaders = initialShaderCapacity;

    // Load initial shaders
    initShaders();
    // Set up the projection matrix
    updateProjectionMatrix(screenWidth, screenHeight);
}

void freeRenderer() {
    // Delete shaders
    for (int i = 0; i < renderer.numShaders; i++) {
        glDeleteProgram(renderer.shaders[i].shaderID);
        free(renderer.shaders[i].name);
    }
    free(renderer.shaders); // Free the shader array itself

    // Delete VBO and VAO
    glDeleteBuffers(1, &renderer.vbo);
    glDeleteVertexArrays(1, &renderer.vao);
}


int newShaderString(const char *vertexSrc, const char *fragmentSrc, const char *shaderName) {
    if (renderer.numShaders >= renderer.maxShaders) {
        // Optionally resize the array
        int newCapacity = renderer.maxShaders * 2;
        Shader *newArray = realloc(renderer.shaders, sizeof(Shader) * newCapacity);
        if (!newArray) {
            fprintf(stderr, "Failed to allocate memory for new shaders\n");
            return -1; // Allocation failed
        }
        renderer.shaders = newArray;
        renderer.maxShaders = newCapacity;
    }

    GLuint vertexShader = compileShader(vertexSrc, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentSrc, GL_FRAGMENT_SHADER);

    if (vertexShader == 0 || fragmentShader == 0) {
        if (vertexShader != 0) glDeleteShader(vertexShader);
        if (fragmentShader != 0) glDeleteShader(fragmentShader);
        return -1; // Shader compilation failed
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER_PROGRAM_LINKING_FAILED\n%s\n", infoLog);
        glDeleteProgram(program); // Don't leak the program
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return -1;
    }

    // Detach and delete shaders after linking
    glDetachShader(program, vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    Shader newShader = {
        .shaderID = program,
        .name = strdup(shaderName) // Duplicate name string
    };
    renderer.shaders[renderer.numShaders++] = newShader;

    return renderer.numShaders - 1; // Return the index of the new shader
}

// TODO Make a version that takes the renderer 2 strings and the name
int newShader(const char *vertexPath, const char *fragmentPath,
              const char *shaderName)
{
    if (renderer.numShaders >= renderer.maxShaders) {
        // Optionally resize the array
        int newCapacity = renderer.maxShaders * 2;
        Shader *newArray = realloc(renderer.shaders, sizeof(Shader) * newCapacity);
        if (!newArray)
            return -1; // Allocation failed
        renderer.shaders = newArray;
        renderer.maxShaders = newCapacity;
    }

    char *vertexSource = readFile(vertexPath);
    char *fragmentSource = readFile(fragmentPath);
    GLuint program = linkProgram(vertexSource, fragmentSource);
    free(vertexSource);
    free(fragmentSource);

    if (program == 0)
        return -1; // Shader compilation/linking failed

    Shader newShader = {
        .shaderID = program,
        .name = strdup(shaderName) // Duplicate name string
    };
    renderer.shaders[renderer.numShaders++] = newShader;
    return renderer.numShaders - 1; // Return the index of the new shader
}


void deleteShaders() {
    for (int i = 0; i < renderer.numShaders; i++) {
        glDeleteProgram(renderer.shaders[i].shaderID);
        free(renderer.shaders[i].name);
        renderer.shaders[i].shaderID = 0;
    }
    renderer.numShaders = 0;
    // renderer->activeShader = 0;
}

void reloadShaders() {
    deleteShaders();
    initShaders();
}

void initShaders() {
    newShader("./shaders/simple.vert", "./shaders/simple.frag",  "simple");
    newShader("./shaders/wave.vert",   "./shaders/simple.frag",  "wave");
    newShader("./shaders/simple.vert", "./shaders/circle.frag",  "circle");
    newShader("./shaders/simple.vert", "./shaders/texture.frag", "texture");
    newShader("./shaders/simple.vert", "./shaders/text.frag",    "text");
    newShader("./shaders/simple.vert", "./shaders/gaytext.frag", "gay");
}

void useShader(const char *shaderName) {
    for (int i = 0; i < renderer.numShaders; i++) {
        if (strcmp(renderer.shaders[i].name, shaderName) == 0) {
            renderer.activeShader = renderer.shaders[i].shaderID;
            glUseProgram(renderer.activeShader);
            break;
        }
    }
}

void flush() {
    glBindBuffer(GL_ARRAY_BUFFER, renderer.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * renderer.vertexCount, renderer.vertices, GL_DYNAMIC_DRAW);

    glUseProgram(renderer.activeShader);


    // Update projection matrix
    // TODO move uniform update into a function that run
    // only once per frame
    GLint projMatrixLocation = glGetUniformLocation(renderer.activeShader, "projectionMatrix");
    if (projMatrixLocation != -1) {
        glUniformMatrix4fv(projMatrixLocation, 1, GL_FALSE, renderer.projectionMatrix);
    } else {
        fprintf(stderr, "Could not find projectionMatrix uniform location\n");
    }

    // TODO is renderer->activeShader correct here ?
    // what if the active shader doesn't use the time uniform at all but the next shader does
    GLint timeLocation = glGetUniformLocation(renderer.activeShader, "time");
    if (timeLocation != -1) {
        float currentTime = glfwGetTime();
        glUniform1f(timeLocation, currentTime);
    }

    glBindVertexArray(renderer.vao);
    glDrawArrays(GL_TRIANGLES, 0, renderer.vertexCount);

    renderer.vertexCount = 0;
}

void drawVertex(Vec2f position, Color color, Vec2f uv) {
    /* if (renderer.vertexCount >= VERTICIES_CAP) { */
    /*     flush(); */
    /* } */

    Vertex *vertex = &renderer.vertices[renderer.vertexCount++];
    vertex->x = position.x;
    vertex->y = position.y;
    vertex->z = 0.0f; // Z is always 0 for 2D
    vertex->r = color.r;
    vertex->g = color.g;
    vertex->b = color.b;
    vertex->a = color.a;
    vertex->u = uv.x;
    vertex->v = uv.y;
}

void drawTriangleEx(Vec2f p1, Color c1, Vec2f uv1,
                    Vec2f p2, Color c2, Vec2f uv2,
                    Vec2f p3, Color c3, Vec2f uv3)
{
    drawVertex(p1, c1, uv1);
    drawVertex(p2, c2, uv2);
    drawVertex(p3, c3, uv3);
}


void drawTriangle(Color color,
                  Vec2f p1, Vec2f p2, Vec2f p3)
{
    Vec2f uv1 = {0.0f, 0.0f};
    Vec2f uv2 = {1.0f, 0.0f};
    Vec2f uv3 = {0.5f, 1.0f};

    drawVertex(p1, color, uv1);
    drawVertex(p2, color, uv2);
    drawVertex(p3, color, uv3);
}

void drawRectangle(Vec2f position, Vec2f size, Color color) {
    Vec2f p1 = {position.x, position.y};
    Vec2f p2 = {position.x + size.x, position.y};
    Vec2f p3 = {position.x, position.y + size.y};
    Vec2f p4 = {position.x + size.x, position.y + size.y};

    Vec2f uv1 = {0.0f, 0.0f};
    Vec2f uv2 = {1.0f, 0.0f};
    Vec2f uv3 = {0.0f, 1.0f};
    Vec2f uv4 = {1.0f, 1.0f};

    drawTriangleEx(p1, color, uv1, p3, color, uv3, p2, color, uv2);
    drawTriangleEx(p2, color, uv2, p3, color, uv3, p4, color, uv4);
}


void drawRectangleLines(Vec2f position, Vec2f size, Color color, float lineThickness) {
    drawRectangle((Vec2f){position.x, position.y}, 
                  (Vec2f){size.x, lineThickness}, 
                  color);

    drawRectangle((Vec2f){position.x, position.y + size.y - lineThickness},
                  (Vec2f){size.x, lineThickness},
                  color);

    drawRectangle((Vec2f){position.x, position.y},
                  (Vec2f){lineThickness, size.y},
                  color);

    drawRectangle((Vec2f){position.x + size.x - lineThickness, position.y},
                  (Vec2f){lineThickness, size.y},
                  color);
}



void drawLine(Vec2f start, Vec2f end, Color color, float thickness) {
    Vec2f position, size;

    if (start.y == end.y) { // Horizontal line
        position = (Vec2f){fminf(start.x, end.x), start.y - thickness / 2};
        size = (Vec2f){fabsf(end.x - start.x), thickness};
    } else if (start.x == end.x) { // Vertical line
        position = (Vec2f){start.x - thickness / 2, fminf(start.y, end.y)};
        size = (Vec2f){thickness, fabsf(end.y - start.y)};
    } else {
        fprintf(stderr, "drawLine only supports strictly horizontal or vertical lines.\n");
        return;
    }

    drawRectangle(position, size, color);
}



void updateProjectionMatrix(int width, int height) {
    float near = -1.0f;
    float far = 1.0f;
    float left = 0.0f;
    float right = (float) width;
    float top = (float) height;
    float bottom = 0.0f;

    // Set orthographic projection matrix (column-major order)
    memset(renderer.projectionMatrix, 0, sizeof(renderer.projectionMatrix));
    renderer.projectionMatrix[0] = 2.0f / (right - left);
    renderer.projectionMatrix[5] = 2.0f / (top - bottom);
    renderer.projectionMatrix[10] = -2.0f / (far - near);
    renderer.projectionMatrix[12] = -(right + left) / (right - left);
    renderer.projectionMatrix[13] = -(top + bottom) / (top - bottom);
    renderer.projectionMatrix[14] = -(far + near) / (far - near);
    renderer.projectionMatrix[15] = 1.0f;
}

static char *readFile(const char *filePath) {
    FILE *file = fopen(filePath, "r");
    if (!file) {
        fprintf(stderr, "Unable to open file: %s\n", filePath);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = (char *)malloc(length + 1);
    fread(content, 1, length, file);
    content[length] = '\0';

    fclose(file);
    return content;
}

static GLuint compileShader(const char *source, GLenum shaderType) {
    // Create the shader object
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    // Error checking
    GLint success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER_COMPILATION_FAILED\n%s\n", infoLog);
        glDeleteShader(shader); // Don't leak the shader.
        return 0;
    }
    return shader;
}

static GLuint linkProgram(const char *vertexSource, const char *fragmentSource) {
    GLuint vertexShader = compileShader(vertexSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentSource, GL_FRAGMENT_SHADER);
    if (vertexShader == 0 || fragmentShader == 0) {
        if (vertexShader != 0) glDeleteShader(vertexShader);
        if (fragmentShader != 0) glDeleteShader(fragmentShader);
        return 0; // One of the shaders failed to compile, exit early.
    }

    // Create program and link
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    // Check for linking errors
    GLint success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER_LINKING_FAILED\n%s\n", infoLog);
        glDeleteProgram(program); // Don't leak the program
        program = 0;
    }

    // Cleanup: detach and delete shaders
    glDetachShader(program, vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

GLint getUniformLocation(const char* uniformName) {
    if (renderer.activeShader == 0) {
        fprintf(stderr, "No active shader program.\n");
        return -1;
    }
    GLint location = glGetUniformLocation(renderer.activeShader, uniformName);
    if (location == -1) {
        fprintf(stderr, "Uniform '%s' not found in the current shader program.\n", uniformName);
    }
    return location;
}

void uniform4f(const char *name, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        glUniform4f(location, v0, v1, v2, v3);
    }
}

void beginScissorMode(Vec2f position, Vec2f size) {
    glEnable(GL_SCISSOR_TEST);
    glScissor((int)position.x, (int)position.y, (int)size.x, (int)size.y);
}

void endScissorMode(void) {
    glDisable(GL_SCISSOR_TEST); // Disable scissor test
}



// Basic easing functions
float easeLinear(float t)    { return t; }
float easeInQuad(float t)    { return t * t; }
float easeOutQuad(float t)   { return t * (2 - t); }
float easeInOutQuad(float t) { return t < 0.5 ? 2 * t * t : -1 + (4 - 2 * t) * t;}


void deleteTextures(GLsizei n, const TextureID *textures) {
    glDeleteTextures(n, textures);
}
