#include <lume.h>
#include "theme.h"
#include "buffer.h"
#include "edit.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
/* #include <string.h> */

// TODO Region
// TODO message(), and FIX it while the minibuffer is active [message]
// TODO isearch-backward and color unmatched characters
// TODO forward-paragraph
// TODO backward-paragraph
// TODO forward-word
// TODO backward-word
// TODO scrolling, and recenter_top_bottom
// TODO syntax highlighting
// TODO unhardcode the keybinds

// TODO IMPORTANT Don't fetch for the same buffer multiple times
// per frame, do it only once and pass it arround.


// ENGINE
// TODO Scissors

typedef struct {
    Buffer *searchBuffer;
    size_t lastMatchIndex;
    size_t startIndex;
    bool searching;
    char *lastSearch;
    bool wrap;
} ISearch;

ISearch isearch = {0};

bool electric_pair_mode = true;
bool blink_cursor_mode = true;
float blink_cursor_delay = 0.5; // Seconds of idle time before the first blink of the cursor.
float blink_cursor_interval = 0.5; // Lenght of cursor blink interval in seconds.
int blink_cursor_blinks = 10; // How many times to blink before stopping.
int indentation = 4;
bool show_paren_mode = true;
float show_paren_delay = 0.125; // Time in seconds to delay before showing a matching paren.

void drawCursor(Buffer *buffer, Font *font, float x, float y, Color color);
void isearch_forward(Buffer *buffer, Buffer *minibuffer, bool updateStartIndex);

void highlightMatchingBrackets(Buffer *buffer, Font *font, Color highlightColor);
void highlightAllOccurrences(Buffer *buffer, const char *searchText, Font *font, Color highlightColor);
void drawHighlight(Buffer *buffer, Font *font, size_t pos, size_t length, Color highlightColor);

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
    newBuffer(&bm, "prompt");
    newBuffer(&bm, "second");
    newBuffer(&bm, "main");

    
    while (!windowShouldClose()) {
        sw = getScreenWidth();
        sh = getScreenHeight();
        reloadShaders();


        Buffer *minibuffer = getBuffer(&bm, "minibuffer");
        Buffer *prompt = getBuffer(&bm, "prompt");
        Buffer *currentBuffer = getActiveBuffer(&bm);        
        
        beginDrawing();
        clearBackground(CT.bg);


        float promptWidth = 0;

        for (size_t i = 0; i < strlen(prompt->content); i++) {
            promptWidth += getCharacterWidth(minifont, prompt->content[i]);
        }

        bool set;
        if (isearch.searching) {
            if (!set) {
                prompt->content = strdup("isearch: ");
                set = true;
            }
        } else {
            if (set) {
                prompt->content = strdup("");
                set = false;
            }
        }

        // PROMPT TEXT
        drawTextEx(minifont, prompt->content,
                   0, minifont->ascent - minifont->descent * 1.3,
                   1.0, 1.0, CT.minibuffer_prompt, CT.bg, -1, cursorVisible);


        useShader("simple");
        drawRectangle((Vec2f){0, minifont->ascent + minifont->descent}, (Vec2f){sw, 25}, CT.modeline); //21

        highlightMatchingBrackets(currentBuffer, font, CT.show_paren_match);
        if (isearch.searching) highlightAllOccurrences(currentBuffer, minibuffer->content, font, CT.isearch_highlight);


        if (isCurrentBuffer(&bm, "minibuffer")) {
            drawCursor(currentBuffer, minifont, promptWidth, 0 + minifont->descent,
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
                       promptWidth, minifont->ascent - minifont->descent * 1.3,
                       1.0, 1.0, WHITE, CT.bg, currentBuffer->point, cursorVisible);
        } else {
            drawTextEx(minifont, minibuffer->content,
                       promptWidth, minifont->ascent - minifont->descent * 1.3,
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
                    isearch_forward(buffer, minibuffer, false);
                } else {
                    // If the minibuffer is empty, move the cursor back to where the search started
                    buffer->point = isearch.startIndex;
                    // isearch.searching = false;  NOTE Keep searching (like emacs)
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
                isearch.lastSearch = strdup(minibuffer->content);
                minibuffer->size = 0;
                minibuffer->point = 0;
                minibuffer->content[0] = '\0';
                isearch.searching = false;
                printf("[STOPPED ISEARCH]\n");
                printf("lastsearch: %s\n", isearch.lastSearch);
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
                if (!isearch.searching) {
                    isearch.searching = true;
                    minibuffer->size = 0;
                    minibuffer->content[0] = '\0';
                    isearch.startIndex = buffer->point;
                    printf("[STARTED ISEARCH]\n");
                } else {
                    // Ensures that we start the search from the right point even if minibuffer hasn't changed
                    if (minibuffer->size == 0 && isearch.lastSearch) {
                        if (minibuffer->content) free(minibuffer->content);
                        minibuffer->content = strdup(isearch.lastSearch);
                        minibuffer->size = strlen(minibuffer->content);
                        minibuffer->point = minibuffer->size;
                    }
                    isearch.startIndex = buffer->point;  // Update to ensure search starts from next position
                    isearch_forward(buffer, minibuffer, true);  // Continue search from new start index
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
                isearch_forward(buffer, minibuffer, false);      // Perform the search
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






void isearch_forward(Buffer *buffer, Buffer *minibuffer, bool updateStartIndex) {
    const char *start = buffer->content + isearch.startIndex;
    const char *found = strstr(start, minibuffer->content);

    if (found) {
        size_t matchIndex = found - buffer->content;
        buffer->point = matchIndex + strlen(minibuffer->content);  // Move cursor to end of found text
        isearch.lastMatchIndex = matchIndex + 1;  // Prepare for next search from next character
        if (updateStartIndex) {
            isearch.startIndex = buffer->point;  // Update start index for next search only if required
        }
        printf("Found '%s' at position %zu\n", minibuffer->content, matchIndex);
        isearch.wrap = false;  // Reset wrap around flag after a successful find
    } else {
        if (!isearch.wrap) {
            printf("Reached end of buffer. Press Ctrl+S again to wrap search.\n");
            isearch.wrap = true;  // Set flag to allow wrapping on next Ctrl+S press
        } else {
            if (updateStartIndex) {
                isearch.startIndex = 0;  // Wrap around the search
                isearch.wrap = false;  // Reset wrap around flag
                isearch_forward(buffer, minibuffer, false);  // Optionally, continue search from start
            }
        }
    }
}
 


// TODO use show_paren_delay
// TODO highlifght the 2 characters as well
void highlightMatchingBrackets(Buffer *buffer, Font *font, Color highlightColor) {
    if (!show_paren_mode) return;
    if (buffer->point > buffer->size) return;
    
    char currentChar = buffer->point < buffer->size ? buffer->content[buffer->point] : '\0';
    char prevChar = buffer->point > 0 ? buffer->content[buffer->point - 1] : '\0';
    char matchChar = '\0';
    int direction = 0;
    int searchPos = buffer->point;

    if (buffer->point < buffer->size && strchr("({[", currentChar)) {
        matchChar = currentChar == '(' ? ')' :
            currentChar == '[' ? ']' : '}';
        direction = 1;
    } else if (prevChar == ')' || prevChar == ']' || prevChar == '}') {
        currentChar = prevChar;
        matchChar = currentChar == ')' ? '(' :
            currentChar == ']' ? '[' : '{';
        direction = -1;
        searchPos = buffer->point - 1;
    } else {
        return;  // Not on or immediately after a bracket
    }

    int depth = 1;
    searchPos += direction;

    // Search for the matching bracket
    while (searchPos >= 0 && searchPos < buffer->size) {
        char c = buffer->content[searchPos];
        if (c == currentChar) {
            depth++;
        } else if (c == matchChar) {
            depth--;
            if (depth == 0) {
                drawHighlight(buffer, font, buffer->point - (direction == -1 ? 1 : 0), 1, highlightColor);
                drawHighlight(buffer, font, searchPos, 1, highlightColor);
                return;
            }

        }
        searchPos += direction;
    }
}


void highlightAllOccurrences(Buffer *buffer, const char *searchText, Font *font, Color highlightColor) {
    if (!searchText || strlen(searchText) == 0 || !buffer || !buffer->content) return;

    size_t searchLength = strlen(searchText);
    const char *current = buffer->content;
    size_t pos = 0;

    while ((current = strstr(current, searchText)) != NULL) {
        pos = current - buffer->content;
        drawHighlight(buffer, font, pos, searchLength, highlightColor);
        current += searchLength; // Move past the current match

        // Stop searching if the next search start is beyond buffer content
        if (current >= buffer->content + buffer->size) break;
    }
}

void drawHighlight(Buffer *buffer, Font *font, size_t pos, size_t length, Color highlightColor) {
    float x = 0, y = 0;
    int lineCount = 0;
    
    // Calculate position in pixels where to start the highlight
    for (int i = 0; i < pos; i++) {
        if (buffer->content[i] == '\n') {
            lineCount++;
            x = 0;
        } else {
            x += getCharacterWidth(font, buffer->content[i]);
        }
    }

    // Calculate the width of the highlighted area
    float highlightWidth = 0;
    for (int i = pos; i < pos + length && buffer->content[i] != '\n'; i++) {
        highlightWidth += getCharacterWidth(font, buffer->content[i]);
    }

    y = lineCount * (font->ascent + font->descent);
    y = getScreenHeight() - y - font->ascent - font->descent;

    Vec2f position = {x, y};
    Vec2f size = {highlightWidth, font->ascent + font->descent};

    drawRectangle(position, size, highlightColor);
}
