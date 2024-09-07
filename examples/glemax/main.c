#include "font.h"
#include "renderer.h"
#include "window.h"
#include "input.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *content;   // Dynamic array to hold the text content
    size_t size;     // Current size of the content
    size_t capacity; // Allocated capacity of the content
    int point;       // Cursor position (Emacs "point")
} Buffer;

void initBuffer(Buffer *buffer) {
    buffer->capacity = 1024; // Initial capacity
    buffer->content = malloc(buffer->capacity * sizeof(char));
    buffer->size = 0;
    buffer->point = 0;
    if (buffer->content) {
        buffer->content[0] = '\0';
    } else {
        fprintf(stderr, "Failed to allocate memory for buffer.\n");
        exit(EXIT_FAILURE);
    }
}

void freeBuffer(Buffer *buffer) {
    free(buffer->content);
    buffer->content = NULL;
    buffer->size = 0;
    buffer->capacity = 0;
    buffer->point = 0;
}

void insertChar(Buffer *buffer, char c) {
    if (buffer->size + 1 >= buffer->capacity) {
        buffer->capacity *= 2;
        buffer->content = realloc(buffer->content, buffer->capacity * sizeof(char));
        if (!buffer->content) {
            fprintf(stderr, "Failed to reallocate memory for buffer.\n");
            return;
        }
    }
    memmove(buffer->content + buffer->point + 1, buffer->content + buffer->point, buffer->size - buffer->point);
    buffer->content[buffer->point] = c;
    buffer->point++;
    buffer->size++;
    buffer->content[buffer->size] = '\0';
}

void handleInput(Buffer *buffer) {
    // Handle text input
    for (int key = 32; key <= 126; key++) { // Basic ASCII range for printable characters
        if (isKeyPressed(key)) {
            insertChar(buffer, (char)key);
        }
    }

    // Backspace handling
    if (isKeyPressed(KEY_BACKSPACE) && buffer->point > 0) {
        buffer->point--;
        memmove(buffer->content + buffer->point, buffer->content + buffer->point + 1, buffer->size - buffer->point);
        buffer->size--;
        buffer->content[buffer->size] = '\0';
    }
}

int main() {
    const int sw = 1920;
    const int sh = 1080;


    GLFWwindow* window = initWindow(sw, sh, "GL Text Editor");
    initInput(window);
    initRenderer(sw, sh);

    Font *font = loadFont("radon.otf", 30);
    if (!font) {
        fprintf(stderr, "Failed to load font.\n");
        return EXIT_FAILURE;
    }

    Buffer mainBuffer;
    initBuffer(&mainBuffer);
    
    float lh = getFontHeight(font);
    
    while (!windowShouldClose()) {
        handleInput(&mainBuffer);
        updateInput();
        
        beginDrawing();
        clearBackground((Color){0, 0, 0, 1}); // Black background

        drawText(font, mainBuffer.content, 0, sh-lh, 1.0, 1.0);

        endDrawing();
    }

    freeBuffer(&mainBuffer);
    closeWindow();
    return EXIT_SUCCESS;
}
