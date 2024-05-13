#ifndef RENDERER_H
#define RENDERER_H

#include <GL/glew.h>
#include "common.h"

#define MAX_SHADERS 3
#define VERTICIES_CAP (3*640*1000)
/* static_assert(VERTICIES_CAP%3 == 0, "vertex capacity must be divisible by 3. We are rendering triangles after all."); */

typedef struct {
    GLfloat x, y, z;
    GLfloat r, g, b, a;
    GLfloat u, v;
} Vertex;

typedef struct {
    char *name;
    GLint location;
} Uniform;

typedef struct {
    GLuint shaderID;
    char *name;
    Uniform *uniforms;
    int uniformCount;
} Shader;

typedef struct {
    GLuint vao;
    GLuint vbo;
    Shader shaders[MAX_SHADERS]; // TODO dynamic memory allocation
    GLuint activeShader;
    Vertex vertices[VERTICIES_CAP];
    GLfloat projectionMatrix[16];
    size_t vertexCount;
} Renderer;

void initRenderer(Renderer *renderer, int screenWidth, int screenHeight);
void freeRenderer(Renderer *renderer);
void initShaders(Renderer *renderer);
void useShader(Renderer *renderer, const char *shaderName);
void flush(Renderer *renderer);

void drawVertex(Renderer *renderer, Vec2f position, Color color, Vec2f uv);

void drawTriangle(Renderer *renderer,
                  Vec2f p1, Color c1, Vec2f uv1,
                  Vec2f p2, Color c2, Vec2f uv2,
                  Vec2f p3, Color c3, Vec2f uv3);

void drawRectangle(Renderer *renderer, Vec2f position, Vec2f size, Color color);

void updateProjectionMatrix(Renderer *renderer, int width, int height);

#endif  // RENDERER_H
