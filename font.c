#include "font.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>


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

void drawText(Renderer *renderer, Font* font, const char* text, float x, float y, float sx, float sy) {
    const char *p;
    useShader(renderer, "text");  // Assume "text" is a valid shader for text rendering
    glBindTexture(GL_TEXTURE_2D, font->textureID);  // Set the font texture
    glActiveTexture(GL_TEXTURE0);  // Use the first texture unit

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

        Color white = {255, 255, 255, 255}; // Assuming white color for text

        // Draw each character as two triangles
        drawTriangleColors(renderer, (Vec2f){xpos, ypos + h}, white, uv1,
                           (Vec2f){xpos, ypos}, white, uv2,
                           (Vec2f){xpos + w, ypos}, white, uv3);
        drawTriangleColors(renderer, (Vec2f){xpos, ypos + h}, white, uv1,
                           (Vec2f){xpos + w, ypos}, white, uv3,
                           (Vec2f){xpos + w, ypos + h}, white, uv4);

        // Advance cursors for next glyph
        x += ch.ax * sx;
        y += ch.ay * sy;
    }
}


void freeFont(Font* font) {
    glDeleteTextures(1, &font->textureID);
    free(font);
}
