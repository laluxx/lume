#include "font.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

// TODO loadFontEx
FT_Library ft;

void initFreeType() {
    if (FT_Init_FreeType(&ft)) {
        fprintf(stderr, "Could not init FreeType Library\n");
        exit(1);
    }
}

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

bool shouldSaveAtlas = true;

Font* loadFont(const char* fontPath, int fontSize) {
    FT_Face face;
    if (FT_New_Face(ft, fontPath, 0, &face)) {
        fprintf(stderr, "Failed to load font: %s\n", fontPath);
        return NULL;
    }
   
    printf("[LOADED FONT] %s %i\n", fontPath, fontSize);

    FT_Set_Pixel_Sizes(face, 0, fontSize);

    Font* font = (Font*)malloc(sizeof(Font));
    if (!font) {
        fprintf(stderr, "Memory allocation failed for font structure\n");
        return NULL;
    }

    // Create a buffer for the texture atlas
    unsigned char* atlas = (unsigned char*)calloc(1024 * 1024, sizeof(unsigned char));
    if (!atlas) {
        fprintf(stderr, "Memory allocation failed for texture atlas\n");
        free(font);
        return NULL;
    }

    int ox = 0, oy = 0, rowh = 0;
    for (int i = 32; i < 128; i++) {
        if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
            fprintf(stderr, "Loading character %c failed!\n", i);
            continue;
        }

        if (ox + face->glyph->bitmap.width + 1 >= 1024) {
            oy += rowh;
            rowh = 0;
            ox = 0;
        }

        // Flip the glyph vertically as it is loaded into the buffer
        for (int y = 0; y < face->glyph->bitmap.rows; y++) {
            for (int x = 0; x < face->glyph->bitmap.width; x++) {
                int atlas_y = oy + y;
                int glyph_y = face->glyph->bitmap.rows - 1 - y;  // flip here
                atlas[(ox + x) + (atlas_y) * 1024] = face->glyph->bitmap.buffer[x + glyph_y * face->glyph->bitmap.pitch];
            }
        }

        font->characters[i].ax = face->glyph->advance.x >> 6;
        font->characters[i].ay = face->glyph->advance.y >> 6;
        font->characters[i].bw = face->glyph->bitmap.width;
        font->characters[i].bh = face->glyph->bitmap.rows;
        font->characters[i].bl = face->glyph->bitmap_left;
        font->characters[i].bt = face->glyph->bitmap_top;
        font->characters[i].tx = ox / (float)1024;
        font->characters[i].ty = oy / (float)1024;

        rowh = fmax(rowh, face->glyph->bitmap.rows);
        ox += face->glyph->bitmap.width + 1;
    }

    glGenTextures(1, &font->textureID);
    glBindTexture(GL_TEXTURE_2D, font->textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1024, 1024, 0, GL_RED, GL_UNSIGNED_BYTE, atlas);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Write atlas as png
    if (shouldSaveAtlas) {
        stbi_write_png("font_atlas.png", 1024, 1024, 1, atlas, 1024);        
    }

    free(atlas);
    FT_Done_Face(face);

    return font;
}

void drawText(Font* font, const char* text, float x, float y, float sx, float sy) {
    const char *p;
    useShader("text");
    glBindTexture(GL_TEXTURE_2D, font->textureID);
    glActiveTexture(GL_TEXTURE0);

    for (p = text; *p; p++) {
        Character ch = font->characters[*p];

        GLfloat xpos = x + ch.bl * sx;
        GLfloat ypos = y - (ch.bh - ch.bt) * sy;

        GLfloat w = ch.bw * sx;
        GLfloat h = ch.bh * sy;

        Vec2f uv1 = {ch.tx, ch.ty + ch.bh / 1024.0f};
        Vec2f uv2 = {ch.tx, ch.ty};
        Vec2f uv3 = {ch.tx + ch.bw / 1024.0f, ch.ty};
        Vec2f uv4 = {ch.tx + ch.bw / 1024.0f, ch.ty + ch.bh / 1024.0f};


        // Draw each character NOTE BLACK is defined somehow
        drawTriangleColors((Vec2f){xpos, ypos + h}, BLACK, uv1,
                           (Vec2f){xpos, ypos}, BLACK, uv2,
                           (Vec2f){xpos + w, ypos}, BLACK, uv3);
        
        drawTriangleColors((Vec2f){xpos, ypos + h}, BLACK, uv1,
                           (Vec2f){xpos + w, ypos}, BLACK, uv3,
                           (Vec2f){xpos + w, ypos + h}, BLACK, uv4);

        // Advance cursors for next glyph
        x += ch.ax * sx;
        y += ch.ay * sy;
    }
    flush();
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



/* static double lastTime = 0.0; */
/* static int frameCount = 0; */
/* static char fpsText[256]; */

/* void initFPS() { */
/*     lastTime = glfwGetTime(); */
/*     frameCount = 0; */
/*     sprintf(fpsText, "FPS: %d", 0); */
/* } */

/* void updateFPS() { */
/*     double currentTime = glfwGetTime(); */
/*     frameCount++; */
/*     if (currentTime - lastTime >= 1.0) { // If a second has passed */
/*         // Display the frame count here any way you like or simply store it */
/*         sprintf(fpsText, "FPS: %d", frameCount); */

/*         frameCount = 0; */
/*         lastTime += 1.0; */
/*     } */
/* } */


/* void drawFPS(Font* font, float x, float y, float sx, float sy) { */
/*     drawText(font, fpsText, x, y, sx, sy); */
/* } */
