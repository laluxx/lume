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

void beginning_of_buffer(Buffer *buffer) {
    if (buffer != NULL && buffer->content != NULL) {
        buffer->point = 0; // Move the cursor to the beginning of the buffer's content
    }
}

void end_of_buffer(Buffer *buffer) {
    if (buffer != NULL && buffer->content != NULL) {
        buffer->point = buffer->size; // Move the cursor to the end of the buffer's content
    }
}



void right_char(Buffer *buffer, bool shift) {
    if (shift) {
        if (!buffer->region.active) {
            activateRegion(buffer);
        }
    } else {
        if (!buffer->region.marked) buffer->region.active = false;
    }

    if (buffer->point < buffer->size) {
        buffer->point++;
    }
}

void left_char(Buffer * buffer, bool shift) {
    if (shift) {
        if (!buffer->region.active) {
            activateRegion(buffer);
        }
    } else {
        if (!buffer->region.marked) buffer->region.active = false;
    }

    if (buffer->point > 0) {
        buffer->point--;
    }
}


// TODO Remember the highest character position you were in
// when you first called previous_line or next_line and kept calling it
// like emacs does
void previous_line(Buffer *buffer, bool shift) {
    if (shift) {
        if (!buffer->region.active) {
            activateRegion(buffer);
        }
    } else {
        if (!buffer->region.marked) buffer->region.active = false;
    }
    
    if (buffer->point == 0) return;

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

void next_line(Buffer *buffer, bool shift) {
    if (shift) {
        if (!buffer->region.active) {
            activateRegion(buffer);
        }
    } else {
        if (!buffer->region.marked) buffer->region.active = false;
    }

    if (buffer->point >= buffer->size) return;

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

void move_beginning_of_line(Buffer * buffer, bool shift) {
    if (shift) {
        if (!buffer->region.active) {
            activateRegion(buffer);
        }
    } else {
        if (!buffer->region.marked) buffer->region.active = false;
    }

    for (int i = buffer->point - 1; i >= 0; i--) {
        if (buffer->content[i] == '\n') {
            buffer->point = i + 1; // Set point right after the newline.
            return;
        }
    }
    buffer->point = 0; // no newline was found, go to the beginning of buffer
}

void move_end_of_line(Buffer *buffer, bool shift) {
    if (shift) {
        if (!buffer->region.active) {
            activateRegion(buffer);
        }
    } else {
        if (!buffer->region.marked) buffer->region.active = false;
    }

    for (size_t i = buffer->point; i < buffer->size; i++) {
        if (buffer->content[i] == '\n') {
            buffer->point = i;
            return;
        }
    }
    buffer->point = buffer->size; // no newline was found, go to the end of buffer
}

void delete_char(Buffer *buffer) {
    if (buffer->point >= buffer->size) return;
    if (buffer->region.active) buffer->region.active = false;

    // Move all characters after the cursor left by one position
    memmove(buffer->content + buffer->point, buffer->content + buffer->point + 1,
            buffer->size - buffer->point - 1);
    // Decrease the size of the buffer
    buffer->size--;
    // Null-terminate the string
    buffer->content[buffer->size] = '\0';
}


void kill_line(Buffer *buffer, KillRing *kr) {
    if (buffer->point >= buffer->size) return; // Nothing to delete if at the end of the buffer

    size_t startOfLine = buffer->point;
    size_t endOfLine = startOfLine;

    // Determine the end of the current line, but do not include the newline character
    while (endOfLine < buffer->size && buffer->content[endOfLine] != '\n') {
        endOfLine++;
    }

    size_t numToDelete = endOfLine - startOfLine;

    if (numToDelete > 0) {
        // Capture the text to be killed
        char *cut_text = malloc(numToDelete + 1);
        if (cut_text) {
            memcpy(cut_text, buffer->content + startOfLine, numToDelete);
            cut_text[numToDelete] = '\0';
            kill(kr, cut_text); // Add to kill ring
            free(cut_text); // Free temporary text buffer
        }

        // Shift remaining text in the buffer left over the killed text
        memmove(buffer->content + startOfLine, buffer->content + endOfLine, buffer->size - endOfLine + 1); // +1 for null terminator

        // Update buffer size
        buffer->size -= numToDelete;
    }

    // Handle special case for an empty line or a line where the cursor is right at the newline
    if (startOfLine == endOfLine && startOfLine < buffer->size && buffer->content[startOfLine] == '\n') {
        // Kill the newline itself
        memmove(buffer->content + startOfLine, buffer->content + startOfLine + 1, buffer->size - startOfLine);
        buffer->size--;
    }
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
    move_beginning_of_line(buffer, false);

    if (buffer->point > 0) {
        left_char(buffer, false);
        delete_char(buffer);
        insertChar(buffer, ' ');
        left_char(buffer, false);
    }
}

/* void backward_kill_word(Buffer *buffer) { */
/*     if (buffer->point == 0) return; */

/*     size_t originalPoint = buffer->point; */
/*     size_t start = buffer->point; */
/*     size_t end = buffer->point; */

/*     // Skip any trailing whitespace */
/*     while (start > 0 && isspace((unsigned char)buffer->content[start - 1])) { */
/*         start--; */
/*     } */

/*     // Find the start of the word */
/*     while (start > 0 && isWordChar(buffer->content[start - 1])) { */
/*         start--; */
/*     } */

/*     size_t lengthToDelete = end - start; */

/*     if (lengthToDelete > 0) { */
/*         memmove(buffer->content + start, buffer->content + end, buffer->size - end); */
/*         buffer->size -= lengthToDelete; */
/*         buffer->point = start; */
/*         buffer->content[buffer->size] = '\0';  // Null-terminate the string */
/*     } */
/* } */


void addIndentation(Buffer *buffer, int indentation) {
    // Ensure there is enough capacity in the buffer
    if (buffer->size + indentation >= buffer->capacity) {
        buffer->capacity *= 2;
        char *newContent = realloc(buffer->content, buffer->capacity * sizeof(char));
        if (!newContent) {
            fprintf(stderr, "Failed to reallocate memory for buffer.\n");
            return;
        }
        buffer->content = newContent;
    }

    int lineStart = buffer->point;
    while (lineStart > 0 && buffer->content[lineStart - 1] != '\n') {
        lineStart--;
    }

    // Add spaces at the beginning of the line
    memmove(buffer->content + lineStart + indentation, buffer->content + lineStart, buffer->size - lineStart + 1); // Including null terminator
    for (int i = 0; i < indentation; i++) {
        buffer->content[lineStart + i] = ' ';
    }
    buffer->size += indentation;

    // Adjust cursor position relative to the added spaces if cursor is beyond the added spaces
    if (buffer->point >= lineStart) {
        buffer->point += indentation;
    }
}

void removeIndentation(Buffer *buffer, int indentation) {
    int lineStart = buffer->point;
    while (lineStart > 0 && buffer->content[lineStart - 1] != '\n') {
        lineStart--;
    }

    // Determine the actual number of spaces we can remove
    int count = 0;
    for (int i = lineStart; i < lineStart + indentation && buffer->content[i] == ' '; i++) {
        count++;
    }

    if (count > 0) {
        memmove(buffer->content + lineStart, buffer->content + lineStart + count, buffer->size - (lineStart + count) + 1); // Including null terminator
        buffer->size -= count;

        // Adjust cursor position relative to the removed spaces if cursor is beyond the removed spaces
        if (buffer->point > lineStart + count) {
            buffer->point -= count;
        } else if (buffer->point > lineStart) {
            buffer->point = lineStart;
        }
    }
}







void initKillRing(KillRing* kr, int capacity) {
    kr->entries = malloc(sizeof(char*) * capacity);
    kr->size = 0;
    kr->capacity = capacity;
    kr->index = 0;
    for (int i = 0; i < capacity; i++) {
        kr->entries[i] = NULL;
    }
}

void freeKillRing(KillRing* kr) {
    for (int i = 0; i < kr->capacity; i++) {
        free(kr->entries[i]);
    }
    free(kr->entries);
}


void kill(KillRing* kr, const char* text) {
    if (kr->size >= kr->capacity) {
        free(kr->entries[kr->index]); // free the oldest entry if the ring is full
    } else {
        kr->size++;
    }

    kr->entries[kr->index] = strdup(text);
    kr->index = (kr->index + 1) % kr->capacity;
}


void kill_region(Buffer *buffer, KillRing *kr) {
    if (!buffer->region.active) {
        printf("No active region to kill.\n");
        return;  // No region active, nothing to kill
    }

    size_t start = buffer->region.start;
    size_t end = buffer->region.end;

    // Ensure start is always less than end
    if (start > end) {
        size_t temp = start;
        start = end;
        end = temp;
    }

    if (end > buffer->size) end = buffer->size;  // Clamp end to buffer size

    size_t region_length = end - start;
    if (region_length == 0) {
        printf("Empty region, nothing to kill.\n");
        return;  // Empty region, nothing to kill
    }

    // Allocate and copy the region to kill
    char *cut_text = malloc(region_length + 1);
    if (cut_text) {
        memcpy(cut_text, buffer->content + start, region_length);
        cut_text[region_length] = '\0';
        kill(kr, cut_text);  // Add to kill ring
        free(cut_text);  // Free temporary text buffer
    }

    // Remove the region text from the buffer
    memmove(buffer->content + start, buffer->content + end, buffer->size - end + 1);
    buffer->size -= region_length;

    // Update cursor position to start of the killed region
    buffer->point = start;

    // Deactivate region after killing it
    buffer->region.active = false;
}

void yank(Buffer *buffer, KillRing *kr) {
    if (kr->size == 0) return;

    int yankIndex = (kr->index - 1 + kr->capacity) % kr->capacity;
    char *textToYank = kr->entries[yankIndex];

    if (textToYank != NULL) {
        for (int i = 0; textToYank[i] != '\0'; ++i) {
            insertChar(buffer, textToYank[i]);
        }
    }
}

void kill_ring_save(Buffer *buffer, KillRing *kr) {
    if (!buffer->region.active) return;

    size_t start = buffer->region.start;
    size_t end = buffer->region.end;

    // Ensure start is always less than end
    if (start > end) {
        size_t temp = start;
        start = end;
        end = temp;
    }

    if (end > buffer->size) end = buffer->size;  // Clamp end to buffer size

    size_t region_length = end - start;
    if (region_length == 0) return;  // Empty region, nothing to save

    char *text_to_save = malloc(region_length + 1);
    if (text_to_save) {
        memcpy(text_to_save, buffer->content + start, region_length);
        text_to_save[region_length] = '\0';
        kill(kr, text_to_save);
        free(text_to_save);
    } else {
        fprintf(stderr, "Failed to allocate memory for kill ring save.\n");
    }
    buffer->region.active = false;
}





void duplicate_line(Buffer *buffer) {
    if (buffer == NULL || buffer->content == NULL) return;

    size_t lineStart = buffer->point;
    size_t lineEnd = buffer->point;

    // Move lineStart to the beginning of the line
    while (lineStart > 0 && buffer->content[lineStart - 1] != '\n') {
        lineStart--;
    }

    // Move lineEnd to the end of the line (including the newline character if present)
    while (lineEnd < buffer->size && buffer->content[lineEnd] != '\n') {
        lineEnd++;
    }

    // Include the newline character in duplication if it exists
    bool hasNewLine = (lineEnd < buffer->size && buffer->content[lineEnd] == '\n');
    if (hasNewLine) {
        lineEnd++;
    }

    size_t lineLength = lineEnd - lineStart;

    // If duplicating the last line which does not end with a newline, add it first
    if (!hasNewLine && lineEnd == buffer->size) {
        // Ensure there is capacity for the newline
        if (buffer->size + 1 > buffer->capacity) {
            buffer->capacity = buffer->size + 2; // Just need one more byte for '\n'
            char *newContent = realloc(buffer->content, buffer->capacity);
            if (newContent == NULL) return; // Allocation failed
            buffer->content = newContent;
        }

        buffer->content[buffer->size] = '\n';
        buffer->size++;
        lineEnd++;
        lineLength++; // Now includes the newly added newline
    }

    // Ensure there is enough capacity in the buffer for duplication
    if (buffer->size + lineLength > buffer->capacity) {
        buffer->capacity = (buffer->size + lineLength) * 2;
        char *newContent = realloc(buffer->content, buffer->capacity);
        if (newContent == NULL) return; // Allocation failed
        buffer->content = newContent;
    }

    // Shift the text after lineEnd to make space for the duplicate line
    memmove(buffer->content + lineEnd + lineLength, buffer->content + lineEnd, buffer->size - lineEnd);

    // Copy the line to duplicate
    memcpy(buffer->content + lineEnd, buffer->content + lineStart, lineLength);

    // Update buffer size
    buffer->size += lineLength;

    // Null-terminate the buffer
    buffer->content[buffer->size] = '\0';
}



bool isWordChar(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

bool isPunctuationChar(char c) {
    // Checks common punctuation used in programming and text
    return strchr(",.;:!?'\"(){}[]<>-+*/&|^%$#@~", c) != NULL;
}

bool forward_word(Buffer *buffer, int count) {
    if (buffer == NULL || buffer->content == NULL) return false;

    size_t pos = buffer->point;
    size_t end = buffer->size;
    int direction = count > 0 ? 1 : -1;

    while (pos < end && count != 0) {
        if (direction > 0) { // Moving forward
            // Skip initial spaces and punctuation
            while (pos < end && (isspace((unsigned char)buffer->content[pos]) || isPunctuationChar(buffer->content[pos])))
                pos++;

            // Move through the word
            while (pos < end && isWordChar(buffer->content[pos])) 
                pos++;

            // If the next character is punctuation, stop before it
            if (pos < end && isPunctuationChar(buffer->content[pos])) 
                break;
        } else { // Moving backward
            // Skip initial spaces and punctuation when moving backward
            while (pos > 0 && (isspace((unsigned char)buffer->content[pos - 1]) || isPunctuationChar(buffer->content[pos - 1])))
                pos--;

            // Move to the start of the word
            while (pos > 0 && isWordChar(buffer->content[pos - 1]))
                pos--;
        }
        count -= direction;
    }

    buffer->point = pos; // Update the cursor position
    return true;
}

bool backward_word(Buffer *buffer, int count) {
    if (buffer == NULL || buffer->content == NULL) return false;
    
    // Ensure the cursor starts within the buffer bounds
    if (buffer->point >= buffer->size) {
        buffer->point = buffer->size - 1;
    }

    // Ensure cursor is moved to a valid position if it starts right at a non-character
    if (!isWordChar(buffer->content[buffer->point]) && !isspace(buffer->content[buffer->point]) && buffer->point > 0) {
        buffer->point--;
    }

    return forward_word(buffer, -count);
}


void backward_kill_word(Buffer *buffer, KillRing *kr) {
    if (buffer == NULL || buffer->content == NULL || buffer->point == 0) return;

    size_t end = buffer->point;
    size_t start = end;

    // Skip any whitespace characters directly before the cursor
    while (start > 0 && isspace((unsigned char)buffer->content[start - 1])) {
        start--;
    }

    // Move start backward until a space is encountered or it reaches the start of the buffer
    while (start > 0 && !isspace((unsigned char)buffer->content[start - 1])) {
        start--;
    }

    size_t lengthToDelete = end - start;
    if (lengthToDelete == 0) return; // No word to delete

    // Copy the word that will be killed
    char* killed_text = malloc(lengthToDelete + 1);
    if (killed_text) {
        memcpy(killed_text, buffer->content + start, lengthToDelete);
        killed_text[lengthToDelete] = '\0';

        // Add the killed text to the kill ring
        kill(kr, killed_text); // Assume this function handles the addition to the kill ring

        // Free the allocated memory for killed text
        free(killed_text);
    }

    // Remove the word from the buffer by shifting the remaining characters
    memmove(buffer->content + start, buffer->content + end, buffer->size - end + 1); // Including null terminator

    // Update the size of the buffer
    buffer->size -= lengthToDelete;

    // Update the cursor position
    buffer->point = start;
}


void forward_paragraph(Buffer *buffer) {
    if (buffer == NULL || buffer->content == NULL || buffer->point >= buffer->size) return;

    size_t pos = buffer->point;
    bool is_empty_line = false;

    // Scan from the current position
    while (pos < buffer->size) {
        if (buffer->content[pos] == '\n') {
            size_t next_line_start = pos + 1;
            if (next_line_start < buffer->size && buffer->content[next_line_start] == '\n') {
                // Found an empty line
                buffer->point = next_line_start;
                return;
            }
        }
        pos++;
    }
    // If no empty line is found, move to end of buffer
    buffer->point = buffer->size;
}

void backward_paragraph(Buffer *buffer) {
    if (buffer == NULL || buffer->content == NULL || buffer->point == 0) return;

    size_t pos = buffer->point - 1;  // Start from one character before the current cursor position to check current line first
    bool found_empty_line = false;

    // Scan backward from the current position
    while (pos > 0) {
        if (buffer->content[pos] == '\n' && buffer->content[pos - 1] == '\n') {
            // Found an empty line
            buffer->point = pos;
            return;
        }
        pos--;
    }

    // If no empty line is found, move to start of buffer
    buffer->point = 0;
}
