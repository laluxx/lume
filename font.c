#include "font.h"
#include "renderer.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fontconfig/fontconfig.h>

// TODO Keep an atlas just for codepoints, and another just for nerdicons,
// don't mix shit and the be surprised shitslow; Also look into atlas packing algorithms.

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

/*         font->ascii[i].ax = face->glyph->advance.x >> 6; */
/*         font->ascii[i].ay = face->glyph->advance.y >> 6; */
/*         font->ascii[i].bw = face->glyph->bitmap.width / 3; */
/*         font->ascii[i].bh = face->glyph->bitmap.rows; */
/*         font->ascii[i].bl = face->glyph->bitmap_left; */
/*         font->ascii[i].bt = face->glyph->bitmap_top; */
/*         font->ascii[i].tx = (float)ox / 1024; */
/*         font->ascii[i].ty = (float)oy / 1024; */

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



Font* loadFont(const char *fontPath, int fontSize, char *fontName, size_t tab) {
    // Initialize FreeType face
    FT_Face face;
    if (FT_New_Face(ft, fontPath, 0, &face)) {
        fprintf(stderr, "Failed to load font: %s\n", fontPath);
        return NULL;
    }

    printf("[LOADED FONT] %s %i\n", fontPath, fontSize);
    FT_Set_Pixel_Sizes(face, 0, fontSize);

    // Allocate font structure
    Font* font = (Font*)calloc(1, sizeof(Font));
    if (!font) {
        FT_Done_Face(face);
        fprintf(stderr, "Memory allocation failed for font structure\n");
        return NULL;
    }

    // Store font metadata
    font->path = strdup(fontPath);
    font->name = strdup(fontName);
    font->tab = tab;
    font->ascent = face->size->metrics.ascender >> 6;
    font->descent = -(face->size->metrics.descender >> 6);

    // Determine required atlas size
    int atlasSize = 512;
    unsigned char* atlas = NULL;
    bool atlasCreated = false;

    // Try creating atlas until we find a large enough size
    while (!atlasCreated) {
        // Free previous attempt if we're retrying
        if (atlas) free(atlas);

        atlas = (unsigned char*)calloc(atlasSize * atlasSize, sizeof(unsigned char));
        if (!atlas) {
            free(font);
            FT_Done_Face(face);
            fprintf(stderr, "Memory allocation failed for texture atlas\n");
            return NULL;
        }

        int ox = 0, oy = 0, rowh = 0;
        bool fits = true;

        // Try packing ASCII characters
        for (int i = 32; i < MAX_CHARACTERS && fits; i++) {
            if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
                fprintf(stderr, "Loading character %c failed!\n", i);
                continue;
            }

            FT_GlyphSlot glyph = face->glyph;
            int width = glyph->bitmap.width;
            int height = glyph->bitmap.rows;

            // Check if we need to move to next row
            if (ox + width + 1 >= atlasSize) {
                oy += rowh;
                rowh = 0;
                ox = 0;
            }

            // Check if glyph fits in current atlas
            if (oy + height >= atlasSize) {
                fits = false;
                break;
            }

            // Copy glyph to atlas with vertical flip
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    int dstY = oy + (height - 1 - y); // Flip vertically
                    atlas[(ox + x) + (dstY * atlasSize)] =
                        glyph->bitmap.buffer[x + y * glyph->bitmap.pitch];
                }
            }

            // Store character metrics
            font->ascii[i].ax = glyph->advance.x >> 6;
            font->ascii[i].ay = glyph->advance.y >> 6;
            font->ascii[i].bw = width;
            font->ascii[i].bh = height;
            font->ascii[i].bl = glyph->bitmap_left;
            font->ascii[i].bt = glyph->bitmap_top;
            font->ascii[i].tx = (float)ox / atlasSize;
            font->ascii[i].ty = (float)oy / atlasSize;

            // Update packing positions
            ox += width + 1;
            rowh = fmax(rowh, height);
        }

        if (fits) {
            atlasCreated = true;
            // Save packing state for future Unicode icons
            font->current_ox = ox;
            font->current_oy = oy;
            font->current_rowh = rowh;
        } else {
            // Try again with larger atlas
            atlasSize *= 2;
            if (atlasSize > 8192) { // Safety limit
                fprintf(stderr, "Font requires too large texture atlas\n");
                free(atlas);
                free(font);
                FT_Done_Face(face);
                return NULL;
            }
        }
    }

    // Create OpenGL texture
    glGenTextures(1, &font->textureID);
    glBindTexture(GL_TEXTURE_2D, font->textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlasSize, atlasSize, 0,
                 GL_RED, GL_UNSIGNED_BYTE, atlas);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Store atlas properties
    font->width = atlasSize;
    font->height = atlasSize;
    font->atlasData = atlas; // Keep CPU copy for adding Unicode icons later

    // Save atlas to file if requested
    if (shouldSaveAtlas) {
        // Create vertically flipped version for saving
        unsigned char* flipped = (unsigned char*)malloc(atlasSize * atlasSize);
        for (int y = 0; y < atlasSize; y++) {
            memcpy(flipped + y * atlasSize,
                   atlas + (atlasSize - 1 - y) * atlasSize,
                   atlasSize);
        }
        stbi_write_png("font_atlas.png", atlasSize, atlasSize, 1, flipped, atlasSize);
        free(flipped);
    }

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

        Character ch = font->ascii[*p];

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

// NOTE we dont set the shader to allow the user to set whatever shader he want and batch render
void drawChar(Font* font, char character, float x, float y, float sx, float sy, Color color) {
    if (character == '\t') {
        // Tab character is handled as cursor positioning only, nothing to draw
        return; // Simply return as the tab character itself isn't rendered
    }

    if ((unsigned char)character < 32 || (unsigned char)character >= ASCII) {
        fprintf(stderr, "Character out of supported range: %d.\n", character);
        return;
    }

    Character ch = font->ascii[character];

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

void freeFont(Font* font) {
    glDeleteTextures(1, &font->textureID);
    free(font->atlasData);
    free(font->path);
    free(font->name);
    free(font);
}


float getFontHeight(Font* font) {
    float max_height = 0;
    for (int i = 0; i < ASCII; i++) {
        if (font->ascii[i].bh > max_height) {
            max_height = font->ascii[i].bh;
        }
    }
    return max_height;
}

float getFontWidth(Font* font) {
    return font->ascii[' '].ax;
}

float getTabWidth(Font *font, size_t tab) {
  return font->ascii[' '].ax * tab;
}

#define FPRINTF_WITH_LINE(stream, ...) fprintf_with_line(stream, __FILE__, __LINE__, __VA_ARGS__)

void fprintf_with_line(FILE* stream, const char* file, int line, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    // Print file and line information first
    fprintf(stream, "[%s:%d] ", file, line);
    
    // Then print the original message
    vfprintf(stream, format, args);
    
    va_end(args);
}


float getCharacterWidth(Font* font, char character) {
    if (character == '\t') {
        return font->ascii[' '].ax * font->tab;  // Tab width = N spaces
    }
    if ((unsigned char)character < 32) {
        return 0.0f;  // Other control chars (like \n) have no width
    }
    if ((unsigned char)character >= ASCII) {
        FPRINTF_WITH_LINE(stderr, "[LUME] getCharacterWidth :: Character out of supported range.\n");
        return 0.0f;
    }
    return font->ascii[character].ax;
}

/* float getCharacterWidth(Font* font, char character) { */
/*     if ((unsigned char)character < 32 || (unsigned char)character >= ASCII) { */
/*         /\* fprintf(stderr, "[LUME] getCharacterWidth :: Character out of supported range.\n"); *\/ */


/*         FPRINTF_WITH_LINE(stderr, "[LUME] getCharacterWidth :: Character out of supported range.\n"); */
/*         return 0; */
/*     } */
/*     return font->ascii[character].ax; */
/* } */

float getTextWidth(Font *font, const char *text) {
    float width = 0.0f;
    for (int i = 0; text[i] != '\0'; i++) {
        char c = text[i];
        width += getCharacterWidth(font, c);
    }
    return width;
}


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


bool loadUnicodeGlyph(Font* font, uint32_t codepoint) {
    // Check if glyph already exists
    for (int i = 0; i < font->unicode_count; i++) {
        if (font->unicode[i].codepoint == codepoint) {
            return true;
        }
    }

    FT_Face face;
    if (FT_New_Face(ft, font->path, 0, &face)) {
        fprintf(stderr, "Failed to load font face for U+%04X\n", codepoint);
        return false;
    }

    // Use same pixel size as ASCII icons
    FT_Set_Pixel_Sizes(face, 0, font->ascent + font->descent);

    if (FT_Load_Char(face, codepoint, FT_LOAD_RENDER)) {
        fprintf(stderr, "Failed to load glyph for U+%04X\n", codepoint);
        FT_Done_Face(face);
        return false;
    }

    int width = face->glyph->bitmap.width;
    int height = face->glyph->bitmap.rows;

    // Check if we need to move to next row
    if (font->current_ox + width + 1 >= font->width) {
        font->current_oy += font->current_rowh + 1; // Add 1px padding
        font->current_ox = 0;
        font->current_rowh = 0;
    }

    // Check if atlas is full
    if (font->current_oy + height >= font->height) {
        fprintf(stderr, "Atlas full (U+%04X needs %dx%d, available %dx%d)\n",
                codepoint, width, height, 
                font->width - font->current_ox, 
                font->height - font->current_oy);
        FT_Done_Face(face);
        return false;
    }

    // Clear target area
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int dstY = font->current_oy + y;
            font->atlasData[(font->current_ox + x) + (dstY * font->width)] = 0;
        }
    }

    // Copy glyph data (vertically flipped)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int dstY = font->current_oy + (height - 1 - y);
            font->atlasData[(font->current_ox + x) + (dstY * font->width)] =
                face->glyph->bitmap.buffer[x + y * face->glyph->bitmap.pitch];
        }
    }

    // Store character metrics (consistent with ASCII icons)
    Character ch = {
        .ax = face->glyph->advance.x >> 6,
        .ay = face->glyph->advance.y >> 6,
        .bw = width,
        .bh = height,
        .bl = face->glyph->bitmap_left,
        .bt = face->glyph->bitmap_top,
        .tx = (float)font->current_ox / font->width,
        .ty = (float)font->current_oy / font->height
    };

    // Add to cache
    if (font->unicode_count < UNICODE_CACHE_SIZE) {
        font->unicode[font->unicode_count].codepoint = codepoint;
        font->unicode[font->unicode_count].data = ch;
        font->unicode_count++;
    } else {
        fprintf(stderr, "Unicode cache full (U+%04X not cached)\n", codepoint);
    }

    // Update texture - upload entire atlas to prevent corruption
    glBindTexture(GL_TEXTURE_2D, font->textureID);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 
                   font->width, font->height,
                   GL_RED, GL_UNSIGNED_BYTE,
                   font->atlasData);

    // Save atlas to file if enabled
    if (shouldSaveAtlas) {
        static int saveCount = 0;
        char filename[256];
        sprintf(filename, "font_atlas_%d.png", saveCount++);
        
        // Create a flipped version for saving (since OpenGL expects origin at bottom)
        unsigned char* flipped = (unsigned char*)malloc(font->width * font->height);
        for (int y = 0; y < font->height; y++) {
            memcpy(flipped + y * font->width,
                   font->atlasData + (font->height - 1 - y) * font->width,
                   font->width);
        }
        
        stbi_write_png(filename, font->width, font->height, 1, flipped, font->width);
        free(flipped);
        
        printf("Saved atlas to %s\n", filename);
    }

    // Update packing positions
    font->current_ox += width + 1; // Add 1px padding between icons
    font->current_rowh = fmax(font->current_rowh, height);

    FT_Done_Face(face);
    return true;
}

const char* utf8_to_codepoint(const char* p, uint32_t* dst) {
    uint32_t res = 0;
    
    if ((*p & 0x80) == 0) {
        // 1-byte sequence (0xxxxxxx)
        *dst = *p;
        return p + 1;
    } 
    else if ((*p & 0xE0) == 0xC0) {
        // 2-byte sequence (110xxxxx 10xxxxxx)
        res = (*p & 0x1F) << 6;
        res |= (*(p+1) & 0x3F);
        *dst = res;
        return p + 2;
    } 
    else if ((*p & 0xF0) == 0xE0) {
        // 3-byte sequence (1110xxxx 10xxxxxx 10xxxxxx)
        res = (*p & 0x0F) << 12;
        res |= (*(p+1) & 0x3F) << 6;
        res |= (*(p+2) & 0x3F);
        *dst = res;
        return p + 3;
    } 
    else if ((*p & 0xF8) == 0xF0) {
        // 4-byte sequence (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
        res = (*p & 0x07) << 18;
        res |= (*(p+1) & 0x3F) << 12;
        res |= (*(p+2) & 0x3F) << 6;
        res |= (*(p+3) & 0x3F);
        *dst = res;
        return p + 4;
    }
    
    // Invalid UTF-8, return the next byte
    *dst = *p;
    return p + 1;
}

Character* findUnicodeCharacter(Font* font, uint32_t codepoint) {
    for (int i = 0; i < font->unicode_count; i++) {
        if (font->unicode[i].codepoint == codepoint) {
            return &font->unicode[i].data;
        }
    }
    return NULL;
}

// NOTE How we don't flush here either
void drawUnicodeChar(Font* font, Character* ch, float x, float y, float sx, float sy, Color color) {
    float xpos = x + ch->bl * sx;
    float ypos = y - (ch->bh - ch->bt + font->descent) * sy;
    
    float w = ch->bw * sx;
    float h = ch->bh * sy;

    // Flip texture coordinates
    float tx1 = ch->tx;
    float ty1 = ch->ty + ch->bh / (float)font->height; // Bottom in texture space
    float tx2 = ch->tx + ch->bw / (float)font->width;
    float ty2 = ch->ty; // Top in texture space

    drawTriangleEx(
        (Vec2f){xpos, ypos + h}, color, (Vec2f){tx1, ty2},
        (Vec2f){xpos, ypos}, color, (Vec2f){tx1, ty1},
        (Vec2f){xpos + w, ypos}, color, (Vec2f){tx2, ty1}
    );
    
    drawTriangleEx(
        (Vec2f){xpos, ypos + h}, color, (Vec2f){tx1, ty2},
        (Vec2f){xpos + w, ypos}, color, (Vec2f){tx2, ty1},
        (Vec2f){xpos + w, ypos + h}, color, (Vec2f){tx2, ty2}
    );
}

void drawUnicodeText(Font* font, const char* text, float x, float y, Color color) {
    const char* p = text;
    useShader(text);

    while (*p) {
        uint32_t codepoint;
        p = utf8_to_codepoint(p, &codepoint);
        
        if (codepoint < ASCII) {
            // Use existing ASCII character
            drawChar(font, (char)codepoint, x, y, 1.0f, 1.0f, color);
            x += font->ascii[codepoint].ax;
        } else {
            // Check unicode cache
            Character* ch = findUnicodeCharacter(font, codepoint);
            if (!ch) {
                // Try to load dynamically
                if (loadUnicodeGlyph(font, codepoint)) {
                    ch = findUnicodeCharacter(font, codepoint);
                }
            }
            
            if (ch) {
                drawUnicodeChar(font, ch, x, y, 1.0f, 1.0f, color);
                x += ch->ax;
            }
        }
    }

    flush();
}



// NERD ICONS

bool initNerdFontAtlas(NerdFont* nf, int size) {
    nf->width = size;
    nf->height = size;
    
    // Initialize packing state
    nf->packer.x = 1;  // Start with 1px padding
    nf->packer.y = 1;
    nf->packer.row_height = 0;
    
    // Allocate CPU-side atlas
    nf->atlas_data = calloc(size * size, 1);
    if (!nf->atlas_data) return false;
    
    // Create OpenGL texture
    glGenTextures(1, &nf->texture_atlas);
    glBindTexture(GL_TEXTURE_2D, nf->texture_atlas);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, size, size, 0,
                 GL_RED, GL_UNSIGNED_BYTE, nf->atlas_data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    return true;
}


/* bool initNerdFontAtlas(NerdFont* nf, int size) { */
/*     nf->width = size; */
/*     nf->height = size; */
    
/*     // Create CPU-side atlas */
/*     nf->atlas_data = calloc(size * size, 1); */
/*     if (!nf->atlas_data) return false; */
    
/*     // Create GPU texture */
/*     glGenTextures(1, &nf->texture_atlas); */
/*     glBindTexture(GL_TEXTURE_2D, nf->texture_atlas); */
/*     glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, size, size, 0, */
/*                 GL_RED, GL_UNSIGNED_BYTE, nf->atlas_data); */
/*     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); */
/*     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); */
/*     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); */
/*     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); */
    
/*     return true; */
/* } */

NerdFont* loadNerdFont(const char* fontPath, int fontSize) {
    NerdFont* nf = calloc(1, sizeof(NerdFont));
    if (!nf) return NULL;
    
    // Load FreeType face
    FT_Face face;
    if (FT_New_Face(ft, fontPath, 0, &face)) {
        free(nf);
        return NULL;
    }
    
    FT_Set_Pixel_Sizes(face, 0, fontSize);
    
    // Store basic metrics
    nf->ascent = face->size->metrics.ascender >> 6;
    nf->descent = -(face->size->metrics.descender >> 6);
    nf->path = strdup(fontPath);
    nf->name = strdup("NerdFont");
    
    // Initialize icon storage
    nf->icon_capacity = 256;
    nf->icons = malloc(nf->icon_capacity * sizeof(NerdIcon));
    if (!nf->icons) goto cleanup;
    
    // Initialize texture atlas (2048x2048 by default)
    if (!initNerdFontAtlas(nf, 2048)) goto cleanup;
    
    FT_Done_Face(face);
    return nf;

cleanup:
    if (face) FT_Done_Face(face);
    if (nf->path) free(nf->path);
    if (nf->name) free(nf->name);
    if (nf->icons) free(nf->icons);
    free(nf);
    return NULL;
}

/* NerdFont* loadNerdFont(const char* fontPath, int fontSize) { */
/*     NerdFont* nf = calloc(1, sizeof(NerdFont)); */
/*     if (!nf) return NULL; */
    
/*     // Initialize FreeType face */
/*     FT_Face face; */
/*     if (FT_New_Face(ft, fontPath, 0, &face)) { */
/*         free(nf); */
/*         return NULL; */
/*     } */
    
/*     FT_Set_Pixel_Sizes(face, 0, fontSize); */
    
/*     // Store basic font metrics */
/*     nf->ascent = face->size->metrics.ascender >> 6; */
/*     nf->descent = -(face->size->metrics.descender >> 6); */
/*     nf->path = strdup(fontPath); */
/*     nf->name = strdup("NerdFont"); */
    
/*     // Initialize icon storage */
/*     nf->icon_capacity = 256; */
/*     nf->icons = malloc(nf->icon_capacity * sizeof(NerdIcon)); */
/*     if (!nf->icons) { */
/*         FT_Done_Face(face); */
/*         free(nf->path); */
/*         free(nf->name); */
/*         free(nf); */
/*         return NULL; */
/*     } */
    
/*     // Create texture atlas (start with 2048x2048) */
/*     if (!initNerdFontAtlas(nf, 2048)) { */
/*         FT_Done_Face(face); */
/*         free(nf->icons); */
/*         free(nf->path); */
/*         free(nf->name); */
/*         free(nf); */
/*         return NULL; */
/*     } */
    
/*     FT_Done_Face(face); */
/*     return nf; */
/* } */

void freeNerdFont(NerdFont* nf) {
    if (!nf) return;
    
    if (nf->path) free(nf->path);
    if (nf->name) free(nf->name);
    
    for (size_t i = 0; i < nf->icon_count; i++) {
        if (nf->icons[i].name) free(nf->icons[i].name);
    }
    
    if (nf->icons) free(nf->icons);
    if (nf->atlas_data) free(nf->atlas_data);
    if (nf->texture_atlas) glDeleteTextures(1, &nf->texture_atlas);
    free(nf);
}

/* void freeNerdFont(NerdFont* nf) { */
/*     if (!nf) return; */
    
/*     if (nf->path) free(nf->path); */
/*     if (nf->name) free(nf->name); */
/*     if (nf->icons) free(nf->icons); */
/*     if (nf->atlas_data) free(nf->atlas_data); */
/*     if (nf->texture_atlas) glDeleteTextures(1, &nf->texture_atlas); */
/*     free(nf); */
/* } */



NerdIcon *findNerdIcon(NerdFont *nf, uint32_t codepoint) {
    for (size_t i = 0; i < nf->icon_count; i++) {
        if (nf->icons[i].codepoint == codepoint) {
            return &nf->icons[i];
        }
    }
    return NULL;
}

float getNerdIconAdvance(NerdFont* nf, uint32_t codepoint) {
    NerdIcon* icon = findNerdIcon(nf, codepoint);
    return icon ? icon->ax : 0.0f;
}

float getNerdTextWidth(NerdFont* nf, const char* text) {
    float width = 0.0f;
    const char* p = text;
    
    while (*p) {
        uint32_t codepoint;
        p = utf8_to_codepoint(p, &codepoint);
        width += getNerdIconAdvance(nf, codepoint);
    }
    
    return width;
}

void drawNerdText(NerdFont *nf, const char *text, float x, float y, Color color) {
    drawNerdTextEx(nf, text, x, y, 1.0f, 1.0f, color, "text");
}




void drawNerdTextEx(NerdFont* nf, const char* text, float x, float y,
                   float sx, float sy, Color color, const char* shader) {
    const char* p = text;
    float initial_x = x;
    float line_height = (nf->ascent + nf->descent) * sy;

    useShader(shader);
    glBindTexture(GL_TEXTURE_2D, nf->texture_atlas);

    while (*p) {
        if (*p == '\n') {
            x = initial_x;
            y -= line_height;
            p++;
            continue;
        }

        uint32_t codepoint;
        p = utf8_to_codepoint(p, &codepoint);

        NerdIcon* icon = findNerdIcon(nf, codepoint);
        if (!icon && !loadNerdIcon(nf, codepoint, CUSTOM, NULL)) {
            continue;
        }
        icon = findNerdIcon(nf, codepoint); // Reload in case we just loaded it

        float xpos = x + icon->bl * sx;
        float ypos = y - (icon->bh - icon->bt + nf->descent) * sy;
        float w = icon->bw * sx;
        float h = icon->bh * sy;

        float tx1 = icon->tx;
        float ty1 = icon->ty;
        float tx2 = icon->tx + icon->bw / (float)nf->width;
        float ty2 = icon->ty + icon->bh / (float)nf->height;

        // First triangle
        drawTriangleEx(
            (Vec2f){xpos, ypos}, color, (Vec2f){tx1, ty1},
            (Vec2f){xpos, ypos + h}, color, (Vec2f){tx1, ty2},
            (Vec2f){xpos + w, ypos + h}, color, (Vec2f){tx2, ty2}
        );
        
        // Second triangle
        drawTriangleEx(
            (Vec2f){xpos, ypos}, color, (Vec2f){tx1, ty1},
            (Vec2f){xpos + w, ypos + h}, color, (Vec2f){tx2, ty2},
            (Vec2f){xpos + w, ypos}, color, (Vec2f){tx2, ty1}
        );

        x += icon->ax * sx;
    }
    flush();
}

/* void drawNerdTextEx(NerdFont* nf, const char* text, float x, float y, */
/*                    float sx, float sy, Color color, const char* shader) { */
/*     const char* p = text; */
/*     float initial_x = x; */
/*     float line_height = (nf->ascent + nf->descent) * sy; */

/*     useShader(shader); */
/*     glBindTexture(GL_TEXTURE_2D, nf->texture_atlas); */
/*     glActiveTexture(GL_TEXTURE0); */

/*     while (*p) { */
/*         if (*p == '\n') { */
/*             x = initial_x; */
/*             y -= line_height; */
/*             p++; */
/*             continue; */
/*         } */

/*         uint32_t codepoint; */
/*         p = utf8_to_codepoint(p, &codepoint); */

/*         NerdIcon* icon = findNerdIcon(nf, codepoint); */
/*         if (!icon) { */
/*             // Try to load on demand */
/*             if (loadNerdIcon(nf, codepoint, CUSTOM, NULL)) { */
/*                 icon = findNerdIcon(nf, codepoint); */
/*             } */
/*         } */

/*         if (icon) { */
/*             float xpos = x + icon->bl * sx; */
/*             float ypos = y - (icon->bh - icon->bt + nf->descent) * sy; */
/*             float w = icon->bw * sx; */
/*             float h = icon->bh * sy; */

/*             // Texture coordinates */
/*             float tx1 = icon->tx; */
/*             float ty1 = icon->ty; */
/*             float tx2 = icon->tx + icon->bw / (float)nf->width; */
/*             float ty2 = icon->ty + icon->bh / (float)nf->height; */

/*             // Draw quad */
/*             drawTriangleEx( */
/*                 (Vec2f){xpos, ypos + h}, color, (Vec2f){tx1, ty2}, */
/*                 (Vec2f){xpos, ypos}, color, (Vec2f){tx1, ty1}, */
/*                 (Vec2f){xpos + w, ypos}, color, (Vec2f){tx2, ty1} */
/*             ); */
/*             drawTriangleEx( */
/*                 (Vec2f){xpos, ypos + h}, color, (Vec2f){tx1, ty2}, */
/*                 (Vec2f){xpos + w, ypos}, color, (Vec2f){tx2, ty1}, */
/*                 (Vec2f){xpos + w, ypos + h}, color, (Vec2f){tx2, ty2} */
/*             ); */

/*             x += icon->ax * sx; */
/*         } */
/*     } */
/*     flush(); */
/* } */

void drawNerdIconByName(NerdFont* nf, const char* name, float x, float y, float scale, Color color) {
    // This requires a name-to-codepoint mapping
    // You'll need to implement this based on your Nerd Font version
    // Here's a simplified version:
    
    // First check if we have an icon with this exact name
    for (size_t i = 0; i < nf->icon_count; i++) {
        if (nf->icons[i].name && strcmp(nf->icons[i].name, name) == 0) {
            drawNerdIcon(nf, nf->icons[i].codepoint, x, y, scale, color);
            return;
        }
    }
    
    // If not found, you might want to implement a more sophisticated lookup
    // For example, mapping "fa-git" to 0xF1D3, etc.
    fprintf(stderr, "Icon name '%s' not found\n", name);
}

bool loadNerdIcon(NerdFont* nf, uint32_t codepoint, NerdFontCategory category, const char* name) {
    // Check if already loaded
    for (size_t i = 0; i < nf->icon_count; i++) {
        if (nf->icons[i].codepoint == codepoint) {
            return true;
        }
    }
    
    FT_Face face;
    if (FT_New_Face(ft, nf->path, 0, &face)) return false;
    FT_Set_Pixel_Sizes(face, 0, nf->ascent + nf->descent);
    
    if (FT_Load_Char(face, codepoint, FT_LOAD_RENDER)) {
        FT_Done_Face(face);
        return false;
    }
    
    FT_GlyphSlot glyph = face->glyph;
    int width = glyph->bitmap.width;
    int height = glyph->bitmap.rows;
    
    // Check if we need to move to next row
    if (nf->packer.x + width + 1 > nf->width) {
        nf->packer.y += nf->packer.row_height + 1;
        nf->packer.x = 1;  // Reset with 1px padding
        nf->packer.row_height = 0;
    }
    
    // Check if atlas is full
    if (nf->packer.y + height > nf->height) {
        FT_Done_Face(face);
        return false;
    }
    
    // Copy glyph data (with vertical flip)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int dstY = nf->packer.y + (height - 1 - y);
            nf->atlas_data[(nf->packer.x + x) + (dstY * nf->width)] =
                glyph->bitmap.buffer[x + y * glyph->bitmap.pitch];
        }
    }
    
    // Update texture
    glBindTexture(GL_TEXTURE_2D, nf->texture_atlas);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, nf->width, nf->height,
                   GL_RED, GL_UNSIGNED_BYTE, nf->atlas_data);
    
    // Add to icon array
    if (nf->icon_count >= nf->icon_capacity) {
        size_t new_capacity = nf->icon_capacity * 2;
        NerdIcon* new_icons = realloc(nf->icons, new_capacity * sizeof(NerdIcon));
        if (!new_icons) {
            FT_Done_Face(face);
            return false;
        }
        nf->icons = new_icons;
        nf->icon_capacity = new_capacity;
    }
    
    nf->icons[nf->icon_count++] = (NerdIcon){
        .codepoint = codepoint,
        .category = category,
        .name = name ? strdup(name) : NULL,
        .ax = glyph->advance.x >> 6,
        .ay = glyph->advance.y >> 6,
        .bw = width,
        .bh = height,
        .bl = glyph->bitmap_left,
        .bt = glyph->bitmap_top,
        .tx = (float)nf->packer.x / nf->width,
        .ty = (float)nf->packer.y / nf->height
    };
    
    // Update packing positions
    nf->packer.x += width + 1;  // Add 1px padding between glyphs
    nf->packer.row_height = fmax(nf->packer.row_height, height);
    
    FT_Done_Face(face);
    return true;
}

/* bool loadNerdIcon(NerdFont* nf, uint32_t codepoint, NerdFontCategory category, const char* name) { */
/*     // Check if already loaded */
/*     for (size_t i = 0; i < nf->icon_count; i++) { */
/*         if (nf->icons[i].codepoint == codepoint) { */
/*             return true; */
/*         } */
/*     } */
    
/*     FT_Face face; */
/*     if (FT_New_Face(ft, nf->path, 0, &face)) { */
/*         return false; */
/*     } */
    
/*     FT_Set_Pixel_Sizes(face, 0, nf->ascent + nf->descent); */
    
/*     if (FT_Load_Char(face, codepoint, FT_LOAD_RENDER)) { */
/*         FT_Done_Face(face); */
/*         return false; */
/*     } */
    
/*     FT_GlyphSlot glyph = face->glyph; */
/*     int width = glyph->bitmap.width; */
/*     int height = glyph->bitmap.rows; */
    
/*     // Check if we need to move to next row */
/*     if (nf->current_ox + width + 1 >= nf->width) { */
/*         nf->current_oy += nf->current_rowh + 1; */
/*         nf->current_ox = 0; */
/*         nf->current_rowh = 0; */
/*     } */
    
/*     // Check if atlas is full */
/*     if (nf->current_oy + height >= nf->height) { */
/*         FT_Done_Face(face); */
/*         return false; */
/*     } */
    
/*     // Copy glyph data (vertically flipped) */
/*     for (int y = 0; y < height; y++) { */
/*         for (int x = 0; x < width; x++) { */
/*             int dstY = nf->current_oy + (height - 1 - y); */
/*             nf->atlas_data[(nf->current_ox + x) + (dstY * nf->width)] = */
/*                 glyph->bitmap.buffer[x + y * glyph->bitmap.pitch]; */
/*         } */
/*     } */
    
/*     // Update texture */
/*     glBindTexture(GL_TEXTURE_2D, nf->texture_atlas); */
/*     glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, nf->width, nf->height, */
/*                    GL_RED, GL_UNSIGNED_BYTE, nf->atlas_data); */
    
/*     // Add to icon array */
/*     if (nf->icon_count >= nf->icon_capacity) { */
/*         size_t new_capacity = nf->icon_capacity * 2; */
/*         NerdIcon* new_icons = realloc(nf->icons, new_capacity * sizeof(NerdIcon)); */
/*         if (!new_icons) { */
/*             FT_Done_Face(face); */
/*             return false; */
/*         } */
/*         nf->icons = new_icons; */
/*         nf->icon_capacity = new_capacity; */
/*     } */
    
/*     nf->icons[nf->icon_count++] = (NerdIcon){ */
/*         .codepoint = codepoint, */
/*         .category = category, */
/*         .name = name ? strdup(name) : NULL, */
/*         .ax = glyph->advance.x >> 6, */
/*         .ay = glyph->advance.y >> 6, */
/*         .bw = width, */
/*         .bh = height, */
/*         .bl = glyph->bitmap_left, */
/*         .bt = glyph->bitmap_top, */
/*         .tx = (float)nf->current_ox / nf->width, */
/*         .ty = (float)nf->current_oy / nf->height */
/*     }; */
    
/*     // Update packing positions */
/*     nf->current_ox += width + 1; */
/*     nf->current_rowh = fmax(nf->current_rowh, height); */
    
/*     FT_Done_Face(face); */
/*     return true; */
/* } */

bool loadNerdIcons(NerdFont* nf, NerdFontCategory category) {
    // Define codepoint ranges for each category
    static const struct {
        NerdFontCategory category;
        uint32_t start;
        uint32_t end;
        const char* prefix;  // For name generation
    } ranges[] = {
        {WICON, 0xE300, 0xE3EB, "weather-"},
        {FAICON, 0xF000, 0xF2E0, "fa-"},
        // Add all other ranges...
    };
    
    bool success = true;
    for (size_t i = 0; i < sizeof(ranges)/sizeof(ranges[0]); i++) {
        if (ranges[i].category == category || category == ALL) {
            for (uint32_t cp = ranges[i].start; cp <= ranges[i].end; cp++) {
                char name[64];
                snprintf(name, sizeof(name), "%s%04X", ranges[i].prefix, cp);
                if (!loadNerdIcon(nf, cp, ranges[i].category, name)) {
                    fprintf(stderr, "Failed to load Nerd icon U+%04X\n", cp);
                    success = false;
                }
            }
        }
    }
    
    return success;
}

// NOTE How we don't flush here
void drawNerdIcon(NerdFont* nf, uint32_t codepoint, float x, float y, float scale, Color color) {
    // Find the icon
    for (size_t i = 0; i < nf->icon_count; i++) {
        if (nf->icons[i].codepoint == codepoint) {
            NerdIcon* icon = &nf->icons[i];
            
            float xpos = x + icon->bl * scale;
            float ypos = y - (icon->bh - icon->bt + nf->descent) * scale;
            float w = icon->bw * scale;
            float h = icon->bh * scale;
            
            // Texture coordinates
            float tx1 = icon->tx;
            float ty1 = icon->ty;
            float tx2 = icon->tx + icon->bw / (float)nf->width;
            float ty2 = icon->ty + icon->bh / (float)nf->height;
            
            // Draw quad
            glBindTexture(GL_TEXTURE_2D, nf->texture_atlas);
            drawTriangleEx(
                (Vec2f){xpos, ypos + h}, color, (Vec2f){tx1, ty2},
                (Vec2f){xpos, ypos}, color, (Vec2f){tx1, ty1},
                (Vec2f){xpos + w, ypos}, color, (Vec2f){tx2, ty1}
            );
            drawTriangleEx(
                (Vec2f){xpos, ypos + h}, color, (Vec2f){tx1, ty2},
                (Vec2f){xpos + w, ypos}, color, (Vec2f){tx2, ty1},
                (Vec2f){xpos + w, ypos + h}, color, (Vec2f){tx2, ty2}
            );
            return;
        }
    }
    
    // Try to load on demand if not found
    if (loadNerdIcon(nf, codepoint, CUSTOM, NULL)) {
        drawNerdIcon(nf, codepoint, x, y, scale, color);
    }
}



