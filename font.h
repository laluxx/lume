#ifndef FONT_H
#define FONT_H

#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "renderer.h"
#include <stdint.h>

#define MAX_PATH 4096
#define MAX_CHARACTERS 4096
#define ASCII 128
#define UNICODE_CACHE_SIZE 1024 * 200  // SO bad

// TODO Hashmap for nerdIcons!

typedef enum {
    CUSTOM = 0,
    WICON,      FAICON,    FLICON,
    MDICON,     CODICON,   DEVICON,
    IPSICON,    OCTICON,   POMICON,
    SUCICON,    POWERLINE, ALL
} NerdFontCategory;

typedef struct {
    uint32_t codepoint;
    NerdFontCategory category;
    const char* name;
    // Glyph metrics
    float ax, ay;  // advance
    float bw, bh;  // bitmap width/height
    float bl, bt;  // bitmap left/top
    float tx, ty;  // texture coordinates
} NerdIcon;

typedef struct {
    GLuint texture_atlas;       // OpenGL texture ID
    unsigned char* atlas_data;  // CPU-side texture data
    unsigned int width;         // Atlas width
    unsigned int height;        // Atlas height
    
    struct {
        int x;          // Current x position in atlas
        int y;          // Current y position in atlas
        int row_height; // Height of current row
    } packer;
    
    // Font metrics
    int ascent;
    int descent;
    char *path;
    char *name;
    
    // Glyph storage
    NerdIcon* icons;
    size_t icon_count;
    size_t icon_capacity;
} NerdFont;




NerdFont* loadNerdFont(const char* fontPath, int fontSize);
void freeNerdFont(NerdFont* nf);
NerdIcon *findNerdIcon(NerdFont *nf, uint32_t codepoint);
float getNerdIconAdvance(NerdFont *nf, uint32_t codepoint);
float getNerdTextWidth(NerdFont *nf, const char *text);
void drawNerdText(NerdFont *nf, const char *text, float x, float y, Color color);
void drawNerdTextEx(NerdFont *nf, const char *text, float x, float y, float sx,
                    float sy, Color color, const char *shader);
bool initNerdFontAtlas(NerdFont* nf, int size);


bool loadNerdIcons(NerdFont* nf, NerdFontCategory category);
void drawNerdIcon(NerdFont* nf, uint32_t codepoint, float x, float y, float scale, Color color);
/* void drawNerdIcon(NerdFont* nf, uint32_t codepoint, float x, float y, float scale, Color color); */
bool loadNerdIcon(NerdFont* nf, uint32_t codepoint, NerdFontCategory category, const char* name);
void drawNerdIconByName(NerdFont* nf, const char* name, float x, float y, float scale, Color color);
float getNerdIconAdvance(NerdFont* nf, uint32_t codepoint);
void drawNerdText(NerdFont* nf, const char* text, float x, float y, Color color);

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


typedef struct {
    Character ascii[ASCII];
    // Unicode character cache TODO it should be an hash table
    struct {
        uint32_t codepoint;
        Character data;
    } unicode[UNICODE_CACHE_SIZE];
    int unicode_count;
    
    GLuint textureID;
    unsigned int width;
    unsigned int height;
    int ascent;
    int descent;
    char *path;
    char *name;
    size_t tab;

    // CPU Atlat
    unsigned char *atlasData;
    int current_ox;
    int current_oy;
    int current_rowh;
} Font;

void initFreeType();
Font *loadFont(const char *fontPath, int fontSize, char *fontName, size_t tab);
Font* loadFontWithUnicode(const char* fontPath, int fontSize, const uint32_t* additionalCodepoints, size_t numCodepoints);

void drawText(Font* font, const char* text, float x, float y, Color color);

void drawTextEx(Font* font, const char* text,
                float x, float y,
                float sx, float sy,
                Color textColor, Color highlightColor,
                int highlightPos, bool cursorVisible,
                char *shader);

float getFontHeight(Font* font);
float getFontWidth(Font* font);
float getCharacterWidth(Font* font, char character);
float getTextWidth(Font* font, const char* text);
void freeFont(Font* font);

void initFPS();
void updateFPS();
void drawFPS(Font* font, float x, float y, Color color);

char* getFontPath(const char* fontName);
float getTabWidth(Font* font, size_t tab);


const char* utf8_to_codepoint(const char* p, uint32_t* dst);
bool loadUnicodeGlyph(Font* font, uint32_t codepoint);
Character* findUnicodeCharacter(Font* font, uint32_t codepoint);
void drawUnicodeText(Font* font, const char* text, float x, float y, Color color);

void drawChar(Font* font, char character, float x, float y, float sx, float sy, Color color);

void drawUnicodeChar(Font* font, Character* ch,
                     float x, float y,
                     float sx, float sy,
                     Color color);


#endif // FONT_H
