#include <lume.h>
#include "theme.h"
#include "buffer.h"
#include "edit.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
/* #include <string.h> */

// TODO Region
// TODO isearch-backward and color unmatched characters
// TODO forward-paragraph
// TODO backward-paragraph
// TODO forward-word
// TODO backward-word
// TODO scrolling, and recenter_top_bottom
// TODO syntax highlighting
// TODO unhardcode the keybinds

// TODO Don't fetch for the same buffer multiple times
// per frame, do it only once and pass it arround.


// ENGINE
// TODO Scissors

typedef struct {
    Buffer *searchBuffer;
    size_t lastMatchIndex;
    size_t startIndex;
    bool searching;
} ISearch;

ISearch isearch = {0};

bool electric_pair_mode = true;
bool blink_cursor_mode = true;
float blink_cursor_delay = 0.5; // Seconds of idle time before the first blink of the cursor.
float blink_cursor_interval = 0.5; // Lenght of cursor blink interval in seconds.
int blink_cursor_blinks = 10; // How many times to blink before stopping.
int indentation = 4;
    
void drawCursor(Buffer *buffer, Font *font, float x, float y, Color color);
void isearch_forward(Buffer *buffer, Buffer *minibuffer);

void keyInputHandler(int key, int action, int mods);
void textInputHandler(unsigned int codepoint);
void insertUnicodeCharacter(Buffer *buffer, unsigned int codepoint);
int encodeUTF8(char *out, unsigned int codepoint);



static double lastBlinkTime = 0.0;  // Last time the cursor state changed
static bool cursorVisible = true;  // Initial state of the cursor visibility
static int blinkCount = 0;  // Counter for number of blinks

void drawCursor(Buffer *buffer, Font *font, float x, float y, Color color) {
    int lineCount = 0;
    float cursorX = x;

    for (size_t i = 0; i < buffer->point; i++) {
        if (buffer->content[i] == '\n') {
            lineCount++;
            cursorX = x;
        } else {
            cursorX += getCharacterWidth(font, buffer->content[i]);
        }
    }

    float cursorWidth = buffer->point < buffer->size && buffer->content[buffer->point] != '\n'
        ? getCharacterWidth(font, buffer->content[buffer->point])
        : getCharacterWidth(font, ' ');

    Vec2f cursorPosition = {cursorX, y - lineCount * (font->ascent + font->descent) - font->descent};
    Vec2f cursorSize = {cursorWidth, font->ascent + font->descent};

    if (blink_cursor_mode && blinkCount < blink_cursor_blinks) {
        double currentTime = getTime();
        if (currentTime - lastBlinkTime >= (cursorVisible ? blink_cursor_interval : blink_cursor_delay)) {
            cursorVisible = !cursorVisible;
            lastBlinkTime = currentTime;
            if (cursorVisible) {
                blinkCount++;  // Increment only on visibility toggle to visible
            }
        }

        if (cursorVisible) {
            drawRectangle(cursorPosition, cursorSize, color);
        }
    } else {
        drawRectangle(cursorPosition, cursorSize, color);  // Always draw cursor if not blinking
    }
}


BufferManager bm = {0};

int main() {
    int sw = 1920;
    int sh = 1080;

    initThemes();

    initWindow(sw, sh, "main.c - Glemax");
    registerTextInputCallback(textInputHandler);
    registerKeyInputCallback(keyInputHandler);

    Font *font = loadFont("jetb.ttf", 40); // 14~16
    Font *minifont = loadFont("jetb.ttf", 22); // 16

    initBufferManager(&bm);
    newBuffer(&bm, "minibuffer");
    newBuffer(&bm, "second");
    newBuffer(&bm, "main");


    
    while (!windowShouldClose()) {
        sw = getScreenWidth();
        sh = getScreenHeight();
        reloadShaders();


        Buffer *minibuffer = getBuffer(&bm, "minibuffer");
        Buffer *currentBuffer = getActiveBuffer(&bm);        
        
        /* minibuffer->content = "porcodio"; */
            
        beginDrawing();
        clearBackground(CT.bg);


        useShader("simple");
        drawRectangle((Vec2f){0, minifont->ascent + minifont->descent}, (Vec2f){sw, 25}, CT.modeline); //21


        if (isCurrentBuffer(&bm, "minibuffer")) {
            drawCursor(currentBuffer, minifont, 0, 0 + minifont->descent,
                       CT.cursor);
        } else {
            drawCursor(currentBuffer, font, 0, sh - font->ascent, CT.cursor);
        }

        flush();
        
        // BUFFER TEXT
        if (!isCurrentBuffer(&bm, "minibuffer")) {
            drawTextEx(font, currentBuffer->content,
                       0, sh - font->ascent + font->descent, 1.0, 1.0,
                       WHITE, CT.bg,
                       currentBuffer->point, cursorVisible);
        } 

        // MINIBUFFER TEXT
        if (isCurrentBuffer(&bm, "minibuffer")) {
            drawTextEx(minifont, minibuffer->content,
                       0, + minifont->ascent - minifont->descent * 1.3,
                       1.0, 1.0, WHITE, CT.bg, currentBuffer->point, cursorVisible);
        } else {
            drawTextEx(minifont, minibuffer->content,
                       0, + minifont->ascent - minifont->descent * 1.3,
                       1.0, 1.0, WHITE, CT.bg, -1, cursorVisible);
        }
        
        endDrawing();
    }

    freeFont(font);
    freeBufferManager(&bm);
    closeWindow();
    return 0;
}

void backspace(Buffer *buffer) {
    if (buffer->point > 0 && electric_pair_mode) {
        // Check if backspacing over an opening character that has a closing pair right after
        unsigned int currentChar = buffer->content[buffer->point - 1];
        unsigned int nextChar = buffer->content[buffer->point];
        if ((currentChar == '(' && nextChar == ')') ||
            (currentChar == '[' && nextChar == ']') ||
            (currentChar == '{' && nextChar == '}') ||
            (currentChar == '\'' && nextChar == '\'') ||
            (currentChar == '\"' && nextChar == '\"')) {
            // Remove both characters
            memmove(buffer->content + buffer->point - 1, buffer->content + buffer->point + 1, buffer->size - buffer->point - 1);
            buffer->size -= 2;
            buffer->point--;
            buffer->content[buffer->size] = '\0';
            return;
        }
    }
    // Default backspace behavior when not deleting a pair
    if (buffer->point > 0) {
        buffer->point--;
        memmove(buffer->content + buffer->point, buffer->content + buffer->point + 1, buffer->size - buffer->point);
        buffer->size--;
        buffer->content[buffer->size] = '\0';
    }
}

void keyInputHandler(int key, int action, int mods) {
    Buffer *buffer = getActiveBuffer(&bm);
    Buffer *minibuffer = getBuffer(&bm, "minibuffer");

    bool shiftPressed = mods & GLFW_MOD_SHIFT;
    bool ctrlPressed = mods & GLFW_MOD_CONTROL;
    bool altPressed = mods & GLFW_MOD_ALT;

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {

        case KEY_BACKSPACE:
            if (isearch.searching) {
                if (altPressed || ctrlPressed) {
                    backward_kill_word(minibuffer);
                } else {
                    backspace(minibuffer);
                }
                if (minibuffer->size > 0) {
                    isearch_forward(buffer, minibuffer);
                } else {
                    // If the minibuffer is empty, move the cursor back to where the search started
                    buffer->point = isearch.startIndex;
                    /* isearch.searching = false; */
                }
            } else {
                if (altPressed || ctrlPressed) {
                    backward_kill_word(buffer);
                } else {
                    backspace(buffer);
                }
            }
            break;


        case KEY_ENTER:
            if (isearch.searching) {
                minibuffer->size = 0;
                minibuffer->point = 0;
                minibuffer->content[0] = '\0';
                isearch.searching = false;
                printf("[STOPPED ISEARCH]\n");
            } else {
                if (buffer->point > 0 && buffer->point < buffer->size &&
                    buffer->content[buffer->point - 1] == '{' && buffer->content[buffer->point] == '}') {
                    // Insert a newline and indent for the opening brace
                    insertChar(buffer, '\n');
                    indent(buffer);
        
                    // Record the position after the first indent, which is where the cursor should end up
                    size_t newCursorPosition = buffer->point;

                    // Insert another newline for the closing brace and indent again
                    insertChar(buffer, '\n');
                    indent(buffer);

                    // Move cursor to the line between the braces
                    buffer->point = newCursorPosition;
                } else {
                    insertChar(buffer, '\n');
                }
                indent(buffer);
            }
            break;

        case KEY_S:
            if (ctrlPressed) {
                if (!isearch.searching) { // Start a new search
                    isearch.searching = true;
                    printf("[STARTED ISEARCH]\n");
                    isearch.searchBuffer = minibuffer;
                    minibuffer->size = 0;
                    minibuffer->content[0] = '\0';
                    /* isearch.lastMatchIndex = buffer->point; */
                    isearch.startIndex = buffer->point; // Safely setting start index
                }
            }
            break;

        case KEY_6:
            if (altPressed && shiftPressed) delete_indentation(buffer);
            break;
        case KEY_TAB:
            indent(buffer);
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
        case KEY_HOME:
            move_beginning_of_line(buffer);
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
        case KEY_LEFT_BRACKET:
            if (altPressed) nextBuffer(&bm);
            break;
        case KEY_RIGHT_BRACKET:
            if (altPressed) previousBuffer(&bm);
            break;
        }
    }
    
    if (blink_cursor_mode) {
        blinkCount = 0;
        lastBlinkTime = getTime();
        cursorVisible = true;
    }
}

void textInputHandler(unsigned int codepoint) {
    Buffer *buffer = getActiveBuffer(&bm);
    Buffer *minibuffer = getBuffer(&bm, "minibuffer");  // Ensure this is properly initialized to act as the minibuffer

    if (buffer != NULL) {
        if (isearch.searching) {
            // When search mode is active, append characters to the minibuffer

            if (isprint(codepoint)) {
                insertChar(minibuffer, (char)codepoint);  // Append character to minibuffer
                isearch_forward(buffer, minibuffer);      // Perform the search
            }
        } else {
            // Normal behavior when not in search mode
            if ((codepoint == ')' || codepoint == ']' || codepoint == '}' || 
                 codepoint == '\'' || codepoint == '\"') &&
                buffer->point < buffer->size && buffer->content[buffer->point] == codepoint) {
                right_char(buffer);
            } else {
                insertUnicodeCharacter(buffer, codepoint);

                if (electric_pair_mode) {
                    switch (codepoint) {
                    case '(':
                        insertUnicodeCharacter(buffer, ')');
                        break;
                    case '[':
                        insertUnicodeCharacter(buffer, ']');
                        break;
                    case '{':
                        insertUnicodeCharacter(buffer, '}');
                        break;
                    case '\'':
                        if (!(buffer->point > 1 && buffer->content[buffer->point - 2] == '\'')) {
                            insertUnicodeCharacter(buffer, '\'');
                        }
                        break;
                    case '\"':
                        if (!(buffer->point > 1 && buffer->content[buffer->point - 2] == '\"')) {
                            insertUnicodeCharacter(buffer, '\"');
                        }
                        break;
                    }

                    // Move the cursor back to between the pair of characters
                    if (codepoint == '(' || codepoint == '[' || codepoint == '{' ||
                        codepoint == '\'' || codepoint == '\"') {
                        buffer->point--;
                    }
                }
            }
        }
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

void indent(Buffer *buffer) {
    size_t cursor_row_start = 0, cursor_row_end = buffer->size;
    int braceLevel = 0;
    bool startsWithClosingBrace = false;

    // Find the start of the current line
    for (int i = buffer->point - 1; i >= 0; i--) {
        if (buffer->content[i] == '\n') {
            cursor_row_start = i + 1;
            break;
        }
    }

    // Find the end of the current line
    for (size_t i = buffer->point; i < buffer->size; i++) {
        if (buffer->content[i] == '\n') {
            cursor_row_end = i;
            break;
        }
    }

    // Calculate the current brace level up to the start of the current line
    for (size_t i = 0; i < cursor_row_start; ++i) {
        char c = buffer->content[i];
        if (c == '{') {
            braceLevel++;
        } else if (c == '}') {
            braceLevel = (braceLevel > 0) ? braceLevel - 1 : 0;
        }
    }

    // Check if the current line starts with a '}' before any other non-whitespace character
    size_t firstNonWhitespace = cursor_row_start;
    while (firstNonWhitespace < cursor_row_end && isspace(buffer->content[firstNonWhitespace])) {
        firstNonWhitespace++;
    }
    if (firstNonWhitespace < cursor_row_end && buffer->content[firstNonWhitespace] == '}') {
        startsWithClosingBrace = true;
        braceLevel = (braceLevel > 0) ? braceLevel - 1 : 0;  // Decrement brace level for the line that starts with }
    }

    // Determine indentation level
    int requiredIndentation = braceLevel * indentation;
    int currentIndentation = 0;

    // Count existing spaces at the beginning of the line
    size_t i = cursor_row_start;
    while (i < cursor_row_end && isspace(buffer->content[i])) {
        if (buffer->content[i] == ' ') currentIndentation++;
        i++;
    }

    // Adjust indentation to the required level
    size_t old_point = buffer->point;  // Save old cursor position
    buffer->point = cursor_row_start; // Move cursor to the start of the line

    while (currentIndentation < requiredIndentation) {
        insertChar(buffer, ' '); // Insert additional spaces
        currentIndentation++;
    }
    while (currentIndentation > requiredIndentation && currentIndentation > 0) {
        delete_char(buffer); // Delete excess spaces
        currentIndentation--;
    }

    // Correct cursor position based on the previous position of non-whitespace text
    if (old_point >= firstNonWhitespace) {
        buffer->point = old_point - (firstNonWhitespace - cursor_row_start - requiredIndentation);
    } else {
        buffer->point = cursor_row_start + requiredIndentation;
    }
}


void isearch_forward(Buffer *buffer, Buffer *minibuffer) {
    if (!isearch.searching) {
        isearch.searching = true;
        isearch.searchBuffer = minibuffer;
        isearch.startIndex = buffer->point;
    }

    minibuffer->content[minibuffer->size] = '\0'; // Ensure null-termination

    char *match = strstr(buffer->content + isearch.startIndex, minibuffer->content);
    if (match) {
        size_t matchIndex = match - buffer->content;
        buffer->point = matchIndex + minibuffer->size; // Move cursor to the end of the match
        isearch.lastMatchIndex = matchIndex + 1; // Prepare for next incremental search
    } else {
        printf("No match found for '%s'\n", minibuffer->content);
        // If no match found, simply do not rollback minibuffer to allow adding more characters
        if (minibuffer->size > 0 && minibuffer->content[0] != '\0') {
            // Manage the state only if the minibuffer is not empty
            isearch.lastMatchIndex = buffer->point; // Reset search start for new inputs
        } else {
            // Reset everything if the minibuffer got cleared
            minibuffer->size = 0;
            minibuffer->content[0] = '\0';
            isearch.searching = false;
        }
    }
}
