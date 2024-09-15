#include <lume.h>
#include "theme.h"
#include "buffer.h"
#include "edit.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <dirent.h>

// TODO message(), and FIX it while the minibuffer is active [message] and fix the key cleanup
// TODO isearch-backward and color unmatched characters
// TODO scrolling, and recenter_top_bottom
// TODO syntax highlighting
// TODO unhardcode the keybinds
// TODO check if the lastmatchindex changes and stop search (like for moving)
// TODO Multiline in the minibuffer
// TODO Window manager
// TODO save-buffer
// TODO M-x
// TODO rainbow-mode
// TODO ) inside () shoudl jumpt to the closing one not move right once
// TODO add sx and sy parameters to drawCursor()
// FIXME use setBufferContent() to set the prompt aswell

// TODO IMPORTANT Don't fetch for the same buffer multiple times
// per frame, do it only once and pass it arround.

void find_file(BufferManager *bm);
char* autocomplete_path(const char* input);
void fetch_completions(const char* input);


// ENGINE
// TODO Subpixel font rendering

typedef struct {
    Buffer *searchBuffer;
    size_t lastMatchIndex;
    size_t startIndex;
    bool searching;
    char *lastSearch;
    bool wrap;
} ISearch;

typedef struct {
    char** items;        // Array of completion strings
    int count;           // Number of completions
    int currentIndex;    // Current index in the completion list
    bool isActive;       // Is completion active
} Completion;

Completion completion = {0};


ISearch isearch = {0};
BufferManager bm = {0};
KillRing kr = {0};

bool electric_pair_mode = true; // TODO Wrap selection for () [] {} '' ""
bool blink_cursor_mode = true;
float blink_cursor_delay = 0.5; // Seconds of idle time before the first blink of the cursor.
float blink_cursor_interval = 0.5; // Lenght of cursor blink interval in seconds.
int blink_cursor_blinks = 10; // How many times to blink before stopping.
int indentation = 4;
bool show_paren_mode = true;
float show_paren_delay = 0.125; // Time in seconds to delay before showing a matching paren.
int kill_ring_max = 120; // Maximum length of kill ring before oldest elements are thrown away.

void drawCursor(Buffer *buffer, Font *font, float x, float y, Color color);
void isearch_forward(Buffer *buffer, Buffer *minibuffer, bool updateStartIndex);

void highlightMatchingBrackets(Buffer *buffer, Font *font, Color highlightColor);
void highlightAllOccurrences(Buffer *buffer, const char *searchText, Font *font, Color highlightColor);
void drawHighlight(Buffer *buffer, Font *font, size_t pos, size_t length, Color highlightColor);

void drawRegion(Buffer *buffer, Font *font, Color regionColor);


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


int sw = 1920;
int sh = 1080;

int main() {
    initThemes();

    initWindow(sw, sh, "*scratch* - Glemax");
    registerTextInputCallback(textInputHandler);
    registerKeyInputCallback(keyInputHandler);

    Font *font = loadFont("jetb.ttf", 15); // 15
    Font *minifont = loadFont("jetb.ttf", 15); // 15

    initKillRing(&kr, kill_ring_max);
    initBufferManager(&bm);
    newBuffer(&bm, "minibuffer", "~/");
    newBuffer(&bm, "prompt", "~/");
    newBuffer(&bm, "*scratch*", "~/");
    bm.lastBuffer = getBuffer(&bm, "*scratch*");

    
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

        // PROMPT TEXT
        drawTextEx(minifont, prompt->content,
                   0, minifont->ascent - minifont->descent * 1.3,
                   1.0, 1.0, CT.minibuffer_prompt, CT.bg, -1, cursorVisible);


        useShader("simple");
        float minibufferHeight = minifont->ascent + minifont->descent;
        float modelineHeight = 25.0;

        drawRectangle((Vec2f){0, minibufferHeight}, (Vec2f){sw, modelineHeight}, CT.modeline); //21

        highlightMatchingBrackets(currentBuffer, font, CT.show_paren_match);
        if (isearch.searching) highlightAllOccurrences(currentBuffer, minibuffer->content, font, CT.isearch_highlight);

        updateRegion(currentBuffer, currentBuffer->point);
        drawRegion(currentBuffer, font, CT.region);


        if (isCurrentBuffer(&bm, "minibuffer")) {
            drawCursor(currentBuffer, minifont, promptWidth, 0 + minifont->descent,
                       CT.cursor);
        } else {
            drawCursor(currentBuffer, font, 0, sh - font->ascent, CT.cursor);
        }

        flush();

        // BUFFER TEXT
        beginScissorMode((Vec2f){0, minibufferHeight + modelineHeight}, (Vec2f) {sw, sh - minibufferHeight});
        if (!isCurrentBuffer(&bm, "minibuffer")) {
            drawTextEx(font, currentBuffer->content,
                       0, sh - font->ascent + font->descent, 1.0, 1.0,
                       WHITE, CT.bg,
                       currentBuffer->point, cursorVisible);
        } else {
            drawTextEx(font, bm.lastBuffer->content,
                       0, sh - font->ascent + font->descent, 1.0, 1.0,
                       WHITE, CT.bg,
                       -1, cursorVisible);
            
        }
        endScissorMode();

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
    freeKillRing(&kr);
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
    Buffer *prompt = getBuffer(&bm, "prompt");

    bool shiftPressed = mods & GLFW_MOD_SHIFT;
    bool ctrlPressed = mods & GLFW_MOD_CONTROL;
    bool altPressed = mods & GLFW_MOD_ALT;

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {

        case KEY_BACKSPACE:
            if (buffer->region.active && !isearch.searching) {
                kill_region(buffer, &kr);
                
            } else if (isearch.searching) {
                if (altPressed || ctrlPressed) {
                    backward_kill_word(minibuffer, &kr);
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
                    backward_kill_word(buffer, &kr);
                } else {
                    backspace(buffer);
                }
            }
            break;

        case KEY_SPACE:
            if (ctrlPressed) {
                if (!buffer->region.active) {
                    activateRegion(buffer);
                    buffer->region.marked = true;
                } else {
                    deactivateRegion(buffer);
                    buffer->region.marked = false;
                }
            }
            break;

        case KEY_ENTER:
            if (buffer->region.active) buffer->region.active = false;
            if (isearch.searching) {
                isearch.lastSearch = strdup(minibuffer->content);
                minibuffer->size = 0;
                minibuffer->point = 0;
                minibuffer->content[0] = '\0';
                isearch.searching = false;
                prompt->content = strdup("");
            } else if (strcmp(prompt->content, "Find file: ") == 0) {
                find_file(&bm);
                minibuffer->size = 0;
                minibuffer->point = 0;
                minibuffer->content[0] = '\0';
                prompt->content = strdup("");
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

        case KEY_Y:
            if (ctrlPressed) yank(buffer, &kr);
            break;

        case KEY_C:
            if (ctrlPressed) find_file(&bm);
            break;

        case KEY_X:
            if (ctrlPressed) {
                message(&bm, "TEST!");
            }
            break;

        case KEY_S:
            if (ctrlPressed) {
                if (!isearch.searching) {
                    isearch.searching = true;
                    minibuffer->size = 0;
                    minibuffer->content[0] = '\0';
                    isearch.startIndex = buffer->point;
                    prompt->content = strdup("I-search: ");
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

        case KEY_W:
            if (ctrlPressed) {
                kill_region(buffer, &kr);
            } else if (altPressed) {
                kill_ring_save(buffer, &kr);                
            }
            break;

        case KEY_G:
            if (ctrlPressed){
                buffer->region.active = false;
                buffer->region.marked = false;
                if (isearch.searching) {
                    buffer->point = isearch.startIndex;
                    minibuffer->size = 0;
                    minibuffer->point = 0;
                    minibuffer->content[0] = '\0';
                    isearch.searching = false;
                    prompt->content = strdup("");
                } else {
                    minibuffer->size = 0;
                    minibuffer->point = 0;
                    minibuffer->content[0] = '\0';
                    prompt->content = strdup("");
                    switchToBuffer(&bm, bm.lastBuffer->name);
                }
            }
            break;
                
        case KEY_I:
            if (ctrlPressed) {
                if (shiftPressed) {
                    removeIndentation(buffer, indentation);                    
                } else {
                    addIndentation(buffer, indentation);                    
                }
            }
            break;
        case KEY_6:
            if (altPressed && shiftPressed) delete_indentation(buffer);
            break;

            
        case KEY_TAB:
            if (isCurrentBuffer(&bm, "minibuffer") && strcmp(prompt->content, "Find file: ") == 0) {
                if (!completion.isActive || strcmp(minibuffer->content, completion.items[completion.currentIndex]) != 0) {
                    fetch_completions(minibuffer->content);
                    completion.currentIndex = 0; // Start from the first completion.
                } else {
                    if (shiftPressed) {
                        // Move to the previous completion, wrapping around if necessary.
                        if (completion.currentIndex == 0) {
                            completion.currentIndex = completion.count - 1;
                        } else {
                            completion.currentIndex--;
                        }
                    } else {
                        // Cycle through the completions.
                        completion.currentIndex = (completion.currentIndex + 1) % completion.count;
                    }
                }

                // Set the minibuffer content to the current completion and update necessary fields.
                if (completion.count > 0) {
                    setBufferContent(minibuffer, completion.items[completion.currentIndex]);
                }
            } else {
                indent(buffer);
            }
            break;
        case KEY_DOWN:
            next_line(buffer, shiftPressed);
            break;
        case KEY_UP:
            previous_line(buffer, shiftPressed);
            break;
        case KEY_LEFT:
            left_char(buffer, shiftPressed);
            break;
        case KEY_RIGHT:
            right_char(buffer, shiftPressed);
            break;
        case KEY_DELETE:
            delete_char(buffer);
            break;
        case KEY_N:
            if (ctrlPressed) {
                next_line(buffer, shiftPressed);                
            } else if (altPressed) {
                forward_paragraph(buffer);
            }

            break;
        case KEY_P:
            if (ctrlPressed) {
                previous_line(buffer, shiftPressed);                
            } else if (altPressed) {
                backward_paragraph(buffer);
            }

            break;
        case KEY_F:
            if (ctrlPressed) {
                right_char(buffer, shiftPressed);
            } else if (altPressed) {
                forward_word(buffer, 1);
            }
             
            break;
        case KEY_B:
            if (ctrlPressed) {
                left_char(buffer, shiftPressed);                
            } else if (altPressed) {
                backward_word(buffer, 1);
            }

            break;
        case KEY_E:
            if (ctrlPressed) move_end_of_line(buffer, shiftPressed);
            break;
        case KEY_A:
            if (ctrlPressed) move_beginning_of_line(buffer, shiftPressed);
            break;
        case KEY_HOME:
            move_beginning_of_line(buffer, shiftPressed);
            break;
        case KEY_D:
            if (ctrlPressed) delete_char(buffer);
            break;
        case KEY_K:
            if (ctrlPressed) {
                if (buffer->region.active) {
                    kill_region(buffer, &kr);
                } else {
                    kill_line(buffer, &kr);
                }
            }
            break;
        case KEY_O:
            if (buffer->region.active) buffer->region.active = false;
            if (ctrlPressed && shiftPressed) {
                duplicate_line(buffer);
            } else if (ctrlPressed) {
                open_line(buffer);
            }
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
    Buffer *minibuffer = getBuffer(&bm, "minibuffer");
    
    if (buffer != NULL) {
        buffer->region.active = false;
        if (isearch.searching) {
            if (isprint(codepoint)) {
                insertChar(minibuffer, (char)codepoint);
                isearch_forward(buffer, minibuffer, false);
            }
        } else {
            // Normal behavior when not in search mode
            if ((codepoint == ')' || codepoint == ']' || codepoint == '}' || 
                 codepoint == '\'' || codepoint == '\"') &&
                buffer->point < buffer->size && buffer->content[buffer->point] == codepoint) {
                right_char(buffer, false);
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
    int bytes = encodeUTF8(utf8, codepoint); // Function to convert codepoint to UTF-8
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
        buffer->point = matchIndex + strlen(minibuffer->content);
        isearch.lastMatchIndex = matchIndex + 1;
        if (updateStartIndex) {
            isearch.startIndex = buffer->point;
        }
        isearch.wrap = false;
    } else {
        if (!isearch.wrap) {
            printf("Reached end of buffer. Press Ctrl+S again to wrap search.\n");
            isearch.wrap = true;
        } else {
            if (updateStartIndex) {
                isearch.startIndex = 0;
                isearch.wrap = false;
                isearch_forward(buffer, minibuffer, false);
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

/* void drawHighlight(Buffer *buffer, Font *font, size_t pos, size_t length, Color highlightColor) { */
/*     float x = 0, y = 0; */
/*     int lineCount = 0; */
    
/*     // Calculate position in pixels where to start the highlight */
/*     for (int i = 0; i < pos; i++) { */
/*         if (buffer->content[i] == '\n') { */
/*             lineCount++; */
/*             x = 0; */
/*         } else { */
/*             x += getCharacterWidth(font, buffer->content[i]); */
/*         } */
/*     } */

/*     // Calculate the width of the highlighted area */
/*     float highlightWidth = 0; */
/*     for (int i = pos; i < pos + length && buffer->content[i] != '\n'; i++) { */
/*         highlightWidth += getCharacterWidth(font, buffer->content[i]); */
/*     } */

/*     y = lineCount * (font->ascent + font->descent); */
/*     y = getScreenHeight() - y - font->ascent - font->descent; */

/*     Vec2f position = {x, y}; */
/*     Vec2f size = {highlightWidth, font->ascent + font->descent}; */

/*     drawRectangle(position, size, highlightColor); */
/* } */


void drawHighlight(Buffer *buffer, Font *font, size_t startPos, size_t length, Color highlightColor) {
    float x = 0, y = 0;
    int lineCount = 0;
    
    for (size_t i = 0; i < startPos; i++) {
        if (buffer->content[i] == '\n') {
            lineCount++;
            x = 0;
        } else {
            x += getCharacterWidth(font, buffer->content[i]);
        }
    }

    float highlightWidth = 0;
    for (size_t i = startPos; i < startPos + length && i < buffer->size; i++) {
        if (buffer->content[i] == '\n') break;
        highlightWidth += getCharacterWidth(font, buffer->content[i]);
    }

    y = lineCount * (font->ascent + font->descent);
    y = getScreenHeight() - y - font->ascent - font->descent;

    Vec2f position = {x, y};
    Vec2f size = {highlightWidth, font->ascent + font->descent};
    drawRectangle(position, size, highlightColor);
}


void drawRegion(Buffer *buffer, Font *font, Color regionColor) {
    if (!buffer->region.active) return;

    size_t start = buffer->region.start;
    size_t end = buffer->region.end;
    // Ensure the start is less than end for consistency
    if (start > end) {
        size_t temp = start;
        start = end;
        end = temp;
    }

    float x = 0, y = 0;
    int lineCount = 0;
    bool inRegion = false;

    // Iterate over each character in the buffer
    for (size_t i = 0; i < buffer->size; i++) {
        char ch = buffer->content[i];

        if (i == start) {
            // Start of the region
            inRegion = true;
            x = 0; // Reset x at the start of the region
            for (size_t j = 0; j < i; j++) { // Recalculate x for the start of the region
                if (buffer->content[j] == '\n') {
                    lineCount++;
                    x = 0; // Reset x at each new line
                } else {
                    x += getCharacterWidth(font, buffer->content[j]);
                }
            }
        }

        if (inRegion) {
            // Calculate the width of the highlight for the current line
            float highlightWidth = 0;
            size_t segmentStart = i;
            while (i < end && buffer->content[i] != '\n' && i < buffer->size) {
                highlightWidth += getCharacterWidth(font, buffer->content[i]);
                i++;
            }

            // Calculate y position
            y = lineCount * (font->ascent + font->descent);
            y = getScreenHeight() - y - font->ascent - font->descent;

            // Draw highlight for the current line segment
            Vec2f position = {x, y};
            Vec2f size = {highlightWidth, font->ascent + font->descent};
            drawRectangle(position, size, regionColor);

            // If a newline is encountered, adjust for the next line
            if (buffer->content[i] == '\n') {
                lineCount++;
                x = 0; // Reset x for the next line
            } else {
                x += getCharacterWidth(font, buffer->content[i]);
            }

            // Break if the end of the region has been reached
            if (i >= end) {
                inRegion = false;
            }
        }
    }
}


// TODO Dired when calling find_file on a directory
// TODO Create files when they don't exist (and directories to get to that file)

void find_file(BufferManager *bm) {
    Buffer *minibuffer = getBuffer(bm, "minibuffer");
    Buffer *prompt = getBuffer(bm, "prompt");

    if (minibuffer->size == 0) {
        if (bm->lastBuffer && bm->lastBuffer->path) {
            minibuffer->size = 0;
            minibuffer->content[0] = '\0';
            minibuffer->point = 0;
            setBufferContent(minibuffer, bm->lastBuffer->path);
        }
        free(prompt->content);
        prompt->content = strdup("Find file: ");
        switchToBuffer(bm, "minibuffer");
        return;
    }

    const char *homeDir = getenv("HOME");
    char fullPath[PATH_MAX];
    const char *filePath = minibuffer->content;

    // Resolve full path
    if (filePath[0] == '~') {
        if (homeDir) {
            snprintf(fullPath, sizeof(fullPath), "%s%s", homeDir, filePath + 1);
        } else {
            fprintf(stderr, "Environment variable HOME is not set.\n");
            return;
        }
    } else {
        strncpy(fullPath, filePath, sizeof(fullPath) - 1);
        fullPath[sizeof(fullPath) - 1] = '\0'; // Ensure null termination
    }

    FILE *file = fopen(fullPath, "r");
    if (file) {
        // Creating buffer with '~' notation for user-friendly display
        char displayPath[PATH_MAX];
        if (strncmp(fullPath, homeDir, strlen(homeDir)) == 0) {
            snprintf(displayPath, sizeof(displayPath), "~%s", fullPath + strlen(homeDir));
        } else {
            strncpy(displayPath, fullPath, sizeof(displayPath));
            displayPath[sizeof(displayPath) - 1] = '\0';
        }

        newBuffer(bm, displayPath, displayPath);
        Buffer *fileBuffer = getBuffer(bm, displayPath);

        if (fileBuffer) {
            fileBuffer->content = malloc(sizeof(char) * 1024);
            fileBuffer->capacity = 1024;
            fileBuffer->size = 0;

            char buffer[1024]; // Temporary buffer for reading file content
            size_t bytesRead;
            while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                if (fileBuffer->size + bytesRead >= fileBuffer->capacity) {
                    fileBuffer->capacity = (fileBuffer->size + bytesRead) * 2;
                    char *newContent = realloc(fileBuffer->content, fileBuffer->capacity);
                    if (!newContent) {
                        free(fileBuffer->content);
                        fclose(file);
                        fprintf(stderr, "Failed to allocate memory for file content.\n");
                        return;
                    }
                    fileBuffer->content = newContent;
                }
                memcpy(fileBuffer->content + fileBuffer->size, buffer, bytesRead);
                fileBuffer->size += bytesRead;
            }
            fileBuffer->content[fileBuffer->size] = '\0';
            fclose(file);
            switchToBuffer(bm, fileBuffer->name);
        } else {
            fprintf(stderr, "Failed to retrieve or create buffer for file: %s\n", displayPath);
            fclose(file);
        }
    } else {
        fprintf(stderr, "File does not exist: %s\n", fullPath);
    }
}




void fetch_completions(const char* input) {
    DIR* dir;
    struct dirent* entry;
    char fullPath[PATH_MAX];
    char dirPath[PATH_MAX];
    const char* homeDir = getenv("HOME");

    // Resolve path, considering '~' and extracting directory path
    if (input[0] == '~') {
        snprintf(fullPath, PATH_MAX, "%s%s", homeDir, input + 1);
    } else {
        strncpy(fullPath, input, PATH_MAX);
    }
    fullPath[PATH_MAX - 1] = '\0';

    char* lastSlash = strrchr(fullPath, '/');
    if (lastSlash) {
        memcpy(dirPath, fullPath, lastSlash - fullPath);
        dirPath[lastSlash - fullPath] = '\0';
    } else {
        strcpy(dirPath, fullPath);  // Full path is the directory if no slash
    }

    dir = opendir(dirPath);
    if (!dir) {
        perror("Failed to open directory");
        return;
    }

    // Free previous completions
    for (int i = 0; i < completion.count; i++) {
        free(completion.items[i]);
    }
    free(completion.items);
    completion.items = NULL;
    completion.count = 0;

    // Collect new completions
    while ((entry = readdir(dir)) != NULL) {
        const char* lastPart = lastSlash ? lastSlash + 1 : input;
        if (strncmp(entry->d_name, lastPart, strlen(lastPart)) == 0) {
            char formattedPath[PATH_MAX];

            // Format the path with a tilde for user-friendly display
            if (entry->d_type == DT_DIR) {
                snprintf(formattedPath, PATH_MAX, "%s/%s/", dirPath, entry->d_name);
            } else {
                snprintf(formattedPath, PATH_MAX, "%s/%s", dirPath, entry->d_name);
            }

            if (strncmp(formattedPath, homeDir, strlen(homeDir)) == 0) {
                snprintf(formattedPath, PATH_MAX, "~%s", formattedPath + strlen(homeDir));
            }

            completion.items = realloc(completion.items, sizeof(char*) * (completion.count + 1));
            completion.items[completion.count++] = strdup(formattedPath);
        }
    }

    closedir(dir);
    completion.isActive = true;
    completion.currentIndex = -1; // Reset index for new session
}
