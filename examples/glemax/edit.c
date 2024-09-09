#include "edit.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

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

void right_char(Buffer * buffer) {
    if (buffer->point < buffer->size) {
        buffer->point++;
    }
}

void left_char(Buffer * buffer) {
    if (buffer->point > 0) {
        buffer->point--;
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
    for (size_t i = buffer->point; i < buffer->size; i++) {
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
    size_t targetPosition = nextLineStart + columnPosition;
    for (size_t i = nextLineStart; i <= buffer->size; i++) {
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
    for (size_t i = buffer->point; i < buffer->size; i++) {
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

    size_t endOfLine = buffer->point;
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

    memmove(buffer->content + buffer->point + 1, buffer->content + buffer->point,
            buffer->size - buffer->point + 1); // NOTE +1 for null terminator

    buffer->content[buffer->point] = '\n';
    buffer->size++;
}

void delete_indentation(Buffer *buffer) {
    move_beginning_of_line(buffer);

    if (buffer->point > 0) {
        left_char(buffer);
        delete_char(buffer);
        insertChar(buffer, ' ');
        left_char(buffer);
    }
}


bool isWordChar(char c) {
    return isalnum(c) || c == '_';
}

void backward_kill_word(Buffer *buffer) {
    if (buffer->point == 0) return;

    size_t originalPoint = buffer->point;
    size_t start = buffer->point;
    size_t end = buffer->point;

    // Skip any trailing whitespace
    while (start > 0 && isspace((unsigned char)buffer->content[start - 1])) {
        start--;
    }

    // Find the start of the word
    while (start > 0 && isWordChar(buffer->content[start - 1])) {
        start--;
    }

    size_t lengthToDelete = end - start;

    if (lengthToDelete > 0) {
        memmove(buffer->content + start, buffer->content + end, buffer->size - end);
        buffer->size -= lengthToDelete;
        buffer->point = start;
        buffer->content[buffer->size] = '\0';  // Null-terminate the string
    }
}

