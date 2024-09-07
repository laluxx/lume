#ifndef FONT_H
#define FONT_H

#include "ft2build.h"
#include FT_FREETYPE_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "renderer.h"

typedef struct {
    float ax;  // advance.x
    float ay;  // advance.y
    float bw;  // bitmap.width
    float bh;  // bitmap.rows
    float bl;  // bitmap_left
    float bt;  // bitmap_top
    float tx;  // x offset of glyph in texture coordinates
    float ty;  // y offset of glyph in texture coordinates
} Character;

#define MAX_CHARACTERS 128  // NOTE Supporting basic ASCII

typedef struct {
    GLuint textureID;     // ID handle of the glyph texture
    unsigned int width;   // width of the texture atlas
    unsigned int height;  // height of the texture atlas
    Character characters[MAX_CHARACTERS];
} Font;

void initFreeType();
Font* loadFont(const char* fontPath, int fontSize);
void drawText(Font* font, const char* text, float x, float y, float sx, float sy);
float getFontHeight(Font* font);
void freeFont(Font* font);

#endif // FONT_H