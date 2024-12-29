#ifndef FONT_H
#define FONT_H

#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H

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
    int ascent;
    int descent;
    Character characters[MAX_CHARACTERS];
} Font;


void initFreeType();
Font* loadFont(const char* fontPath, int fontSize);
Font* loadFontEx(const char* fontPath, int fontSize, int* codepoints, int codepointCount);

void drawText(Font* font, const char* text, float x, float y, Color textColor);

void drawTextEx(Font* font, const char* text,
                float x, float y,
                float sx, float sy,
                Color textColor, Color highlightColor,
                int highlightPos, bool cursorVisible,
                char *shader);

void drawChar(Font* font, char character, float x, float y, float sx, float sy, Color color);

float getFontHeight(Font* font);
float getFontWidth(Font* font);
float getCharacterWidth(Font* font, char character);
float getTextWidth(Font* font, const char* text);
void freeFont(Font* font);

void initFPS();
void updateFPS();
void drawFPS(Font* font, float x, float y, Color color);



#endif // FONT_H
