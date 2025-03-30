#include "font.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fontconfig/fontconfig.h>

// TODO loadFontEx() with codepoints 

FT_Library ft;

void initFreeType() {
    if (FT_Init_FreeType(&ft)) {
        fprintf(stderr, "Could not init FreeType Library\n");
        exit(1);
    }
    FT_Library_SetLcdFilter(ft, FT_LCD_FILTER_DEFAULT);
}

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

bool shouldSaveAtlas = false;


// NOTE LCD font rendering
/* Font* loadFont(const char* fontPath, int fontSize) { */
/*     FT_Face face; */
/*     if (FT_New_Face(ft, fontPath, 0, &face)) { */
/*         fprintf(stderr, "Failed to load font: %s\n", fontPath); */
/*         return NULL; */
/*     } */

/*     printf("[LOADED FONT] %s %i\n", fontPath, fontSize); */
/*     FT_Set_Pixel_Sizes(face, 0, fontSize); */
/*     FT_Library_SetLcdFilter(ft, FT_LCD_FILTER_DEFAULT); */

/*     Font* font = (Font*)malloc(sizeof(Font)); */
/*     if (!font) { */
/*         FT_Done_Face(face); */
/*         fprintf(stderr, "Memory allocation failed for font structure\n"); */
/*         return NULL; */
/*     } */

/*     font->ascent = face->size->metrics.ascender >> 6; */
/*     font->descent = -(face->size->metrics.descender >> 6); */

/*     unsigned char* atlas = (unsigned char*)calloc(1024 * 1024 * 3, sizeof(unsigned char)); */
/*     if (!atlas) { */
/*         free(font); */
/*         FT_Done_Face(face); */
/*         fprintf(stderr, "Memory allocation failed for texture atlas\n"); */
/*         return NULL; */
/*     } */

/*     int ox = 0, oy = 0, rowh = 0; */
/*     for (int i = 32; i < 128; i++) { */
/*         if (FT_Load_Char(face, i, FT_LOAD_RENDER | FT_LOAD_TARGET_LCD)) { */
/*             fprintf(stderr, "Loading character %c failed!\n", i); */
/*             continue; */
/*         } */

/*         if (ox + (face->glyph->bitmap.width / 3) + 1 >= 1024) { */
/*             oy += rowh; */
/*             rowh = 0; */
/*             ox = 0; */
/*         } */

/*         for (int y = 0; y < face->glyph->bitmap.rows; y++) { */
/*             for (int x = 0; x < face->glyph->bitmap.width; x++) { */
/*                 int atlas_index = (ox + (x / 3)) + ((oy + y) * 1024); */
/*                 int buffer_index = (face->glyph->bitmap.rows - 1 - y) * face->glyph->bitmap.pitch + x; // Adjust for vertical flip */
/*                 atlas[3 * atlas_index + (x % 3)] = face->glyph->bitmap.buffer[buffer_index]; */
/*             } */
/*         } */

/*         font->characters[i].ax = face->glyph->advance.x >> 6; */
/*         font->characters[i].ay = face->glyph->advance.y >> 6; */
/*         font->characters[i].bw = face->glyph->bitmap.width / 3; */
/*         font->characters[i].bh = face->glyph->bitmap.rows; */
/*         font->characters[i].bl = face->glyph->bitmap_left; */
/*         font->characters[i].bt = face->glyph->bitmap_top; */
/*         font->characters[i].tx = (float)ox / 1024; */
/*         font->characters[i].ty = (float)oy / 1024; */

/*         ox += (face->glyph->bitmap.width / 3) + 1; */
/*         rowh = fmax(rowh, face->glyph->bitmap.rows); */
/*     } */

/*     glGenTextures(1, &font->textureID); */
/*     glBindTexture(GL_TEXTURE_2D, font->textureID); */
/*     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1024, 1024, 0, GL_RGB, GL_UNSIGNED_BYTE, atlas); */
/*     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); */
/*     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); */
/*     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); */
/*     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); */

/*     if (shouldSaveAtlas) { */
/*         stbi_write_png("font_atlas.png", 1024, 1024, 3, atlas, 1024 * 3); */
/*     } */

/*     free(atlas); */
/*     FT_Done_Face(face); */
/*     return font; */
/* } */

// DYNAMIC ATLAS SIZE FIX WITH PIXEL INVERSION
Font* loadFont(const char *fontPath, int fontSize, char *fontName, size_t tab) {
    FT_Face face;
    if (FT_New_Face(ft, fontPath, 0, &face)) {
        fprintf(stderr, "Failed to load font: %s\n", fontPath);
        return NULL;
    }

    printf("[LOADED FONT] %s %i\n", fontPath, fontSize);

    FT_Set_Pixel_Sizes(face, 0, fontSize);

    Font* font = (Font*)malloc(sizeof(Font));
    if (!font) {
        FT_Done_Face(face);
        fprintf(stderr, "Memory allocation failed for font structure\n");
        return NULL;
    }

    font->path = strdup(fontPath);
    font->name = strdup(fontName);
    font->tab  = tab;
    

    // Save ascent and descent, converting from FreeType 26.6 fixed-point format
    font->ascent = face->size->metrics.ascender >> 6;
    font->descent = -(face->size->metrics.descender >> 6); // Make descent positive

    // Dynamic atlas size, starting with a small size and expanding if needed
    int atlasSize = 512;
    bool atlasSizeTooSmall = false;
    unsigned char* atlas = NULL;

    do {
        if (atlas) {
            free(atlas);  // Free previous atlas if resizing
        }

        atlasSizeTooSmall = false;
        atlas = (unsigned char*)calloc(atlasSize * atlasSize, sizeof(unsigned char));
        if (!atlas) {
            free(font);
            FT_Done_Face(face);
            fprintf(stderr, "Memory allocation failed for texture atlas\n");
            return NULL;
        }

        int ox = 0, oy = 0, rowh = 0;

        // Load each character from ASCII 32 to 127
        for (int i = 32; i < MAX_CHARACTERS; i++) {
            if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
                fprintf(stderr, "Loading character %c failed!\n", i);
                continue;
            }

            // Check if we need to move to the next row
            if (ox + face->glyph->bitmap.width + 1 >= atlasSize) {
                oy += rowh;
                rowh = 0;
                ox = 0;
            }

            // If the glyph height exceeds the atlas size, enlarge the atlas
            if (oy + face->glyph->bitmap.rows >= atlasSize) {
                atlasSizeTooSmall = true;
                atlasSize *= 2;  // Double the atlas size and retry
                break;
            }

            // Copy the glyph bitmap into the atlas with vertical flipping
            for (int y = 0; y < face->glyph->bitmap.rows; y++) {
                for (int x = 0; x < face->glyph->bitmap.width; x++) {
                    int atlas_y = oy + (face->glyph->bitmap.rows - 1 - y); // Flip vertically
                    atlas[(ox + x) + (atlas_y * atlasSize)] = face->glyph->bitmap.buffer[x + y * face->glyph->bitmap.pitch];
                }
            }

            // Store glyph data in the font structure
            font->characters[i].ax = face->glyph->advance.x >> 6;
            font->characters[i].ay = face->glyph->advance.y >> 6;
            font->characters[i].bw = face->glyph->bitmap.width;
            font->characters[i].bh = face->glyph->bitmap.rows;
            font->characters[i].bl = face->glyph->bitmap_left;
            font->characters[i].bt = face->glyph->bitmap_top;

            // Store texture coordinates
            font->characters[i].tx = (float)ox / atlasSize;
            font->characters[i].ty = (float)oy / atlasSize;

            rowh = fmax(rowh, face->glyph->bitmap.rows);
            ox += face->glyph->bitmap.width + 1;  // Move to the next glyph position
        }

    } while (atlasSizeTooSmall);  // Retry with a larger atlas size if necessary

    // Upload the atlas texture to the GPU
    glGenTextures(1, &font->textureID);
    glBindTexture(GL_TEXTURE_2D, font->textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlasSize, atlasSize, 0, GL_RED, GL_UNSIGNED_BYTE, atlas);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Save the atlas texture size in the font structure
    font->width = atlasSize;
    font->height = atlasSize;

    if (shouldSaveAtlas) {
        stbi_write_png("font_atlas.png", atlasSize, atlasSize, 1, atlas, atlasSize);
    }

    free(atlas);
    FT_Done_Face(face);
    return font;
}


void drawTextEx(Font* font, const char* text, float x, float y, float sx, float sy,
                Color textColor, Color highlightColor, int highlightPos, bool cursorVisible, char *shader) {

    const char *p;
    int charPos = 0;  // Position index of the character in the string
    float initialX = x;  // Save the starting x coordinate to reset on new lines
    float lineHeight = (font->ascent + font->descent) * sy;  // Adjust to include ascent and descent

    /* useShader("text"); */
    useShader(shader);
    glBindTexture(GL_TEXTURE_2D, font->textureID);
    glActiveTexture(GL_TEXTURE0);

    for (p = text; *p; p++, charPos++) {
        if (*p == '\n') {
            x = initialX;
            y -= lineHeight;
            continue;  // Handle new lines
        }

        Character ch = font->characters[*p];

        // Correct the y position to align the baseline of the font
        GLfloat xpos = x + ch.bl * sx;
        GLfloat ypos = y - (ch.bh - ch.bt + font->descent) * sy;

        GLfloat w = ch.bw * sx;
        GLfloat h = ch.bh * sy;

        // Compute UV coordinates dynamically based on the actual texture size
        GLfloat atlasWidth = (float)font->width;
        GLfloat atlasHeight = (float)font->height;

        Vec2f uv1 = {ch.tx, ch.ty + ch.bh / atlasHeight};
        Vec2f uv2 = {ch.tx, ch.ty};
        Vec2f uv3 = {ch.tx + ch.bw / atlasWidth, ch.ty};
        Vec2f uv4 = {ch.tx + ch.bw / atlasWidth, ch.ty + ch.bh / atlasHeight};

        // Determine color based on highlight position and cursor visibility
        Color currentColor = (highlightPos >= 0 && charPos == highlightPos && cursorVisible) ? highlightColor : textColor;

        // Draw each character with the appropriate color
        drawTriangleEx((Vec2f){xpos, ypos + h}, currentColor, uv1,
                           (Vec2f){xpos, ypos}, currentColor, uv2,
                           (Vec2f){xpos + w, ypos}, currentColor, uv3);

        drawTriangleEx((Vec2f){xpos, ypos + h}, currentColor, uv1,
                           (Vec2f){xpos + w, ypos}, currentColor, uv3,
                           (Vec2f){xpos + w, ypos + h}, currentColor, uv4);

        // Advance cursor to next glyph position
        x += ch.ax * sx;
    }
    flush();
}


void drawText(Font* font, const char* text, float x, float y, Color textColor) {
    useShader("text");
    float sx = 1.0;
    float sy = 1.0;
    drawTextEx(font, text, x, y, sx, sy, textColor, BLACK, -1, false, "text");
    flush();
}


void drawChar(Font* font, char character, float x, float y, float sx, float sy, Color color) {
    if (character == '\t') {
        // Tab character is handled as cursor positioning only, nothing to draw
        return; // Simply return as the tab character itself isn't rendered
    }

    if (character < 0 || character >= MAX_CHARACTERS) {
        fprintf(stderr, "Character out of supported range: %d.\n", character);
        return;
    }

    Character ch = font->characters[character];

    // Correct the y position to align the baseline of the font
    GLfloat xpos = x + ch.bl * sx;
    GLfloat ypos = y - (ch.bh - ch.bt + font->descent) * sy;

    GLfloat w = ch.bw * sx;
    GLfloat h = ch.bh * sy;

    // Compute UV coordinates dynamically based on the actual texture size
    GLfloat atlasWidth = (float)font->width;
    GLfloat atlasHeight = (float)font->height;

    Vec2f uv1 = {ch.tx, ch.ty};
    Vec2f uv2 = {ch.tx, ch.ty + (ch.bh / atlasHeight)};
    Vec2f uv3 = {ch.tx + (ch.bw / atlasWidth), ch.ty + (ch.bh / atlasHeight)};
    Vec2f uv4 = {ch.tx + (ch.bw / atlasWidth), ch.ty};

    glBindTexture(GL_TEXTURE_2D, font->textureID);
    glActiveTexture(GL_TEXTURE0);

    drawTriangleEx((Vec2f){xpos, ypos}, color, uv1,
                   (Vec2f){xpos, ypos + h}, color, uv2,
                   (Vec2f){xpos + w, ypos + h}, color, uv3);

    drawTriangleEx((Vec2f){xpos, ypos}, color, uv1,
                   (Vec2f){xpos + w, ypos + h}, color, uv3,
                   (Vec2f){xpos + w, ypos}, color, uv4);
}



// NOTE we dont set the shader to allow the user to set whatever shader he want and batch render
/* void drawChar(Font* font, char character, float x, float y, float sx, float sy, Color color) { */
/*     if (character < 0 || character >= MAX_CHARACTERS) { */
/*         fprintf(stderr, "Character out of supported range: %d.\n", character); */
/*         return; */
/*     } */

/*     Character ch = font->characters[character]; */

/*     // Correct the y position to align the baseline of the font */
/*     GLfloat xpos = x + ch.bl * sx; */
/*     GLfloat ypos = y - (ch.bh - ch.bt + font->descent) * sy; */

/*     GLfloat w = ch.bw * sx; */
/*     GLfloat h = ch.bh * sy; */

/*     // Compute UV coordinates dynamically based on the actual texture size */
/*     GLfloat atlasWidth = (float)font->width; */
/*     GLfloat atlasHeight = (float)font->height; */

/*     Vec2f uv1 = {ch.tx, ch.ty}; */
/*     Vec2f uv2 = {ch.tx, ch.ty + (ch.bh / atlasHeight)}; */
/*     Vec2f uv3 = {ch.tx + (ch.bw / atlasWidth), ch.ty + (ch.bh / atlasHeight)}; */
/*     Vec2f uv4 = {ch.tx + (ch.bw / atlasWidth), ch.ty}; */

/*     glBindTexture(GL_TEXTURE_2D, font->textureID); */
/*     glActiveTexture(GL_TEXTURE0); */

/*     drawTriangleEx((Vec2f){xpos, ypos}, color, uv1, */
/*                        (Vec2f){xpos, ypos + h}, color, uv2, */
/*                        (Vec2f){xpos + w, ypos + h}, color, uv3); */

/*     drawTriangleEx((Vec2f){xpos, ypos}, color, uv1, */
/*                        (Vec2f){xpos + w, ypos + h}, color, uv3, */
/*                        (Vec2f){xpos + w, ypos}, color, uv4); */
/* } */

void freeFont(Font* font) {
    glDeleteTextures(1, &font->textureID);
    free(font);
}


float getFontHeight(Font* font) {
    float max_height = 0;
    for (int i = 0; i < MAX_CHARACTERS; i++) {
        if (font->characters[i].bh > max_height) {
            max_height = font->characters[i].bh;
        }
    }
    return max_height;
}

float getFontWidth(Font* font) {
    return font->characters[' '].ax;
}

float getTabWidth(Font *font, size_t tab) {
  return font->characters[' '].ax * tab;
}

float getCharacterWidth(Font* font, char character) {
    if (character < 0 || character >= MAX_CHARACTERS) {
        fprintf(stderr, "Character out of supported range.\n");
        return 0;
    }
    return font->characters[character].ax;
}

float getTextWidth(Font *font, const char *text) {
    float width = 0.0f;
    for (int i = 0; text[i] != '\0'; i++) {
        char c = text[i];
        width += getCharacterWidth(font, c);
    }
    return width;
}

/* float getTextWidth(Font* font, const char* text) { */
/*     float width = 0.0f; */
/*     for (int i = 0; text[i] != '\0'; i++) { */
/*         char c = text[i]; */
/*         if (c >= 0 && c < MAX_CHARACTERS) { */
/*             width += getCharacterWidth(font, c); */
/*         } */
/*     } */
/*     return width; */
/* } */


static double lastTime = 0.0;
static int frameCount = 0;
static char fpsText[256];

void initFPS() {
    lastTime = glfwGetTime();
    frameCount = 0;
    sprintf(fpsText, "FPS: %d", 0);
}

void updateFPS() {
    double currentTime = glfwGetTime();
    frameCount++;
    if (currentTime - lastTime >= 1.0) { // If a second has passed
        // Display the frame count here any way you like or simply store it
        sprintf(fpsText, "FPS: %d", frameCount);

        frameCount = 0;
        lastTime += 1.0;
    }
}


void drawFPS(Font* font, float x, float y, Color color) {
    updateFPS();
    drawText(font, fpsText, x, y, color);
}

char* getFontPath(const char* fontSpec) {
    if (!FcInit()) {
        return NULL;
    }

    // Create a pattern from the specification
    FcPattern* pattern = FcNameParse((const FcChar8*)fontSpec);
    if (!pattern) {
        return NULL;
    }

    // Configure the pattern with default values
    FcConfigSubstitute(NULL, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

    // Find the best match
    FcResult result;
    FcPattern* matched = FcFontMatch(NULL, pattern, &result);
    FcPatternDestroy(pattern);

    if (!matched) {
        return NULL;
    }

    // Get the file path
    FcChar8* file = NULL;
    if (FcPatternGetString(matched, FC_FILE, 0, &file) != FcResultMatch) {
        FcPatternDestroy(matched);
        return NULL;
    }

    // Create a copy of the string since the pattern will be destroyed
    char* filepath = strdup((char*)file);
    FcPatternDestroy(matched);

    return filepath;
}

