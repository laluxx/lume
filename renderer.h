#ifndef RENDERER_H
#define RENDERER_H

#include <GL/glew.h>
#include "common.h"
#include <stdbool.h>

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

extern Renderer renderer;

void initRenderer(int screenWidth, int screenHeight);
void freeRenderer();


int newShaderString(const char *vertexSrc, const char *fragmentSrc,
                    const char *shaderName);

int newShader(const char *vertexPath, const char *fragmentPath,
              const char *shaderName);

void initShaders();
void useShader(const char *shaderName);
void flush();

void drawVertex(Vec2f position, Color color, Vec2f uv);

void drawTriangle(Color color,
                  Vec2f p1, Vec2f p2, Vec2f p3);

void drawTriangleEx(Vec2f p1, Color c1, Vec2f uv1,
                        Vec2f p2, Color c2, Vec2f uv2,
                        Vec2f p3, Color c3, Vec2f uv3);

void drawRectangle(Vec2f position, Vec2f size, Color color);

void updateProjectionMatrix(int width, int height);

void deleteShaders();
void reloadShaders();

GLuint loadTexture(const char* filepath);
void drawTexture(Vec2f position, Vec2f size, GLuint textureID);

GLint getUniformLocation(const char* uniformName);
void uniform4f(const char *name, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);

/* void beginScissorMode(int x, int y, int width, int height); */
void beginScissorMode(Vec2f position, Vec2f size);
void endScissorMode(void);




// TODO Hardcoded in lib shaders
/* const char* simple_vert = */
/*     "#version 330 core\n" */
/*     "layout (location = 0) in vec3 aPos;\n" */
/*     "layout (location = 1) in vec3 aColor;\n" */
/*     "layout (location = 2) in vec2 aTexCoord; // UV coordinates\n" */
/*     "\n" */
/*     "uniform mat4 projectionMatrix;\n" */
/*     "\n" */
/*     "out vec3 ourColor;\n" */
/*     "out vec2 TexCoord; // Pass UV coordinates to the fragment shader\n" */
/*     "\n" */
/*     "void main()\n" */
/*     "{\n" */
/*     "    gl_Position = projectionMatrix * vec4(aPos, 1.0);\n" */
/*     "    ourColor = aColor;\n" */
/*     "    TexCoord = aTexCoord; // Pass UV coordinates\n" */
/*     "}\n"; */


/* const char* simple_frag = */
/*     "#version 330 core\n" */
/*     "out vec4 FragColor;\n" */
/*     "\n" */
/*     "in vec3 ourColor;\n" */
/*     "in vec2 TexCoord; // Receive UV coordinates\n" */
/*     "\n" */
/*     "void main()\n" */
/*     "{\n" */
/*     "    // For now, just output the color. This shader doesn't yet use TexCoord.\n" */
/*     "    FragColor = vec4(ourColor, 1.0);\n" */
/*     "}\n"; */



#endif  // RENDERER_H
