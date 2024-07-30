#include "renderer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <GLFW/glfw3.h>

static char *readFile(const char *filePath);
static GLuint compileShader(const char *source, GLenum shaderType);
static GLuint linkProgram(const char *simpleVertex, const char *simpleFragment);

void initRenderer(Renderer *renderer, int screenWidth, int screenHeight) {
    // Initialize VAO and VBO
    glGenVertexArrays(1, &renderer->vao);
    glGenBuffers(1, &renderer->vbo);
    glBindVertexArray(renderer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(renderer->vertices), NULL, GL_DYNAMIC_DRAW);

    // Vertex attribute setup
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(7 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    // Shader initialization
    int initialShaderCapacity = 4;
    renderer->shaders = malloc(sizeof(Shader) * initialShaderCapacity);
    if (!renderer->shaders) {
        fprintf(stderr, "Failed to allocate memory for shaders\n");
        exit(1);
    }
    renderer->numShaders = 0;
    renderer->maxShaders = initialShaderCapacity;

    // Load initial shaders
    initShaders(renderer);

    // Set up the projection matrix
    updateProjectionMatrix(renderer, screenWidth, screenHeight);
}

void freeRenderer(Renderer *renderer) {
    // Delete shaders
    for (int i = 0; i < renderer->numShaders; i++) {
        glDeleteProgram(renderer->shaders[i].shaderID);
        free(renderer->shaders[i].name);
    }
    free(renderer->shaders); // Free the shader array itself

    // Delete VBO and VAO
    glDeleteBuffers(1, &renderer->vbo);
    glDeleteVertexArrays(1, &renderer->vao);
}

int newShader(Renderer *renderer,
              const char *vertexPath, const char *fragmentPath,
              const char *shaderName)
{
  if (renderer->numShaders >= renderer->maxShaders) {
    // Optionally resize the array
    int newCapacity = renderer->maxShaders * 2;
    Shader *newArray = realloc(renderer->shaders, sizeof(Shader) * newCapacity);
    if (!newArray)
      return -1; // Allocation failed
    renderer->shaders = newArray;
    renderer->maxShaders = newCapacity;
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
  renderer->shaders[renderer->numShaders++] = newShader;
  return renderer->numShaders - 1; // Return the index of the new shader
}

void initShaders(Renderer *renderer) {
  newShader(renderer, "./shaders/simple.vert", "./shaders/simple.frag", "simple");
  newShader(renderer, "./shaders/wave.vert", "./shaders/cool.frag", "wave");
}

void useShader(Renderer *renderer, const char *shaderName) {
    for (int i = 0; i < renderer->numShaders; i++) {
        if (strcmp(renderer->shaders[i].name, shaderName) == 0) {
            renderer->activeShader = renderer->shaders[i].shaderID;
            glUseProgram(renderer->activeShader);
            break;
        }
    }
}


void flush(Renderer *renderer) {
    glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * renderer->vertexCount, renderer->vertices, GL_DYNAMIC_DRAW);

    glUseProgram(renderer->activeShader);

    // Update projection matrix in the shader
    GLint projMatrixLocation = glGetUniformLocation(renderer->activeShader, "projectionMatrix");
    if (projMatrixLocation != -1) {
        glUniformMatrix4fv(projMatrixLocation, 1, GL_FALSE, renderer->projectionMatrix);
    } else {
        fprintf(stderr, "Could not find projectionMatrix uniform location\n");
    }

    // the time uniform TODO is renderer->activeShader correct here ?
    // what if the active shader doesn't use the time uniform at all but the next shader does
    GLint timeLocation = glGetUniformLocation(renderer->activeShader, "time");
    if (timeLocation != -1) {
        float currentTime = glfwGetTime();
        glUniform1f(timeLocation, currentTime);
    }


    glBindVertexArray(renderer->vao);
    glDrawArrays(GL_TRIANGLES, 0, renderer->vertexCount);

    renderer->vertexCount = 0;
}

void drawVertex(Renderer *renderer, Vec2f position, Color color, Vec2f uv) {
    if (renderer->vertexCount >= VERTICIES_CAP) {
        flush(renderer);
    }

    Vertex *vertex = &renderer->vertices[renderer->vertexCount++];
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

void drawTriangleColors(Renderer *renderer,
                  Vec2f p1, Color c1, Vec2f uv1,
                  Vec2f p2, Color c2, Vec2f uv2,
                  Vec2f p3, Color c3, Vec2f uv3)
{
    drawVertex(renderer, p1, c1, uv1);
    drawVertex(renderer, p2, c2, uv2);
    drawVertex(renderer, p3, c3, uv3);
}


void drawTriangle(Renderer *renderer, Color color,
                  Vec2f p1, Vec2f p2, Vec2f p3)
{
  Vec2f uv1 = {0.0f, 0.0f};
  Vec2f uv2 = {1.0f, 0.0f};
  Vec2f uv3 = {0.5f, 1.0f};

  drawVertex(renderer, p1, color, uv1);
  drawVertex(renderer, p2, color, uv2);
  drawVertex(renderer, p3, color, uv3);
}

// TODO drawQuad() where you specify the UV's quads are used for textures
void drawRectangle(Renderer *renderer, Vec2f position, Vec2f size, Color color) {
    Vec2f p1 = {position.x, position.y};
    Vec2f p2 = {position.x + size.x, position.y};
    Vec2f p3 = {position.x, position.y + size.y};
    Vec2f p4 = {position.x + size.x, position.y + size.y};

    Vec2f uv1 = {0.0f, 0.0f};
    Vec2f uv2 = {1.0f, 0.0f};
    Vec2f uv3 = {0.0f, 1.0f};
    Vec2f uv4 = {1.0f, 1.0f};

    drawTriangleColors(renderer, p1, color, uv1, p3, color, uv3, p2, color, uv2);
    drawTriangleColors(renderer, p2, color, uv2, p3, color, uv3, p4, color, uv4);
}


void updateProjectionMatrix(Renderer *renderer, int width, int height) {
    float near = -1.0f;
    float far = 1.0f;
    float left = 0.0f;
    float right = (float) width;
    float top = (float) height;
    float bottom = 0.0f;

    // Set orthographic projection matrix (column-major order)
    memset(renderer->projectionMatrix, 0, sizeof(renderer->projectionMatrix));
    renderer->projectionMatrix[0] = 2.0f / (right - left);
    renderer->projectionMatrix[5] = 2.0f / (top - bottom);
    renderer->projectionMatrix[10] = -2.0f / (far - near);
    renderer->projectionMatrix[12] = -(right + left) / (right - left);
    renderer->projectionMatrix[13] = -(top + bottom) / (top - bottom);
    renderer->projectionMatrix[14] = -(far + near) / (far - near);
    renderer->projectionMatrix[15] = 1.0f;
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

