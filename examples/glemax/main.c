#include "font.h"
#include "renderer.h"
#include "window.h"
#include "input.h"
#include <common.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "theme.h"

// TODO keep a list of buffer
// and a reference to the active buffer
// TODO add readonly bool to the Buffer

typedef struct {
    char *content;   // Dynamic array to hold the text content
    size_t size;     // Current size of the content
    size_t capacity; // Allocated capacity of the content
    int point;       // Cursor position (Emacs "point")
} Buffer;

void initBuffer(Buffer *buffer);
void freeBuffer(Buffer *buffer);
void insertChar(Buffer *buffer, char c);
void handleKeys(Buffer *buffer);
void drawCursor(Buffer *buffer, Font *font, float x, float y, Color color);
void right_char(Buffer * buffer);
void left_char(Buffer * buffer);
void previous_line(Buffer * buffer);
void next_line(Buffer * buffer);
void move_end_of_line(Buffer * buffer);
void move_beginning_of_line(Buffer * buffer);
void delete_char(Buffer * buffer);
void kill_line(Buffer * buffer);
void open_line(Buffer *buffer);

void keyInputHandler(int key, int action, int mods);
void textInputHandler(unsigned int codepoint);
void insertUnicodeCharacter(Buffer *buffer, unsigned int codepoint);
int encodeUTF8(char *out, unsigned int codepoint);

void initBuffer(Buffer * buffer) {
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

void insertChar(Buffer * buffer, char c) {
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

void drawCursor(Buffer *buffer, Font *font, float x, float y, Color color) {
    int lineCount = 0;
    float cursorX = x;

    for (int i = 0; i < buffer->point; i++) {
        if (buffer->content[i] == '\n') {
            lineCount++; // Increment line count on newline
            cursorX = x; // Reset cursor position to the start of the new line
        } else {
            cursorX += getCharacterWidth(font, buffer->content[i]); // Use actual character width
        }
    }

    float cursorWidth;
    if (buffer->point < buffer->size && buffer->content[buffer->point] != '\n') {
        cursorWidth = getCharacterWidth(font, buffer->content[buffer->point]);
    } else {
        // Default to the width of a space when at the end of a line or at the end of the buffer
        cursorWidth = getCharacterWidth(font, ' ');
    }

    Vec2f cursorPosition = {cursorX, y - lineCount * (font->ascent + font->descent) - font->descent};
    Vec2f cursorSize = {cursorWidth, (float)(font->ascent + font->descent)};

    drawRectangle(cursorPosition, cursorSize, color);
}



/* void drawCursor(Buffer *buffer, Font *font, float x, float y, Color color) { */
/*     int lineCount = 0; */
/*     float cursorX = x; */

/*     for (int i = 0; i < buffer->point; i++) { */
/*         if (buffer->content[i] == '\n') { */
/*             lineCount++; // Increment line count on newline */
/*             cursorX = x; // Reset cursor position to the start of the new line */
/*         } else { */
/*             cursorX += getFontWidth(font); // NOTE monospace assumption */
/*         } */
/*     } */

/*     Vec2f cursorPosition = {cursorX, y - lineCount * (font->ascent + font->descent) - font->descent}; */

/*     Vec2f cursorSize = {getFontWidth(font), (float)(font->ascent + font->descent)}; */

/*     drawRectangle(cursorPosition, cursorSize, color); */
/* } */

void left_char(Buffer * buffer) {
    if (buffer->point > 0) {
        buffer->point--;
    }
}

void right_char(Buffer * buffer) {
    if (buffer->point < buffer->size) {
        buffer->point++;
    }
}


// TODO Remember the highest character position you were in
// when you first called previous_line or next_line and kept calling it
// like emacs does
void previous_line(Buffer *buffer) {
    if (buffer->point == 0)
        return; // Already at the start of the text, no further up to go.

    int previousLineEnd = 0;
    int previousLineStart = 0;
    int currentLineStart = 0;

    // Step backwards to find the start of the current line
    for (int i = buffer->point - 1; i >= 0; i--) {
        if (buffer->content[i] == '\n') {
            currentLineStart = i + 1;
            break;
        }
    }

    // If we're at the start of the text, the current line is the first line
    if (currentLineStart == 0) {
        buffer->point = 0;
        return;
    }

    // Step backwards to find the start of the previous line
    for (int i = currentLineStart - 2; i >= 0;
         i--) { // Start from before the newline of the current line start
        if (buffer->content[i] == '\n') {
            previousLineStart = i + 1;
            break;
        }
    }

    // Step backwards to find the end of the line before the current
    for (int i = currentLineStart - 1; i >= 0; i--) {
        if (buffer->content[i] == '\n') {
            previousLineEnd = i;
            break;
        }
        if (i == 0) { // If no newline found, the previous line end is at the start
            // of the file
            previousLineEnd = 0;
        }
    }

    // Calculate the desired column position
    int column = buffer->point - currentLineStart;
    int previousLineLength = previousLineEnd - previousLineStart;

    // If the desired column is beyond the length of the previous line, set point
    // at the end of the previous line
    if (column >= previousLineLength) {
        buffer->point = previousLineEnd;
    } else {
        buffer->point = previousLineStart + column;
    }
}

void next_line(Buffer *buffer) {
    if (buffer->point >= buffer->size)
        return; // Already at the end of the text.

    int currentLineEnd = buffer->point;
    int nextLineStart = buffer->size;
    int columnPosition = 0;

    // Determine the end of the current line.
    for (int i = buffer->point; i < buffer->size; i++) {
        if (buffer->content[i] == '\n') {
            currentLineEnd = i;
            nextLineStart = i + 1;
            break;
        }
    }

    // Calculate column position.
    int currentLineStart = buffer->point;
    while (currentLineStart > 0 &&
           buffer->content[currentLineStart - 1] != '\n') {
        currentLineStart--;
    }
    columnPosition = buffer->point - currentLineStart;

    // Calculate the point position in the next line, limited by the line's
    // length.
    int targetPosition = nextLineStart + columnPosition;
    for (int i = nextLineStart; i <= buffer->size; i++) {
        if (buffer->content[i] == '\n' || i == buffer->size) {
            if (targetPosition > i)
                targetPosition = i;
            break;
        }
    }
    buffer->point = targetPosition;
}

void move_beginning_of_line(Buffer * buffer) {
    for (int i = buffer->point - 1; i >= 0; i--) {
        if (buffer->content[i] == '\n') {
            buffer->point = i + 1; // Set point right after the newline.
            return;
        }
    }
    buffer->point = 0; // no newline was found, go to the beginning of buffer
}

void move_end_of_line(Buffer *buffer) {
    for (int i = buffer->point; i < buffer->size; i++) {
        if (buffer->content[i] == '\n') {
            buffer->point = i;
            return;
        }
    }
    buffer->point = buffer->size; // no newline was found, go to the end of buffer
}

void delete_char(Buffer *buffer) {
    if (buffer->point >= buffer->size)
        return;

    // Move all characters after the cursor left by one position
    memmove(buffer->content + buffer->point, buffer->content + buffer->point + 1,
            buffer->size - buffer->point - 1);
    // Decrease the size of the buffer
    buffer->size--;
    // Null-terminate the string
    buffer->content[buffer->size] = '\0';
}

void kill_line(Buffer *buffer) {
    if (buffer->point >= buffer->size)
        return; // Nothing to delete if at the end of the buffer

    int endOfLine = buffer->point;
    // Find the end of the current line or the buffer
    while (endOfLine < buffer->size && buffer->content[endOfLine] != '\n') {
        endOfLine++;
    }

    // If endOfLine is at a newline character, include it in the deletion
    if (endOfLine < buffer->size && buffer->content[endOfLine] == '\n') {
        endOfLine++;
    }

    // Calculate the number of characters to delete
    int numToDelete = endOfLine - buffer->point;

    // Shift remaining text in the buffer left over the killed text
    if (endOfLine < buffer->size) {
        memmove(buffer->content + buffer->point, buffer->content + endOfLine,
                buffer->size - endOfLine);
    }

    // Update buffer size
    buffer->size -= numToDelete;

    // Null-terminate the buffer
    buffer->content[buffer->size] = '\0';
}

void open_line(Buffer *buffer) {
    // Ensure there is enough capacity, and if not, expand the buffer
    if (buffer->size + 1 >= buffer->capacity) {
        buffer->capacity *= 2;
        char *newContent =
            realloc(buffer->content, buffer->capacity * sizeof(char));
        if (!newContent) {
            fprintf(stderr, "Failed to reallocate memory for buffer.\n");
            return;
        }
        buffer->content = newContent;
    }

    // Shift text to the right to make space for a new newline character
    memmove(buffer->content + buffer->point + 1, buffer->content + buffer->point,
            buffer->size - buffer->point +
            1); // Include the null terminator in the move

    // Insert the newline at the current cursor position
    buffer->content[buffer->point] = '\n';

    // Update buffer size
    buffer->size++;

    // Do not move the cursor to reflect the requirement to stay at the end of the
    // original line
}

Buffer mainBuffer;

int main() {
    int sw = 1920;
    int sh = 1080;

    initThemes();

    GLFWwindow *window = initWindow(sw, sh, "main.c - Glemax");
    registerTextInputCallback(textInputHandler);
    registerKeyInputCallback(keyInputHandler);

    Font *font = loadFont("jetb.ttf", 100); // 14~16

    initBuffer(&mainBuffer);


    while (!windowShouldClose()) {
        sw = getScreenWidth();
        sh = getScreenHeight();
        
        beginDrawing();
        clearBackground(CT.bg);


        useShader("simple");
        drawRectangle((Vec2f){0, 21}, (Vec2f){sw, 25}, CT.modeline);
        drawCursor(&mainBuffer, font, 0, sh - font->ascent, CT.cursor);
        flush();

        drawTextEx(font, mainBuffer.content, 0, sh - font->ascent + font->descent, 1.0, 1.0, WHITE, CT.bg, mainBuffer.point);
        
        endDrawing();
    }

    freeFont(font);
    freeBuffer(&mainBuffer);
    closeWindow();
    return EXIT_SUCCESS;
}



void keyInputHandler(int key, int action, int mods) {
    Buffer *buffer = &mainBuffer; // Assuming mainBuffer is globally accessible
    if (!buffer) return;

    bool shiftPressed = mods & GLFW_MOD_SHIFT;
    bool ctrlPressed = mods & GLFW_MOD_CONTROL;
    bool altPressed = mods & GLFW_MOD_ALT;

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
        case KEY_BACKSPACE:
            if (buffer->point > 0) {
                buffer->point--;
                memmove(buffer->content + buffer->point, buffer->content + buffer->point + 1, buffer->size - buffer->point);
                buffer->size--;
                buffer->content[buffer->size] = '\0';
            }
            break;
        case KEY_ENTER:
            insertChar(buffer, '\n');
            break;
        case KEY_DOWN:
            next_line(buffer);
            break;
        case KEY_UP:
            previous_line(buffer);
            break;
        case KEY_LEFT:
            left_char(buffer);
            break;
        case KEY_RIGHT:
            right_char(buffer);
            break;
        case KEY_DELETE:
            delete_char(buffer);
            break;
        case KEY_N:
            if (ctrlPressed) next_line(buffer);
            break;
        case KEY_P:
            if (ctrlPressed) previous_line(buffer);
            break;
        case KEY_F:
            if (ctrlPressed) right_char(buffer);
            break;
        case KEY_B:
            if (ctrlPressed) left_char(buffer);
            break;
        case KEY_E:
            if (ctrlPressed) move_end_of_line(buffer);
            break;
        case KEY_A:
            if (ctrlPressed) move_beginning_of_line(buffer);
            break;
        case KEY_D:
            if (ctrlPressed) delete_char(buffer);
            break;
        case KEY_K:
            if (ctrlPressed) kill_line(buffer);
            break;
        case KEY_O:
            if (ctrlPressed) open_line(buffer);
            break;
        case KEY_EQUAL:
            if (altPressed) nextTheme();
            break;
        case KEY_MINUS:
            if (altPressed) previousTheme();
            break;
        }
    }
}













/* void keyInputHandler(int key, int action, int mods) { */
/*     Buffer *buffer = &mainBuffer; // Assuming mainBuffer is globally accessible */
/*     if (!buffer) return; */

/*     // Handle control keys */
/*     if (action == GLFW_PRESS || action == GLFW_REPEAT) { */
/*         switch (key) { */
/*         case GLFW_KEY_BACKSPACE: */
/*             if (buffer->point > 0) { */
/*                 buffer->point--; */
/*                 memmove(buffer->content + buffer->point, buffer->content + buffer->point + 1, buffer->size - buffer->point); */
/*                 buffer->size--; */
/*                 buffer->content[buffer->size] = '\0'; */
/*             } */
/*             break; */
/*         case KEY_ENTER: */
/*             insertChar(buffer, '\n'); */
/*             break; */
/*         case KEY_DOWN: */
/*             next_line(buffer); */
/*             break; */
/*         case KEY_UP: */
/*             previous_line(buffer); */
/*             break; */
/*         case KEY_LEFT: */
/*             left_char(buffer); */
/*             break; */
/*         case KEY_RIGHT: */
/*             right_char(buffer); */
/*             break; */
/*         case KEY_DELETE: */
/*             delete_char(buffer); */
/*             break; */
/*             // Add more control keys as needed */
/*         } */
/*     } */
/* } */

void textInputHandler(unsigned int codepoint) {
    Buffer *buffer = &mainBuffer; // Assuming 'mainBuffer' is your current
    // text editing buffer
    if (buffer != NULL) {
        insertUnicodeCharacter(buffer, codepoint);
    }
}

void insertUnicodeCharacter(Buffer * buffer, unsigned int codepoint) {
    char utf8[5]; // Buffer to hold UTF-8 encoded character
    int bytes = encodeUTF8(
                           utf8, codepoint); // Function to convert codepoint to UTF-8
    for (int i = 0; i < bytes; i++) {
        insertChar(buffer, utf8[i]);
    }
}

int encodeUTF8(char *out, unsigned int codepoint) {
    if (codepoint <= 0x7F) {
        out[0] = codepoint;
        return 1;
    } else if (codepoint <= 0x7FF) {
        out[0] = 192 + (codepoint >> 6);
        out[1] = 128 + (codepoint & 63);
        return 2;
    } else if (codepoint <= 0xFFFF) {
        out[0] = 224 + (codepoint >> 12);
        out[1] = 128 + ((codepoint >> 6) & 63);
        out[2] = 128 + (codepoint & 63);
        return 3;
    } else {
        out[0] = 240 + (codepoint >> 18);
        out[1] = 128 + ((codepoint >> 12) & 63);
        out[2] = 128 + ((codepoint >> 6) & 63);
        out[3] = 128 + (codepoint & 63);
        return 4;
    }
}
