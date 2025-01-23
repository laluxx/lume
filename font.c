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
Font* loadFont(const char *fontPath, int fontSize, char *fontName) {
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

// NOTE we dont set the shader to allow the user to set whatever shader he want and batch render
void drawChar(Font* font, char character, float x, float y, float sx, float sy, Color color) {
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

float getCharacterWidth(Font* font, char character) {
    if (character < 0 || character >= MAX_CHARACTERS) {
        fprintf(stderr, "Character out of supported range.\n");
        return 0;
    }
    return font->characters[character].ax;
}


float getTextWidth(Font* font, const char* text) {
    float width = 0.0f;
    for (int i = 0; text[i] != '\0'; i++) {
        char c = text[i];
        if (c >= 0 && c < MAX_CHARACTERS) {
            width += getCharacterWidth(font, c);
        }
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



Font* beginFontLoad(const char* fontPath, int fontSize, double duration) {
    Font* font = (Font*)malloc(sizeof(Font));
    if (!font) {
        fprintf(stderr, "Memory allocation failed for font structure\n");
        return NULL;
    }

    FontLoadingState* state = (FontLoadingState*)malloc(sizeof(FontLoadingState));
    if (!state) {
        free(font);
        fprintf(stderr, "Memory allocation failed for loading state\n");
        return NULL;
    }

    if (FT_New_Face(ft, fontPath, 0, &state->face)) {
        free(state);
        free(font);
        fprintf(stderr, "Failed to load font: %s\n", fontPath);
        return NULL;
    }

    FT_Set_Pixel_Sizes(state->face, 0, fontSize);
    
    // Initialize loading state
    state->currentChar = 32;  // Start with space character
    state->fontSize = fontSize;
    state->startTime = glfwGetTime();
    state->duration = duration;
    state->isComplete = false;
    state->atlasSize = 512;  // Initial atlas size
    state->ox = 0;
    state->oy = 0;
    state->rowh = 0;

    // Allocate initial atlas
    state->atlas = (unsigned char*)calloc(state->atlasSize * state->atlasSize, sizeof(unsigned char));
    if (!state->atlas) {
        FT_Done_Face(state->face);
        free(state);
        free(font);
        fprintf(stderr, "Memory allocation failed for texture atlas\n");
        return NULL;
    }

    // Initialize font structure
    font->loadingState = state;
    font->ascent = state->face->size->metrics.ascender >> 6;
    font->descent = -(state->face->size->metrics.descender >> 6);
    font->width = state->atlasSize;
    font->height = state->atlasSize;
    
    // Create initial empty texture
    glGenTextures(1, &font->textureID);
    glBindTexture(GL_TEXTURE_2D, font->textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, state->atlasSize, state->atlasSize, 0, GL_RED, GL_UNSIGNED_BYTE, state->atlas);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return font;
}

// Update font loading progress - call this each frame
bool updateFontLoad(Font* font) {
    if (!font || !font->loadingState || font->loadingState->isComplete) {
        return true;  // Already complete
    }

    FontLoadingState* state = font->loadingState;
    double currentTime = glfwGetTime();
    double elapsed = currentTime - state->startTime;
    
    // Calculate how many characters should be loaded by now
    double progress = elapsed / state->duration;
    int targetChar = 32 + (int)((MAX_CHARACTERS - 32) * progress);
    
    // Load characters up to the target
    while (state->currentChar < targetChar && state->currentChar < MAX_CHARACTERS) {
        if (FT_Load_Char(state->face, state->currentChar, FT_LOAD_RENDER)) {
            fprintf(stderr, "Loading character %c failed!\n", state->currentChar);
            state->currentChar++;
            continue;
        }

        FT_GlyphSlot glyph = state->face->glyph;

        // Check if we need to move to next row
        if (state->ox + glyph->bitmap.width + 1 >= state->atlasSize) {
            state->oy += state->rowh;
            state->rowh = 0;
            state->ox = 0;
        }

        // Copy glyph to atlas
        for (int y = 0; y < glyph->bitmap.rows; y++) {
            for (int x = 0; x < glyph->bitmap.width; x++) {
                int atlas_y = state->oy + (glyph->bitmap.rows - 1 - y);
                state->atlas[(state->ox + x) + (atlas_y * state->atlasSize)] = 
                    glyph->bitmap.buffer[x + y * glyph->bitmap.pitch];
            }
        }

        // Store character info
        font->characters[state->currentChar].ax = glyph->advance.x >> 6;
        font->characters[state->currentChar].ay = glyph->advance.y >> 6;
        font->characters[state->currentChar].bw = glyph->bitmap.width;
        font->characters[state->currentChar].bh = glyph->bitmap.rows;
        font->characters[state->currentChar].bl = glyph->bitmap_left;
        font->characters[state->currentChar].bt = glyph->bitmap_top;
        font->characters[state->currentChar].tx = (float)state->ox / state->atlasSize;
        font->characters[state->currentChar].ty = (float)state->oy / state->atlasSize;

        // Update position
        state->rowh = fmax(state->rowh, glyph->bitmap.rows);
        state->ox += glyph->bitmap.width + 1;
        state->currentChar++;

        // Update texture
        glBindTexture(GL_TEXTURE_2D, font->textureID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, state->atlasSize, state->atlasSize, 
                        GL_RED, GL_UNSIGNED_BYTE, state->atlas);
    }

    // Check if loading is complete
    if (state->currentChar >= MAX_CHARACTERS || elapsed >= state->duration) {
        // Clean up
        free(state->atlas);
        FT_Done_Face(state->face);
        free(state);
        font->loadingState = NULL;
        return true;
    }

    return false;
}

float getFontLoadProgress(Font* font) {
    if (!font || !font->loadingState) {
        return 1.0f;  // Complete
    }
    
    return (float)(font->loadingState->currentChar - 32) / (MAX_CHARACTERS - 32);
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

/* char* getFontPath(const char* fontSpec) { */
/*     char family[256] = ""; */
/*     char weight[32] = ""; */
/*     char style[32] = ""; */
/*     int size = 0; */

/*     // Parse font specification */
/*     sscanf(fontSpec, "%255[^_]_%31[^_]_%31[^_]_%d", family, weight, style, &size); */

/*     // Construct fc-match command */
/*     char cmd[MAX_PATH]; */
/*     snprintf(cmd, sizeof(cmd), "fc-match -f '%%{file}\\n' '%s:weight=%s:style=%s:size=%d' | head -n 1",  */
/*              family, weight, style, size); */

/*     FILE* pipe = popen(cmd, "r"); */
/*     if (!pipe) { */
/*         perror("popen"); */
/*         return NULL; */
/*     } */

/*     char* result = malloc(MAX_PATH); */
/*     if (!result) { */
/*         perror("malloc"); */
/*         pclose(pipe); */
/*         return NULL; */
/*     } */

/*     if (fgets(result, MAX_PATH, pipe) != NULL) { */
/*         size_t len = strlen(result); */
/*         if (len > 0 && result[len-1] == '\n') { */
/*             result[len-1] = '\0'; */
/*         } */
/*     } else { */
/*         free(result); */
/*         result = NULL; */
/*     } */

/*     if (pclose(pipe) == -1) { */
/*         perror("pclose"); */
/*         free(result); */
/*         return NULL; */
/*     } */

/*     return result; */
/* } */
