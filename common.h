#ifndef COMMON_H
#define COMMON_H

typedef struct {
    float x;
    float y;
} Vec2f;

typedef struct {
    float x;
    float y;
    float z;
} Vec3f;

typedef struct {
    float r;
    float g;
    float b;
    float a;
} Color;

#define BLACK   ((Color){0.0f, 0.0f, 0.0f, 1.0f})
#define WHITE   ((Color){1.0f, 1.0f, 1.0f, 1.0f})
#define RED     ((Color){1.0f, 0.0f, 0.0f, 1.0f})
#define GREEN   ((Color){0.0f, 1.0f, 0.0f, 1.0f})
#define BLUE    ((Color){0.0f, 0.0f, 1.0f, 1.0f})
#define YELLOW  ((Color){1.0f, 1.0f, 0.0f, 1.0f})
#define CYAN    ((Color){0.0f, 1.0f, 1.0f, 1.0f})
#define MAGENTA ((Color){1.0f, 0.0f, 1.0f, 1.0f})
#define GRAY    ((Color){0.5f, 0.5f, 0.5f, 1.0f})

#endif  // COMMON_H
