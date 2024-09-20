#include <lume.h>
#include "theme.h"
#include "buffer.h"
#include "wm.h"
#include "edit.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>

// TODO meomoize the fonts rasterize each index only once

// NOTE do this 4 next time
// TODO iedit
// FIXME it segfault when i complete a non existing match, after i complete one more time
// TODO add a modeline struct to the window struct
// TODO draw hollow cursor on non active widnows

// TODO nextBuffer() and previousBuffer() should update the wm->buffer also switchToBuffer
// Do this when we implement the minibuffer as a window
// NOTE I-searrch for "if (isCurrentBuffer(&bm, "minibuffer")"

// TODO message(), and FIX it while the minibuffer is active [message] and fix the key cleanup
// TODO IMPORTANT be able to message() and turn false ctrl_x_pressed in the key
// TODO isearch-backward and color unmatched characters
// TODO scrolling, and recenter_top_bottom
// TODO syntax highlighting
// TODO unhardcode the keybinds
// TODO check if the lastmatchindex changes and stop search (like for moving)
// TODO Multiline in the minibuffer
// TODO Window manager
// TODO save-buffer
// TODO M-x
// TODO modeline
// TODO undo system
// TODO rainbow-mode
// TODO ) inside () shoudl jumpt to the closing one not move right once
// TODO add sx and sy parameters to drawCursor()
// FIXME use setBufferContent() to set the prompt aswell

// TODO IMPORTANT Don't fetch for the same buffer multiple times
// per frame, do it only once and pass it arround. (we did it by using the wm ?)
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
WindowManager wm = {0};

bool electric_pair_mode = true; // TODO Wrap selection for () [] {} '' ""
bool blink_cursor_mode = true;
float blink_cursor_delay = 0.5; // Seconds of idle time before the first blink of the cursor.
float blink_cursor_interval = 0.5; // Lenght of cursor blink interval in seconds.
int blink_cursor_blinks = 10; // How many times to blink before stopping.
int indentation = 4;
bool show_paren_mode = true;
float show_paren_delay = 0.125; // Time in seconds to delay before showing a matching paren.
int kill_ring_max = 120; // Maximum length of kill ring before oldest elements are thrown away.

void drawMiniCursor(Buffer *buffer, Font *font, float x, float y, Color color);
void drawCursor(Buffer *buffer, Font *font, Window *win, Color color);
void isearch_forward(Buffer *buffer, Buffer *minibuffer, bool updateStartIndex);
void drawHighlight(WindowManager *wm, Font *font, size_t startPos, size_t length, Color highlightColor);
void highlightMatchingBrackets(WindowManager *wm, Font *font, Color highlightColor);
void highlightAllOccurrences(WindowManager *wm, const char *searchText, Font *font, Color highlightColor);
void drawRegion(WindowManager *wm, Font *font, Color regionColor);
void drawModeline(WindowManager *wm, Font *font, float minibufferHeight, float modelineHeight, Color color);
void find_file(BufferManager *bm);
char* autocomplete_path(const char* input);
void fetch_completions(const char* input);

void keyInputHandler(int key, int action, int mods);
void textInputHandler(unsigned int codepoint);
void insertUnicodeCharacter(Buffer *buffer, unsigned int codepoint);
int encodeUTF8(char *out, unsigned int codepoint);


static double lastBlinkTime = 0.0;  // Last time the cursor state changed
static bool cursorVisible = true;  // Initial state of the cursor visibility
static int blinkCount = 0;  // Counter for number of blinks

void drawCursor(Buffer *buffer, Font *font, Window *win, Color color) {
    int lineCount = 0;
    float cursorX = win->x;

    // Calculate the cursor's x offset within the content of the buffer
    for (size_t i = 0; i < buffer->point; i++) {
        if (buffer->content[i] == '\n') {
            lineCount++;
            cursorX = win->x;  // Reset cursor x to the start of the window on new lines
        } else {
            cursorX += getCharacterWidth(font, buffer->content[i]);
        }
    }

    // Determine cursor width, handling newline or end of buffer cases
    float cursorWidth = buffer->point < buffer->size && buffer->content[buffer->point] != '\n'
        ? getCharacterWidth(font, buffer->content[buffer->point])
        : getCharacterWidth(font, ' ');

    // Compute cursor's y position based on the number of lines and the window's y offset
    float cursorY = win->y - lineCount * (font->ascent + font->descent) - font->descent*2;

    Vec2f cursorPosition = {cursorX, cursorY};
    Vec2f cursorSize = {cursorWidth, font->ascent + font->descent};

    // Handle cursor blinking logic
    if (blink_cursor_mode && blinkCount < blink_cursor_blinks) {
        double currentTime = getTime();
        if (currentTime - lastBlinkTime >= (cursorVisible ? blink_cursor_interval : blink_cursor_delay)) {
            cursorVisible = !cursorVisible;
            lastBlinkTime = currentTime;
            if (cursorVisible) {
                blinkCount++;
            }
        }

        if (cursorVisible) {
            drawRectangle(cursorPosition, cursorSize, color);
        }
    } else {
        drawRectangle(cursorPosition, cursorSize, color);  // Always draw cursor if not blinking
    }
}


// TODO once we support the minibuffer as a window remove this function
void drawMiniCursor(Buffer *buffer, Font *font, float x, float y, Color color) {
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


bool isBottomWindow(WindowManager *wm, Window *window) {
    // Assuming vertical stacking of windows:
    for (Window *current = wm->head; current != NULL; current = current->next) {
        // Check if there is another window starting below the current one
        if (current != window && current->y > window->y) {
            return false;
        }
    }
    return true;
}

bool ctrl_x_pressed = false;

int sw = 1920;
int sh = 1080;

Font *font;
Font *minifont;

int fontincrement = 2.0; // NOTE not used yes use it to have precise control over the text size
int fontsize = 15;
int minifontsize = 15;
char *fontname = "fira.ttf";

int main() {
    initThemes();
    initWindow(sw, sh, "*scratch* - Glemax");
    registerTextInputCallback(textInputHandler);
    registerKeyInputCallback(keyInputHandler);
    
    font = loadFont(fontname, fontsize);
    minifont = loadFont(fontname, minifontsize);

    initKillRing(&kr, kill_ring_max);
    initBufferManager(&bm);
    newBuffer(&bm, &wm, "minibuffer", "~/", fontname);
    newBuffer(&bm, &wm, "prompt", "~/", fontname);
    newBuffer(&bm, &wm, "*scratch*", "~/", fontname);
    bm.lastBuffer = getBuffer(&bm, "*scratch*");

    sw = getScreenWidth();
    sh = getScreenHeight();

    initWindowManager(&wm, &bm, font, sw, sh);


    /* updateWindows(&wm, font, sw, sh); */

    while (!windowShouldClose()) {
        sw = getScreenWidth();
        sh = getScreenHeight();
        
        /* updateWindows(&wm, font, sw, sh); */
        Buffer *prompt = getBuffer(&bm, "prompt");
        Buffer *minibuffer = getBuffer(&bm, "minibuffer");

        Window *win = wm.head;
        Window *activeWindow = wm.activeWindow;
        Buffer *currentBuffer = activeWindow->buffer;
        /* Buffer *currentBuffer = getActiveBuffer(&bm); */


        beginDrawing();
        clearBackground(CT.bg);

        /* drawFPS(font, 400.0, 400.0, RED); */

        float promptWidth = 0;

        for (size_t i = 0; i < strlen(prompt->content); i++) {
            promptWidth += getCharacterWidth(minifont, prompt->content[i]);
        }

        // PROMPT TEXT
        drawTextEx(minifont, prompt->content,
                   0, minifont->ascent - minifont->descent * 1.3,
                   1.0, 1.0, CT.minibuffer_prompt, CT.bg, -1, cursorVisible);

        // MODELINE
        float minibufferHeight = minifont->ascent + minifont->descent;
        float modelineHeight = 25.0;


        // TODO the cursor should be within the scissors
        useShader("simple");
        if (isCurrentBuffer(&bm, "minibuffer")) {
            drawMiniCursor(minibuffer, minifont, promptWidth, 0 + minifont->descent, CT.cursor);
        } else {
            drawCursor(currentBuffer, currentBuffer->font, wm.activeWindow, CT.cursor);
        }
        flush();


        updateRegion(currentBuffer, currentBuffer->point);


        for (; win != NULL; win = win->next) {
            Buffer *buffer = win->buffer;
            bool bottom = isBottomWindow(&wm, win);
            float scissorStartY = win->y - win->height + minibufferHeight + modelineHeight;
            float scissorHeight = sh - scissorStartY;
            if (bottom) {
                scissorHeight += minibufferHeight;
            }

            drawModeline(&wm, font, minibufferHeight, modelineHeight, CT.modeline);
            beginScissorMode((Vec2f){win->x, scissorStartY}, (Vec2f){win->width, scissorHeight});


            if (win == wm.activeWindow) {
                useShader("simple");


                drawRegion(&wm, buffer->font, CT.region);
                highlightMatchingBrackets(&wm, font, CT.show_paren_match);
                if (isearch.searching) highlightAllOccurrences(&wm, minibuffer->content, font, CT.isearch_highlight);
                flush();


                drawTextEx(buffer->font, buffer->content,
                           win->x, win->y,
                           1.0, 1.0, CT.text, CT.bg,
                           buffer->point, cursorVisible);

            } else {
                drawTextEx(buffer->font, buffer->content,
                           win->x, win->y,
                           1.0, 1.0, CT.text, CT.bg,
                           -1, cursorVisible);
            }
            endScissorMode();
        }
        


        // MINIBUFFER TEXT
        if (isCurrentBuffer(&bm, "minibuffer") || isearch.searching) {
            drawTextEx(minifont, minibuffer->content,
                       promptWidth, minifont->ascent - minifont->descent * 1.3,
                       1.0, 1.0, CT.text, CT.bg, minibuffer->point, cursorVisible);
        /* } else { */
        /*     drawTextEx(minifont, minibuffer->content, */
        /*                promptWidth, minifont->ascent - minifont->descent * 1.3, */
        /*                1.0, 1.0, CT.text, CT.bg, -1, cursorVisible); */
        }



        endDrawing();
    }

    freeFont(font);
    freeFont(minifont);
    freeKillRing(&kr);
    freeBufferManager(&bm);
    freeWindowManager(&wm);
    closeWindow();

    return 0;
}


/* int main() { */
/*     initThemes(); */

/*     initWindow(sw, sh, "*scratch* - Glemax"); */
/*     registerTextInputCallback(textInputHandler); */
/*     registerKeyInputCallback(keyInputHandler); */

/*     Font *font = loadFont("jetb.ttf", 15); // 15 */
/*     Font *minifont = loadFont("jetb.ttf", 15); // 15 */

/*     initKillRing(&kr, kill_ring_max); */
/*     initBufferManager(&bm); */
/*     newBuffer(&bm, "minibuffer", "~/"); */
/*     newBuffer(&bm, "prompt", "~/"); */
/*     newBuffer(&bm, "*scratch*", "~/"); */
/*     bm.lastBuffer = getBuffer(&bm, "*scratch*"); */

/*     initWindowManager(&wm, &bm, sw, sh); */


/*     while (!windowShouldClose()) { */
/*         sw = getScreenWidth(); */
/*         sh = getScreenHeight(); */
/*         /\* reloadShaders(); *\/ */


/*         Buffer *minibuffer = getBuffer(&bm, "minibuffer"); */
/*         Buffer *prompt = getBuffer(&bm, "prompt"); */
/*         Buffer *currentBuffer = getActiveBuffer(&bm); */
        
/*         beginDrawing(); */
/*         clearBackground(CT.bg); */

/*         drawFPS(font, 400.0, 400.0, RED); */
        
/*         float promptWidth = 0; */

/*         for (size_t i = 0; i < strlen(prompt->content); i++) { */
/*             promptWidth += getCharacterWidth(minifont, prompt->content[i]); */
/*         } */

/*         // PROMPT TEXT */
/*         drawTextEx(minifont, prompt->content, */
/*                    0, minifont->ascent - minifont->descent * 1.3, */
/*                    1.0, 1.0, CT.minibuffer_prompt, CT.bg, -1, cursorVisible); */


/*         useShader("simple"); */
/*         float minibufferHeight = minifont->ascent + minifont->descent; */
/*         float modelineHeight = 25.0; */

/*         drawRectangle((Vec2f){0, minibufferHeight}, (Vec2f){sw, modelineHeight}, CT.modeline); //21 */

/*         highlightMatchingBrackets(currentBuffer, font, CT.show_paren_match); */
/*         if (isearch.searching) highlightAllOccurrences(currentBuffer, minibuffer->content, font, CT.isearch_highlight); */

/*         updateRegion(currentBuffer, currentBuffer->point); */
/*         drawRegion(currentBuffer, font, CT.region); */


/*         if (isCurrentBuffer(&bm, "minibuffer")) { */
/*             drawCursor(currentBuffer, minifont, promptWidth, 0 + minifont->descent, */
/*                        CT.cursor); */
/*         } else { */
/*             drawCursor(currentBuffer, font, 0, sh - font->ascent, CT.cursor); */
/*         } */

/*         flush(); */

/*         // BUFFER TEXT */
/*         beginScissorMode((Vec2f){0, minibufferHeight + modelineHeight}, (Vec2f) {sw, sh - minibufferHeight}); */
/*         if (!isCurrentBuffer(&bm, "minibuffer")) { */
/*             drawTextEx(font, currentBuffer->content, */
/*                        0, sh - font->ascent + font->descent, 1.0, 1.0, */
/*                        CT.text, CT.bg, */
/*                        currentBuffer->point, cursorVisible); */
/*         } else { */
/*             drawTextEx(font, bm.lastBuffer->content, */
/*                        0, sh - font->ascent + font->descent, 1.0, 1.0, */
/*                        CT.text, CT.bg, */
/*                        -1, cursorVisible); */
            
/*         } */
/*         endScissorMode(); */

/*         // MINIBUFFER TEXT */
/*         if (isCurrentBuffer(&bm, "minibuffer")) { */
/*             drawTextEx(minifont, minibuffer->content, */
/*                        promptWidth, minifont->ascent - minifont->descent * 1.3, */
/*                        1.0, 1.0, CT.text, CT.bg, currentBuffer->point, cursorVisible); */
/*         } else { */
/*             drawTextEx(minifont, minibuffer->content, */
/*                        promptWidth, minifont->ascent - minifont->descent * 1.3, */
/*                        1.0, 1.0, CT.text, CT.bg, -1, cursorVisible); */
/*         } */

/*         endDrawing(); */
/*     } */

/*     freeFont(font); */
/*     freeKillRing(&kr); */
/*     freeBufferManager(&bm); */
/*     freeWindowManager(&wm); */
/*     closeWindow(); */
/*     return 0; */
/* } */

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
    Window * win = wm.activeWindow;
    Buffer *buffer = isCurrentBuffer(&bm, "minibuffer") ? getBuffer(&bm, "minibuffer") : wm.activeWindow->buffer;
    
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

        case KEY_PERIOD:
            if (altPressed && shiftPressed) end_of_buffer(buffer);
            break;

        case KEY_COMMA:
            if (altPressed && shiftPressed) beginning_of_buffer(buffer);
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
                ctrl_x_pressed = false; // NOTE this is hardcoded because we cant reset ctrl_x_pressed
                // inside the key callback (for now) TODO
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
            /* if (ctrlPressed) find_file(&bm); */
            break;

        case KEY_X:
            if (ctrlPressed) {
                ctrl_x_pressed = true;
                printActiveWindowDetails(&wm);
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
                ctrl_x_pressed = false;
                
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
            if (ctrlPressed && ctrl_x_pressed) {
                find_file(&bm);
            } else if (ctrlPressed) {
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
        case KEY_Q:
            if (altPressed) {
                delete_window(&wm);
                /* updateWindows(&wm, font, sw, sh); */
            }
            break;
        case KEY_O:
            if (altPressed) {
                other_window(&wm, 1);
                printf("other window\n");
            } else if (ctrlPressed && shiftPressed) {
                if (buffer->region.active) buffer->region.active = false;
                duplicate_line(buffer);
            } else if (ctrlPressed) {
                if (buffer->region.active) buffer->region.active = false;
                open_line(buffer);
            }
            break;
        case KEY_EQUAL:
            if (altPressed) {
                nextTheme();   
            } else if (ctrlPressed) {
                increaseFontSize(buffer, fontname);
            }
            break;
        case KEY_MINUS:
            if (altPressed) {
                previousTheme();   
            } else if (ctrlPressed) {
                decreaseFontSize(buffer, fontname);
            }
            break;
        case KEY_L:
            if (altPressed) {
                split_window_right(&wm, font, sw, sh);
                other_window(&wm, 1);
            }
            break;
        case KEY_J:
            if (altPressed && wm.count <= 1) {
                split_window_below(&wm, font, sw, sw);
                other_window(&wm, 1);
            } else if (altPressed) {
                other_window(&wm, 1);
            }
            break;
        case KEY_H:
            if (altPressed && wm.count <= 1) {
                split_window_right(&wm, font, sw, sw);
            }
            break;

        case KEY_K:
            if (altPressed && wm.count <= 1) {
                split_window_below(&wm, font, sw, sw);
            } else if (altPressed) {
                other_window(&wm, -1);
            } else if (ctrlPressed) {
                if (buffer->region.active) {
                    kill_region(buffer, &kr);
                } else {
                    kill_line(buffer, &kr);
                }
            }
            break;
        case KEY_LEFT_BRACKET:
            if (altPressed) {
                nextBuffer(&bm);                
            }
            break;
        case KEY_RIGHT_BRACKET:
            if (altPressed) {
                previousBuffer(&bm);                
            }
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
    Window *win = wm.activeWindow;
    Buffer *buffer = win->buffer;

    /* Buffer *buffer = getActiveBuffer(&bm); */
    Buffer *minibuffer = getBuffer(&bm, "minibuffer");

    if (ctrl_x_pressed && codepoint == '2') {
        split_window_below(&wm, font, getScreenWidth(), getScreenHeight());
        ctrl_x_pressed = false;
        return;
    } else if (ctrl_x_pressed && codepoint == '3') {
        split_window_right(&wm, font, getScreenWidth(), getScreenHeight());
        ctrl_x_pressed = false;
        return;
    } else if (ctrl_x_pressed && codepoint == '0') {
        delete_window(&wm);
        ctrl_x_pressed = false;
        return;
    } 


    
    ctrl_x_pressed = false;
    
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

                if (isCurrentBuffer(&bm, "minibuffer")) {
                    insertUnicodeCharacter(minibuffer, codepoint);                    
                } else {
                    insertUnicodeCharacter(buffer, codepoint);                    
                }


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
void highlightMatchingBrackets(WindowManager *wm, Font *font, Color highlightColor) {
    if (!wm || !wm->activeWindow || !wm->activeWindow->buffer) return;
    if (!show_paren_mode) return;

    Buffer *buffer = wm->activeWindow->buffer;
    if (buffer->point > buffer->size) return;
    
    char currentChar = buffer->point < buffer->size ? buffer->content[buffer->point] : '\0';
    char prevChar = buffer->point > 0 ? buffer->content[buffer->point - 1] : '\0';
    char matchChar = '\0';
    int direction = 0;
    int searchPos = buffer->point;

    // Determine the direction to search based on the current or previous character
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
                // Draw highlights at the positions of the matching brackets
                drawHighlight(wm, font, buffer->point - (direction == -1 ? 1 : 0), 1, highlightColor);
                drawHighlight(wm, font, searchPos, 1, highlightColor);
                return;
            }
        }
        searchPos += direction;
    }
}


void highlightAllOccurrences(WindowManager *wm, const char *searchText, Font *font, Color highlightColor) {
    if (!wm || !wm->activeWindow || !wm->activeWindow->buffer) return;
    Buffer *buffer = wm->activeWindow->buffer;
    if (!searchText || strlen(searchText) == 0 || !buffer->content) return;

    size_t searchLength = strlen(searchText);
    const char *current = buffer->content;
    size_t pos = 0;

    while ((current = strstr(current, searchText)) != NULL) {
        pos = current - buffer->content;
        drawHighlight(wm, font, pos, searchLength, highlightColor);  // Updated to use WindowManager
        current += searchLength; // Move past the current match

        // Stop searching if the next search start is beyond buffer content
        if (current >= buffer->content + buffer->size) break;
    }
}


void drawHighlight(WindowManager *wm, Font *font, size_t startPos, size_t length, Color highlightColor) {
    if (!wm || !wm->activeWindow || !wm->activeWindow->buffer) return;

    Buffer *buffer = wm->activeWindow->buffer;
    Window *activeWindow = wm->activeWindow;
    
    float x = activeWindow->x;
    /* float y = activeWindow->y + font->descent + 1; */
    float y = activeWindow->y + font->ascent - font->descent * 2;
    int lineCount = 0;
    
    // Calculate initial x offset and y position up to startPos
    for (size_t i = 0; i < startPos && i < buffer->size; i++) {
        if (buffer->content[i] == '\n') {
            lineCount++;
            x = activeWindow->x; // Reset x to the start of the line at each new line
            y -= (font->ascent + font->descent); // Move up for each new line
        } else {
            x += getCharacterWidth(font, buffer->content[i]); // Accumulate width
        }
    }

    // Calculate the width of the highlighted area
    float highlightWidth = 0;
    for (size_t i = startPos; i < startPos + length && i < buffer->size; i++) {
        if (buffer->content[i] == '\n') break; // Stop if newline is encountered within highlight
        highlightWidth += getCharacterWidth(font, buffer->content[i]);
    }

    // Adjust y to be the lower left corner of the line to draw the highlight
    y -= font->ascent; 

    // Define the position and size of the highlight rectangle
    Vec2f position = {x, y};
    Vec2f size = {highlightWidth, font->ascent + font->descent};

    // Draw the highlight rectangle
    drawRectangle(position, size, highlightColor);
}

void drawRegion(WindowManager *wm, Font *font, Color regionColor) {
    if (!wm || !wm->activeWindow || !wm->activeWindow->buffer) return;
    Buffer *buffer = wm->activeWindow->buffer;

    if (!buffer->region.active) return;

    size_t start = buffer->region.start;
    size_t end = buffer->region.end;
    // Ensure the start is less than end for consistency
    if (start > end) {
        size_t temp = start;
        start = end;
        end = temp;
    }

    size_t currentLineStart = 0;
    float x = 0;
    int lineCount = 0;

    // Iterate over each character in the buffer to find line starts and ends
    for (size_t i = 0; i <= buffer->size; i++) {
        if (buffer->content[i] == '\n' || i == buffer->size) {  // End of line or buffer
            if (i >= start && currentLineStart <= end) {  // Check if the line contains the region
                // Using the ternary operator directly in place of max and min
                size_t lineStart = (currentLineStart > start) ? currentLineStart : start;
                size_t lineEnd = (i < end) ? i : end;
                size_t lineLength = lineEnd - lineStart;

                if (lineLength > 0) {
                    drawHighlight(wm, font, lineStart, lineLength, regionColor);  // Use drawHighlight for each line segment
                }
            }
            currentLineStart = i + 1;  // Move to the start of the next line
            lineCount++;
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

        newBuffer(bm, &wm, displayPath, displayPath, fontname);
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

// TODO use isBottomWindow() for consistency
// TODO draw the modeline text
void drawModeline(WindowManager *wm, Font *font, float minibufferHeight, float modelineHeight, Color color) {
    for (Window *win = wm->head; win != NULL; win = win->next) {
        bool isBottom = true;  // Assume the window is at the bottom unless proven otherwise.
        for (Window *checkWin = wm->head; checkWin != NULL; checkWin = checkWin->next) {
            if (win != checkWin && win->x == checkWin->x && win->y - win->height == checkWin->y) {
                // There is a window directly below the current one
                isBottom = false;
                break;
            }
        }

        float modelineBaseY = win->y - win->height + font->ascent - font->descent;
        if (isBottom) {
            // Adjust for minibuffer height if this window is at the bottom
            modelineBaseY += minibufferHeight;
            modelineBaseY -= font->ascent - font->descent;
        }

        // Use shader for drawing
        useShader("simple");

        // Adjust width if vertical split
        float width = win->splitOrientation == VERTICAL ? win->width - 1 : win->width;

        // Draw modeline at calculated position
        drawRectangle((Vec2f){win->x, modelineBaseY},
                      (Vec2f){width, modelineHeight}, CT.modeline);

        // MODELINE
        // drawRectangle((Vec2f){win->x, modelineBaseY},
        //               (Vec2f){10, modelineHeight}, RED);

        flush();
    }
}





