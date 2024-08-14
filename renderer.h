#ifndef RENDERER_H
#define RENDERER_H

#include <GL/glew.h>
#include "common.h"

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
    Shader *shaders; // Dynamic array of shaders
    int numShaders;  // Number of shaders currently loaded
    int maxShaders;  // Capacity of the shader array
    GLuint activeShader;
    Vertex vertices[VERTICIES_CAP];
    GLfloat projectionMatrix[16];
    size_t vertexCount;
} Renderer;

void initRenderer(Renderer *renderer, int screenWidth, int screenHeight);
void freeRenderer(Renderer *renderer);

int newShader(Renderer *renderer,
              const char *vertexPath, const char *fragmentPath,
              const char *shaderName);

void initShaders(Renderer *renderer);
void useShader(Renderer *renderer, const char *shaderName);
void flush(Renderer *renderer);

void drawVertex(Renderer *renderer, Vec2f position, Color color, Vec2f uv);

void drawTriangle(Renderer *renderer, Color color,
                  Vec2f p1, Vec2f p2, Vec2f p3);

void drawTriangleColors(Renderer *renderer,
                        Vec2f p1, Color c1, Vec2f uv1,
                        Vec2f p2, Color c2, Vec2f uv2,
                        Vec2f p3, Color c3, Vec2f uv3);

void drawRectangle(Renderer *renderer, Vec2f position, Vec2f size, Color color);

void updateProjectionMatrix(Renderer *renderer, int width, int height);

void deleteShaders(Renderer *renderer);
void reloadShaders(Renderer *renderer);




#endif  // RENDERER_H
