#include <lume.h>
#include "theme.h"
#include "buffer.h"
#include "wm.h"
#include "edit.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "completion.h"
#include "isearch.h"
#include "faces.h"
#include "keychords.h"
#include "history.h"
#include "syntax.h"

// TODO IMPORTANT drawHighlight shoudl consider the scroll now
// TODO streamline the drawRegion function possibly using drawHighlight to unify things
// TODO we should add a Modeline struct to each window
// TODO we should add cursor struct to each window
// TODO we should add scrollbar struct to each window
// TODO we should add 2 fringes to each window
// TODO the point should be per window not per buffer

// TODO mark for each Buffer

// FIXME most functions that take the "Font *font" shoudl take only the wm now
// TODO dabbrev-completion
// TODO scrolling, and recenter_top_bottom
// TODO rainbow-mode
// TODO rainbow-delimiters-mode
// TODO revert-buffer-mode
// TODO wdired
// TODO iedit
// FIXME it segfault when i complete a non existing match, after i complete one more time
// TODO add a modeline struct to the window struct
// TODO draw hollow cursor on non active widnows

// TODO nextBuffer() and previousBuffer() should update the wm->buffer also switchToBuffer
// Do this when we implement the minibuffer as a window
// NOTE I-searrch for "if (isCurrentBuffer(&bm, "minibuffer")"

// TODO color unmatched characters in isearch and fix is behaviour like pressing backspaces etc..
// TODO unhardcode the keybinds
// TODO check if the lastmatchindex changes and stop search (like for moving)
// TODO M-x
// TODO undo system

// TODO ) inside () shoudl jumpt to the closing one not move right once
// TODO add sx and sy parameters to drawCursor()
// FIXME use setBufferContent() to set the prompt aswell

// TODO IMPORTANT Don't fetch for the same buffer multiple times per frame,
// do it only once and pass it arround. (we did it by using the wm ?)

CompletionEngine ce = {0};
ISearch isearch = {0};
BufferManager bm = {0};
KillRing kr = {0};
WindowManager wm = {0};
NamedHistories nh = {0};

bool electric_pair_mode = true; // TODO Wrap selection for () [] {} '' ""
bool blink_cursor_mode = true;
float blink_cursor_delay = 0.5; // Seconds of idle time before the first blink of the cursor.
float blink_cursor_interval = 0.5; // Lenght of cursor blink interval in seconds.
int blink_cursor_blinks = 10; // How many times to blink before stopping.
int indentation = 4;
bool show_paren_mode = true;
float show_paren_delay = 0.125; // TODO Time in seconds to delay before showing a matching paren.
int kill_ring_max = 120; // Maximum length of kill ring before oldest elements are thrown away.
bool draw_region_on_empty_lines = true;
bool electric_indent_mode = true;
bool rainbow_mode = true;
bool crystal_cursor_mode = true; // Make the cursor crystal clear


void drawMiniCursor(Buffer *buffer, Font *font, float x, float y, Color color);
void drawCursor(Buffer *buffer, Window *win, Color defaultColor);
void drawHighlight(WindowManager *wm, Font *font, size_t startPos, size_t length, Color highlightColor);
void highlightMatchingBrackets(WindowManager *wm, Font *font, Color highlightColor);
void highlightAllOccurrences(WindowManager *wm, const char *searchText, Font *font, Color highlightColor);
void drawRegion(WindowManager *wm, Font *font, Color regionColor);
void drawModeline(WindowManager *wm, Font *font, float minibufferHeight, Color color);
void drawBuffer(Window *win, Buffer *buffer, bool cursorVisible, bool colorPoint);
void highlightHexColors(WindowManager *wm, Font *font, Buffer *buffer, bool rm);
int getGlobalArg(Buffer *argBuffer);
void handleScroll(Window *window);

void keyInputHandler(int key, int action, int mods);
void textInputHandler(unsigned int codepoint);
void insertUnicodeCharacter(Buffer * buffer, unsigned int codepoint, int arg);
int encodeUTF8(char *out, unsigned int codepoint);


static double lastBlinkTime = 0.0;  // Last time the cursor state changed
static bool cursorVisible = true;  // Initial state of the cursor visibility
static int blinkCount = 0;  // Counter for number of blinks

// TODO it shoudl take the wm not the win so we cal use it in the key callback
void drawCursor(Buffer *buffer, Window *win, Color defaultColor) {
    float cursorX = win->x;
    float cursorY = win->y;
    int lineCount = 0;

    // Calculate the cursor's x offset within the content of the buffer
    for (size_t i = 0; i < buffer->point; i++) {
        if (buffer->content[i] == '\n') {
            lineCount++;  // Increment line count at each newline
            cursorX = win->x;  // Reset cursor x to the start of the window on new lines
        } else {
            cursorX += getCharacterWidth(buffer->font, buffer->content[i]);  // Move cursor right by character width
        }
    }

    // Determine cursor width, handle newline or end of buffer cases
    float cursorWidth = (buffer->point < buffer->size && buffer->content[buffer->point] != '\n') ?
        getCharacterWidth(buffer->font, buffer->content[buffer->point]) :
        getCharacterWidth(buffer->font, ' ');  // Use a standard width if at the end of a line or buffer

    // Compute cursor's y position based on the number of lines, subtract from initial y and add the scroll
    cursorY += win->scroll.y - lineCount * (buffer->font->ascent + buffer->font->descent) - (buffer->font->descent * 2);

    // Create a rectangle representing the cursor
    Vec2f cursorPosition = {cursorX, cursorY};
    Vec2f cursorSize = {cursorWidth, buffer->font->ascent + buffer->font->descent};

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
            Color cursorColor = defaultColor;
            if (crystal_cursor_mode) {
                for (size_t i = 0; i < buffer->syntaxArray.used; i++) {
                    if (buffer->point >= buffer->syntaxArray.items[i].start && buffer->point < buffer->syntaxArray.items[i].end) {
                        cursorColor = buffer->syntaxArray.items[i].color;
                        break;
                    }
                }
            }
            drawRectangle(cursorPosition, cursorSize, cursorColor);  // Draw the cursor if visible
        }
    } else {
        drawRectangle(cursorPosition, cursorSize, defaultColor);  // Always draw cursor if not blinking
    }
}



// NOTE ORIGINAL
/* void drawCursor(Buffer *buffer, Font *font, Window *win, Color defaultColor) { */
/*     int lineCount = 0; */
/*     float cursorX = win->x; */

/*     // Calculate the cursor's x offset within the content of the buffer */
/*     for (size_t i = 0; i < buffer->point; i++) { */
/*         if (buffer->content[i] == '\n') { */
/*             lineCount++; */
/*             cursorX = win->x;  // Reset cursor x to the start of the window on new lines */
/*         } else { */
/*             cursorX += getCharacterWidth(font, buffer->content[i]); */
/*         } */
/*     } */

/*     // Determine cursor width, handling newline or end of buffer cases */
/*     float cursorWidth = (buffer->point < buffer->size && buffer->content[buffer->point] != '\n') ? */
/*         getCharacterWidth(font, buffer->content[buffer->point]) : */
/*         getCharacterWidth(font, ' '); */

/*     // Compute cursor's y position based on the number of lines and the window's y offset */
/*     float cursorY = win->y - lineCount * (font->ascent + font->descent) - font->descent*2; */

/*     Vec2f cursorPosition = {cursorX, cursorY}; */
/*     Vec2f cursorSize = {cursorWidth, font->ascent + font->descent}; */

/*     // Handle cursor blinking logic */
/*     if (blink_cursor_mode && blinkCount < blink_cursor_blinks) { */
/*         double currentTime = getTime(); */
/*         if (currentTime - lastBlinkTime >= (cursorVisible ? blink_cursor_interval : blink_cursor_delay)) { */
/*             cursorVisible = !cursorVisible; */
/*             lastBlinkTime = currentTime; */
/*             if (cursorVisible) { */
/*                 blinkCount++; */
/*             } */
/*         } */

/*         if (cursorVisible) { */
/*             Color cursorColor = defaultColor; */
/*             if (crystal_cursor_mode) { */
/*                 for (size_t i = 0; i < buffer->syntaxArray.used; i++) { */
/*                     if (buffer->point >= buffer->syntaxArray.items[i].start && buffer->point < buffer->syntaxArray.items[i].end) { */
/*                         cursorColor = buffer->syntaxArray.items[i].color; */
/*                         break; */
/*                     } */
/*                 } */
/*             } */
/*             drawRectangle(cursorPosition, cursorSize, cursorColor); */
/*         } */
/*     } else { */
/*         drawRectangle(cursorPosition, cursorSize, defaultColor);  // Always draw cursor if not blinking */
/*     } */
/* } */


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


int sw = 1920;
int sh = 1080;

int main() {
    initThemes();
    initWindow(sw, sh, "*scratch* - Glemax");
    registerTextCallback(textInputHandler);
    registerKeyCallback(keyInputHandler);
    
    font = loadFont(fontname, fontsize);

    initGlobalParser();
    initKillRing(&kr, kill_ring_max);
    initBufferManager(&bm);
    newBuffer(&bm, &wm, "minibuffer", "~/", fontname, sw, sh);
    newBuffer(&bm, &wm, "prompt",     "~/", fontname, sw, sh);
    newBuffer(&bm, &wm, "message",    "~/", fontname, sw, sh);
    newBuffer(&bm, &wm, "arg",        "~/", fontname, sw, sh);
    newBuffer(&bm, &wm, "*scratch*",  "~/", fontname, sw, sh);
    bm.lastBuffer = getBuffer(&bm, "*scratch*");

    sw = getScreenWidth();
    sh = getScreenHeight();

    initWindowManager(&wm, &bm, font, sw, sh);

    /* updateWindows(&wm, font, sw, sh); */
    
    while (!windowShouldClose()) {
        sw = getScreenWidth();
        sh = getScreenHeight();
        
        /* updateWindows(&wm, font, sw, sh); */
        /* reloadShaders(); */
        Buffer *prompt = getBuffer(&bm, "prompt");
        Buffer *minibuffer = getBuffer(&bm, "minibuffer");
        Buffer *message = getBuffer(&bm, "message");
        

        Window *win = wm.head;
        Window *activeWindow = wm.activeWindow;
        Buffer *currentBuffer = activeWindow->buffer;
        /* Buffer *currentBuffer = getActiveBuffer(&bm); */


        /* parseSyntax(currentBuffer); */
        /* displaySyntax(currentBuffer); */
        
        beginDrawing();
        clearBackground(CT.bg);



        drawFPS(font, 400.0, 400.0, RED);

        float promptWidth = 0;
        for (size_t i = 0; i < strlen(prompt->content); i++) {
            promptWidth += getCharacterWidth(minibuffer->font, prompt->content[i]);
        }


        int lineCount = 1; // Start with 1 to account for content without any newlines
        for (int i = 0; i < strlen(minibuffer->content); i++) {
            if (minibuffer->content[i] == '\n') {
                lineCount++;
            }
        }

        float lineHeight = (minibuffer->font->ascent + minibuffer->font->descent);

        // Set the height based on the number of lines found
        float minibufferHeight = lineHeight * lineCount;


        // PROMPT TEXT
        drawTextEx(minibuffer->font, prompt->content,
                   0, minibufferHeight - (minibuffer->font->ascent - minibuffer->font->descent),
                   1.0, 1.0, CT.minibuffer_prompt, CT.bg, -1, cursorVisible,
                   "text");


        // TODO the cursor should be within the scissors
        useShader("simple");
        if (isCurrentBuffer(&bm, "minibuffer")) {
            drawMiniCursor(minibuffer, minibuffer->font, promptWidth, minibufferHeight - minibuffer->font->ascent, CT.cursor);
        } else {
            drawCursor(currentBuffer, wm.activeWindow, CT.cursor);
        }
        flush();


        updateRegion(currentBuffer, currentBuffer->point);


        for (; win != NULL; win = win->next) {
            Buffer *buffer = win->buffer;
            bool bottom = isBottomWindow(&wm, win);
            float scissorStartY = win->y - win->height + minibufferHeight + win->modelineHeight;
            float scissorHeight = sh - scissorStartY;
            if (bottom) {
                scissorHeight += minibufferHeight;
            }

            drawModeline(&wm, minibuffer->font, minibufferHeight, CT.modeline);


            beginScissorMode((Vec2f){win->x, scissorStartY}, (Vec2f){win->width, scissorHeight});


            if (win == wm.activeWindow) {
                highlightHexColors(&wm, buffer->font, buffer, rainbow_mode);
                useShader("simple");
                drawRegion(&wm, buffer->font, CT.region);
                highlightMatchingBrackets(&wm, buffer->font, CT.show_paren_match);
                if (isearch.searching) highlightAllOccurrences(&wm, minibuffer->content, font, CT.isearch_highlight);
                flush();

                if (isCurrentBuffer(&bm, "minibuffer")) {
                    drawBuffer(win, buffer, cursorVisible, false);                    
                } else {
                    drawBuffer(win, buffer, cursorVisible, true);                    
                }
                handleScroll(win);

            } else {
                drawBuffer(win, buffer, cursorVisible, false);
            }
            endScissorMode();
        }
        

        float minibufferWidth = promptWidth;

        for (size_t i = 0; i < strlen(minibuffer->content); i++) {
            minibufferWidth += getCharacterWidth(minibuffer->font, minibuffer->content[i]);
        }

        // MINIBUFFER TEXT
        drawTextEx(minibuffer->font, minibuffer->content,
                   promptWidth, minibufferHeight - (minibuffer->font->ascent - minibuffer->font->descent),
                   1.0, 1.0, CT.text, CT.bg, minibuffer->point, cursorVisible,
                   "text");

        // MESSAGE TEXT
        float lastLineWidth = getCharacterWidth(minibuffer->font, 32);
        size_t lastLineStart = strrchr(minibuffer->content, '\n') ? strrchr(minibuffer->content, '\n') - minibuffer->content + 1 : 0;
        for (size_t i = lastLineStart; i < strlen(minibuffer->content); i++) {
            lastLineWidth += getCharacterWidth(minibuffer->font, minibuffer->content[i]);
        }

        float lastLineY = minibufferHeight - (lineHeight * lineCount) + getCharacterWidth(minibuffer->font, 32);
        drawTextEx(minibuffer->font, message->content,
                   promptWidth + lastLineWidth, lastLineY,
                   1.0, 1.0, CT.message, CT.bg, message->point, cursorVisible,
                   "text");
        

        endDrawing();
    }

    freeGlobalParser();
    freeFont(font);
    freeKillRing(&kr);
    freeBufferManager(&bm);
    freeWindowManager(&wm);
    closeWindow();

    return 0;
}


void keyInputHandler(int key, int action, int mods) {
    Window * win = wm.activeWindow;
    Buffer *buffer = isCurrentBuffer(&bm, "minibuffer") ? getBuffer(&bm, "minibuffer") : wm.activeWindow->buffer;
    
    Buffer *minibuffer = getBuffer(&bm, "minibuffer");
    Buffer *prompt = getBuffer(&bm, "prompt");
    Buffer *messageBuffer = getBuffer(&bm, "message");
    Buffer *argBuffer = getBuffer(&bm, "arg");
    int arg = getGlobalArg(argBuffer);


    bool shiftPressed = mods & GLFW_MOD_SHIFT;
    bool ctrlPressed = mods & GLFW_MOD_CONTROL;
    bool altPressed = mods & GLFW_MOD_ALT;
    

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {

        // NOTE Handle global arg
        if (ctrlPressed && (key >= KEY_0 && key <= KEY_9)) {
            char digit = '0' + (key - KEY_0);  // Convert key code to corresponding character
            insertChar(argBuffer, digit, arg);
            message(&bm, argBuffer->content);
            return;  // Skip further processing
        }

        if (!isCurrentBuffer(&bm, "minibuffer") && (ctrlPressed || altPressed)) {
            cleanBuffer(&bm, "minibuffer");

        }
        cleanBuffer(&bm, "message");
        
        if (ctrl_x_pressed && key != KEY_X && key != KEY_F && key != KEY_O && key != KEY_S) {
            ctrl_x_pressed = false;
        }

        cleanBuffer(&bm, "arg");
        
        switch (key) {
        case KEY_BACKSPACE:
            if (buffer->region.active && !isearch.searching) {
                kill_region(buffer, &kr);
                
            } else if (isearch.searching) {
                if (altPressed || ctrlPressed) {
                    backward_kill_word(minibuffer, &kr);
                } else {
                    backspace(minibuffer, electric_pair_mode);
                }
                if (minibuffer->size > 0) {
                    isearch_forward(buffer, &bm, minibuffer, false, &isearch);
                } else {
                    // If the minibuffer is empty, move the cursor back to where the search started
                    buffer->point = isearch.startIndex;
                    // isearch.searching = false;  NOTE Keep searching (like emacs)
                }
            } else {
                if (altPressed || ctrlPressed) {
                    backward_kill_word(buffer, &kr);
                } else {
                    backspace(buffer, electric_pair_mode);
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
            enter(buffer, &bm, &wm, minibuffer, prompt, &isearch, indentation, electric_indent_mode, sw, sh, &nh, arg);
            break;
        case KEY_Y:
            if (ctrlPressed) yank(buffer, &kr, arg);
            break;
        case KEY_Z:
            /* printfSyntaxTree() */
            /* message(&bm, "HELLOW"); */
            /* moveTo(buffer, 10, 300); */
            printSyntaxInfo(buffer);
            break;

        case KEY_X:
            if (ctrlPressed) {
                ctrl_x_pressed = true;
                printActiveWindowDetails(&wm);
            }
            break;

        case KEY_R:
            if (ctrlPressed) {
                if (!isearch.searching) {
                    isearch.searching = true;
                    minibuffer->size = 0;
                    minibuffer->content[0] = '\0';
                    isearch.lastMatchIndex = buffer->point;  // Start backward search from the current point
                    prompt->content = strdup("I-search backward: ");
                } else {
                    // If the minibuffer is empty and there was a previous search, reload it
                    if (minibuffer->size == 0 && isearch.lastSearch) {
                        if (minibuffer->content) free(minibuffer->content);
                        minibuffer->content = strdup(isearch.lastSearch);
                        minibuffer->size = strlen(minibuffer->content);
                        minibuffer->point = minibuffer->size;
                    }
                    isearch.lastMatchIndex = buffer->point;  // Set start point for the next backward search
                    isearch_backward(buffer, minibuffer, true, &isearch);  // Continue search backward
                }
            }
            break;


        case KEY_S:
            if (ctrlPressed) {
                if (ctrl_x_pressed) {
                    save_buffer(&bm, buffer);
                    ctrl_x_pressed = false;
                } else if (!isearch.searching) {
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
                    isearch_forward(buffer, &bm, minibuffer, true, &isearch);  // Continue search from new start index
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
                
                resetHistoryIndex(&nh, prompt->content); // NOTE prompt->content is the history name
                if (isearch.searching) {
                    buffer->point = isearch.startIndex;
                    cleanBuffer(&bm, "minibuffer");
                    cleanBuffer(&bm, "prompt");
                    isearch.searching = false;
                } else {
                    buffer->region.active = false;
                    buffer->region.marked = false;
                    cleanBuffer(&bm, "minibuffer");
                    cleanBuffer(&bm, "prompt");
                    switchToBuffer(&bm, bm.lastBuffer->name);
                    cleanBuffer(&bm, "message");
                }

            } else if (altPressed) {
                goto_line(&bm, &wm, sw, sh);
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
            if (altPressed && shiftPressed) delete_indentation(buffer, &bm, arg);
            break;
        case KEY_TAB:
            if (isCurrentBuffer(&bm, "minibuffer") && strcmp(prompt->content, "Find file: ") == 0) {
                if (!ce.isActive || strcmp(minibuffer->content, ce.items[ce.currentIndex]) != 0) {
                    fetch_completions(minibuffer->content, &ce);
                    ce.currentIndex = 0; // Start from the first ce.
                } else {
                    if (shiftPressed) {
                        // Move to the previous ce, wrapping around if necessary.
                        if (ce.currentIndex == 0) {
                            ce.currentIndex = ce.count - 1;
                        } else {
                            ce.currentIndex--;
                        }
                    } else {
                        // Cycle through the completions.
                        ce.currentIndex = (ce.currentIndex + 1) % ce.count;
                    }
                }

                // Set the minibuffer content to the current ce and update necessary fields.
                if (ce.count > 0) {
                    setBufferContent(minibuffer, ce.items[ce.currentIndex]);
                }
            } else {
                if (buffer->region.active) {
                    indent_region(buffer, &bm, indentation, arg);
                } else {
                    indent(buffer, indentation, &bm, arg);
                }
            }
            break;
        case KEY_DOWN:
            if (ctrlPressed) {
                forward_paragraph(buffer, shiftPressed);
            } else {
                next_line(buffer, shiftPressed, &bm);
            }
            break;
        case KEY_UP:
            if (ctrlPressed) {
                backward_paragraph(buffer, shiftPressed);
            } else {
                previous_line(buffer, shiftPressed, &bm);
            }
            break;
        case KEY_LEFT:
            if (ctrlPressed) {
                backward_word(buffer, 1, shiftPressed);
            } else {
                left_char(buffer, shiftPressed, &bm, arg);
            }
            break;
        case KEY_RIGHT:
            if (ctrlPressed) {
                forward_word(buffer, 1, shiftPressed);
            } else {
                right_char(buffer, shiftPressed, &bm, arg);
            }
            break;
        case KEY_DELETE:
            delete_char(buffer, &bm);
            break;
        case KEY_N:
           if (ctrlPressed && altPressed) {
                forward_list(buffer, arg);
            } else if (ctrlPressed) {
                next_line(buffer, shiftPressed, &bm);
            } else if (altPressed) {
                if (isCurrentBuffer(&bm, "minibuffer")) {
                    next_history_element(&nh, prompt->content, minibuffer, &bm);
                } else {
                    forward_paragraph(buffer, shiftPressed);
                }
            }
            break;
        case KEY_P:
            if (ctrlPressed && altPressed) {
                backward_list(buffer, arg);
            } else if (ctrlPressed) {
                previous_line(buffer, shiftPressed, &bm);
            } else if (altPressed) {
                if (isCurrentBuffer(&bm, "minibuffer")) {
                    previous_history_element(&nh, prompt->content, minibuffer, &bm);
                } else {
                    backward_paragraph(buffer, shiftPressed);
                }
            }
            break;
        case KEY_F:
            if (ctrlPressed && ctrl_x_pressed) {
                find_file(&bm, &wm, sw, sh);
            } else if (ctrlPressed) {
                right_char(buffer, shiftPressed, &bm, arg);
            } else if (altPressed) {
                forward_word(buffer, 1, shiftPressed);
            }
            break;
        case KEY_B:
            if (ctrlPressed) {
                left_char(buffer, shiftPressed, &bm, arg);
            } else if (altPressed) {
                backward_word(buffer, 1, shiftPressed);
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
            if (ctrlPressed) {
                if (buffer->region.active) {
                    kill_region(buffer, &kr);
                } else {
                    delete_char(buffer, &bm);
                }
            }
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
            } else if (ctrlPressed && shiftPressed) {
                if (buffer->region.active) buffer->region.active = false;
                duplicate_line(buffer);
            } else if (ctrlPressed) {
                if (ctrl_x_pressed) {
                    delete_blank_lines(buffer, arg);
                    ctrl_x_pressed = false;
                } else {
                    if (buffer->region.active) buffer->region.active = false;
                    open_line(buffer);
                }
            }
            break;

        case KEY_1:
            if (altPressed && shiftPressed) shell_command(&bm);
            break;

        case KEY_EQUAL:
            if (altPressed) {
                nextTheme();
            } else if (ctrlPressed) {
                if (isCurrentBuffer(&bm, "minibuffer")) {
                    increaseFontSize(minibuffer, fontname, &wm, sh, arg);                    
                } else {
                    increaseFontSize(buffer, fontname, &wm, sh, arg);                    
                }
            }
            break;
        case KEY_MINUS:
            if (altPressed) {
                previousTheme();
            } else if (ctrlPressed) {
                if (isCurrentBuffer(&bm, "minibuffer")) {
                    decreaseFontSize(minibuffer, fontname, &wm, sh, arg);                    
                } else {
                    decreaseFontSize(buffer, fontname, &wm, sh, arg);
                }
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
            } else if (ctrlPressed) {
                enter(buffer, &bm, &wm, minibuffer, prompt, &isearch, indentation, electric_indent_mode, sw, sh, &nh, arg);
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
    // TODO it shoudl take all those variables as parameters
    Window *win = wm.activeWindow;
    Buffer *buffer = win->buffer;
    Buffer *prompt = getBuffer(&bm, "prompt");
    Buffer *minibuffer = getBuffer(&bm, "minibuffer");

    Buffer *argBuffer = getBuffer(&bm, "arg");
    int arg = getGlobalArg(argBuffer);


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
        if (!isearch.searching) {
            buffer->region.active = false;            
        }

        if (isearch.searching) {
            if (isprint(codepoint)) {
                insertChar(minibuffer, (char)codepoint, arg);
                if (strcmp(prompt->content, "I-search backward: ") == 0) {
                    isearch_backward(buffer, minibuffer, false, &isearch);  // Continue search backward
                } else {
                    isearch_forward(buffer, &bm, minibuffer, false, &isearch);
                }

            }
        } else {
            // Normal behavior when not in search mode
            if ((codepoint == ')' || codepoint == ']' || codepoint == '}' || 
                 codepoint == '\'' || codepoint == '\"') &&
                buffer->point < buffer->size && buffer->content[buffer->point] == codepoint) {
                right_char(buffer, false, &bm, arg);
            } else {

                if (isCurrentBuffer(&bm, "minibuffer")) {
                    insertChar(minibuffer, codepoint, arg);
                } else {
                    insertChar(buffer, codepoint, arg);
                }


                if (electric_pair_mode) {
                    switch (codepoint) {
                    case '(':
                        insertChar(buffer, ')', arg);
                        break;
                    case '[':
                        insertChar(buffer, ']', arg);
                        break;
                    case '{':
                        insertChar(buffer, '}', arg);
                        break;
                    case '\'':
                        if (!(buffer->point > 1 && buffer->content[buffer->point - 2] == '\'')) {
                            insertChar(buffer, '\'', arg);
                        }
                        break;
                    case '\"':
                        if (!(buffer->point > 1 && buffer->content[buffer->point - 2] == '\"')) {
                            insertChar(buffer, '\"', arg);
                        }
                        break;
                    }

                    // Move the cursor back to between the pair of characters
                    if (codepoint == '(' || codepoint == '[' || codepoint == '{' ||
                        codepoint == '\'' || codepoint == '\"') {
                        buffer->point--;
                    }
                }
                if (electric_indent_mode && (codepoint == '}' || codepoint == ';')) {
                    indent(buffer, indentation, &bm, arg);
                }
            }
        }
    }
}

void insertUnicodeCharacter(Buffer * buffer, unsigned int codepoint, int arg) {
    char utf8[5]; // Buffer to hold UTF-8 encoded character
    int bytes = encodeUTF8(utf8, codepoint); // Function to convert codepoint to UTF-8
    for (int i = 0; i < bytes; i++) {
        insertChar(buffer, utf8[i], arg);
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
        drawHighlight(wm, wm->activeWindow->buffer->font, pos, searchLength, highlightColor);  // Updated to use WindowManager
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





// NOTE DrawRect version
void drawRegion(WindowManager *wm, Font *font, Color regionColor) {
    if (!wm || !wm->activeWindow || !wm->activeWindow->buffer) return;
    Buffer *buffer = wm->activeWindow->buffer;

    if (!buffer->region.active) return;

    size_t start = buffer->region.start;
    size_t end = buffer->region.end;
    if (start > end) {
        size_t temp = start;
        start = end;
        end = temp;
    }

    size_t currentLineStart = 0;
    float initialY = wm->activeWindow->y + font->ascent - font->descent * 2;
    float y = initialY;

    // Iterate over each character in the buffer to find line starts and ends
    for (size_t i = 0; i <= buffer->size; i++) {
        if (buffer->content[i] == '\n' || i == buffer->size) {  // End of line or buffer
            if (i >= start && currentLineStart <= end) {  // Check if the line contains the region
                size_t lineStart = (currentLineStart > start) ? currentLineStart : start;
                size_t lineEnd = (i < end) ? i : end;
                size_t lineLength = lineEnd - lineStart;

                if (lineLength > 0) {
                    float highlightWidth = 0;
                    float lineX = wm->activeWindow->x; // Reset lineX for this line's calculation

                    // Calculate the x offset up to the start of the line
                    for (size_t j = currentLineStart; j < lineStart; j++) {
                        lineX += getCharacterWidth(font, buffer->content[j]);
                    }

                    // Calculate the highlight width for the selected area
                    for (size_t j = lineStart; j < lineEnd; j++) {
                        highlightWidth += getCharacterWidth(font, buffer->content[j]);
                    }

                    // If the newline is part of the selection and cursor is not on this line, extend highlight to end of window
                    if (buffer->content[i] == '\n' && lineEnd == i && (end != i || i == buffer->size)) {
                        highlightWidth += wm->activeWindow->width - (lineX + highlightWidth);
                    }

                    Vec2f position = {lineX, y - font->ascent};  // Position adjusted for font ascent
                    Vec2f size = {highlightWidth, font->ascent + font->descent};  // Height adjusted for font size
                    drawRectangle(position, size, regionColor);  // Draw the rectangle
                }
            }
            currentLineStart = i + 1;
            y -= (font->ascent + font->descent);  // Move y to the next line
        }
    }
}

/* void drawRegion(WindowManager *wm, Font *font, Color regionColor) { */
/*     if (!wm || !wm->activeWindow || !wm->activeWindow->buffer) return; */
/*     Buffer *buffer = wm->activeWindow->buffer; */

/*     if (!buffer->region.active) return; */

/*     size_t start = buffer->region.start; */
/*     size_t end = buffer->region.end; */
/*     // Ensure the start is less than end for consistency */
/*     if (start > end) { */
/*         size_t temp = start; */
/*         start = end; */
/*         end = temp; */
/*     } */

/*     size_t currentLineStart = 0; */
/*     float x = 0; */
/*     int lineCount = 0; */

/*     // Iterate over each character in the buffer to find line starts and ends */
/*     for (size_t i = 0; i <= buffer->size; i++) { */
/*         if (buffer->content[i] == '\n' || i == buffer->size) {  // End of line or buffer */
/*             if (i >= start && currentLineStart <= end) {  // Check if the line contains the region */
/*                 // Using the ternary operator directly in place of max and min */
/*                 size_t lineStart = (currentLineStart > start) ? currentLineStart : start; */
/*                 size_t lineEnd = (i < end) ? i : end; */
/*                 size_t lineLength = lineEnd - lineStart; */

/*                 if (lineLength > 0) { */
/*                     drawHighlight(wm, font, lineStart, lineLength, regionColor);  // Use drawHighlight for each line segment */
/*                 } */
/*             } */
/*             currentLineStart = i + 1;  // Move to the start of the next line */
/*             lineCount++; */
/*         } */
/*     } */
/* } */


// TODO use isBottomWindow() for consistency
// TODO draw the modeline text
void drawModeline(WindowManager *wm, Font *font, float minibufferHeight, Color color) {
    for (Window *win = wm->head; win != NULL; win = win->next) {
        bool isBottom = true;  // Assume the window is at the bottom unless proven otherwise.
        for (Window *checkWin = wm->head; checkWin != NULL; checkWin = checkWin->next) {
            if (win != checkWin && win->x == checkWin->x && win->y - win->height == checkWin->y) {
                // There is a window directly below the current one
                isBottom = false;
                break;
            }
        }

        float modelineBaseY = win->y - win->height + wm->activeWindow->buffer->font->ascent - wm->activeWindow->buffer->font->descent;
        if (isBottom) {
            // Adjust for minibuffer height if this window is at the bottom
            modelineBaseY += minibufferHeight;
            modelineBaseY -= wm->activeWindow->buffer->font->ascent - wm->activeWindow->buffer->font->descent;
        }

        useShader("simple");
        float width = win->splitOrientation == VERTICAL ? win->width - 1 : win->width;
        drawRectangle((Vec2f){win->x, modelineBaseY},
                      (Vec2f){width, win->modelineHeight}, CT.modeline);

        flush();
    }
}


// TODO add a name field to the FONT struct 

void drawBuffer(Window *win, Buffer *buffer, bool cursorVisible, bool colorPoint) {
    Font *font = buffer->font;
    const char *text = buffer->content;
    float x = win->x;
    // Start from the current vertical scroll position, moving content upwards as it increases
    float y = win->y + win->scroll.y;  // Here, adding the scroll moves the initial line up
    size_t index = 0;
    size_t charIndex = 0;
    Color currentColor = CT.text;

    useShader("text");

    while (text[charIndex] != '\0') {
        if (text[charIndex] == '\n') {
            x = win->x;  // Reset to the start of the line
            y -= (font->ascent + font->descent);  // Move up to the next line
            charIndex++;
            continue;
        }

        if (cursorVisible && colorPoint && charIndex == buffer->point) {
            currentColor = CT.bg;
        } else if (index < buffer->syntaxArray.used && charIndex >= buffer->syntaxArray.items[index].start &&
                   charIndex < buffer->syntaxArray.items[index].end) {
            currentColor = buffer->syntaxArray.items[index].color;  // Apply syntax coloring
        } else {
            currentColor = CT.text;  // Default text color
        }

        drawChar(font, text[charIndex], x, y, 1.0, 1.0, currentColor);  // Draw each character

        x += getCharacterWidth(font, text[charIndex]);  // Advance x position
        charIndex++;  // Next character

        // Update syntax index when moving past the end of a highlighted section
        if (index < buffer->syntaxArray.used && charIndex == buffer->syntaxArray.items[index].end) {
            index++;
        }
    }

    flush();
}


/* void drawBuffer(Window *win, Buffer *buffer, bool cursorVisible) { */
/*     Font *font = buffer->font; */
/*     const char *text = buffer->content; */
/*     float x = win->x; */
/*     float y = win->y; */
/*     size_t index = 0; */
/*     size_t charIndex = 0; */
/*     Color currentColor = CT.text; */

/*     useShader("text");  // Assume there's a single shader handling text rendering */

/*     while (text[charIndex] != '\0') { */
/*         if (text[charIndex] == '\n') { */
/*             x = win->x; */
/*             y -= (font->ascent + font->descent); */
/*             charIndex++; */
/*             continue; */
/*         } */

/*         // Adjust color based on syntax highlighting or cursor presence */
/*         if (cursorVisible && charIndex == buffer->point) { */
/*             currentColor = CT.bg; */
/*         } else if (index < buffer->syntaxArray.used && charIndex >= buffer->syntaxArray.items[index].start && */
/*                    charIndex < buffer->syntaxArray.items[index].end) { */
/*             currentColor = buffer->syntaxArray.items[index].color; */
/*         } else { */
/*             currentColor = CT.text;  // Default text color */
/*         } */

/*         drawChar(font, text[charIndex], x, y, 1.0, 1.0, currentColor); */

/*         x += getCharacterWidth(font, text[charIndex]); */
/*         charIndex++; */

/*         // Update index to next syntax item when applicable */
/*         if (index < buffer->syntaxArray.used && charIndex == buffer->syntaxArray.items[index].end) { */
/*             index++; */
/*         } */
/*     } */

/*     flush();  // Ensure all drawn characters are rendered */
/* } */


/* void drawBuffer(Window *win, Buffer *buffer, bool cursorVisible) { */
/*     Font *font = buffer->font; */
/*     const char *text = buffer->content; */
/*     float x = win->x; */
/*     float y = win->y; */
/*     float sx = 1.0; */
/*     float sy = 1.0; */
/*     int highlightPos = buffer->point; */
/*     float lineHeight = (font->ascent + font->descent) * sy; */
/*     Color currentColor = CT.text; */
/*     char *currentShader = "text"; */

/*     useShader(currentShader); */

/*     for (int i = 0; text[i] != '\0'; i++) { */
/*         if (text[i] == '\n') { */
/*             x = win->x; */
/*             y -= lineHeight; */
/*             continue; */
/*         } */

/*         if (cursorVisible && i == highlightPos) { */
/*             if (currentShader != "text") { */
/*                 flush(); */
/*                 useShader("text"); */
/*                 currentShader = "text"; */
/*             } */
/*             currentColor = CT.bg; */
/*         } else if (buffer->region.active && i >= buffer->region.start && i <= buffer->region.end - 1) { */
/*             if (currentShader != "wave") { */
/*                 flush(); */
/*                 useShader("wave"); */
/*                 currentShader = "wave"; */
/*             } */
/*             /\* currentColor = RED; *\/ */
/*         } else { */
/*             if (currentShader != "text") { */
/*                 flush(); */
/*                 useShader("text"); */
/*                 currentShader = "text"; */
/*             } */
/*             currentColor = CT.text; */
/*         } */

/*         drawChar(font, text[i], x, y, sx, sy, currentColor); */
/*         x += getCharacterWidth(font, text[i]) * sx; */
/*     } */

/*     flush(); */
/* } */






#include <regex.h>

void highlightHexColors(WindowManager *wm, Font *font, Buffer *buffer, bool rm) {
    if (!rm || !wm || !wm->activeWindow || !buffer) return;
    useShader("simple");
    const char *pattern = "#([0-9A-Fa-f]{6}|[0-9A-Fa-f]{3})\\b";
    regex_t regex;
    regmatch_t matches[2]; // We expect one full match and one subgroup

    if (regcomp(&regex, pattern, REG_EXTENDED)) {
        fprintf(stderr, "Failed to compile regex.\n");
        return;
    }

    char *text = buffer->content;
    size_t offset = 0;

    while (regexec(&regex, text + offset, 2, matches, 0) == 0) {
        size_t match_start = offset + matches[0].rm_so;
        size_t match_end = offset + matches[0].rm_eo;

        char matchedString[8]; // Enough to hold the full pattern plus null terminator
        snprintf(matchedString, sizeof(matchedString), "%.*s", (int)(match_end - match_start), text + match_start);

        Color highlightColor = hexToColor(matchedString);
        drawHighlight(wm, font, match_start, match_end - match_start, highlightColor);

        offset = match_end; // Move past this match
    }
    flush();
    regfree(&regex);
}



int getGlobalArg(Buffer *argBuffer) {
    if (argBuffer->size == 0 || argBuffer->content[0] == '\0') {
        return 1;
    } else {
        char *endptr;
        int result = (int)strtol(argBuffer->content, &endptr, 10);
        if (*endptr != '\0') {
            // Handle case where non-numeric characters are present
            printf("Non-numeric input in argument buffer. Ignoring non-numeric part.\n");
            return result;
        }
        return result;
    }
}

#include <math.h>
void handleScroll(Window *window) {
    Buffer *buffer = window->buffer;
    Font *font = buffer->font;

    // Calculate the height of one line of text
    float lineHeight = font->ascent + font->descent;

    // Calculate the vertical position of the cursor in the buffer
    int cursorLine = 0;
    for (size_t i = 0; i < buffer->point && i < buffer->size; i++) {
        if (buffer->content[i] == '\n') {
            cursorLine++;
        }
    }
    float cursorY = cursorLine * lineHeight;

    // Define the top and bottom of the viewable area, considering the modeline
    float viewTop = window->scroll.y;
    float viewBottom = window->scroll.y + window->height - lineHeight - window->modelineHeight;

    // Check if the cursor is above the view top or below the view bottom
    if (cursorY < viewTop || cursorY > viewBottom) {
        // Center the cursor vertically by adjusting scrollY
        float newScrollY = cursorY - window->height / 2 + lineHeight / 2;

        // Adjust to round to the nearest line height, favoring a line less
        if (newScrollY >= 0) {
            newScrollY = ceil((newScrollY - lineHeight / 2) / lineHeight) * lineHeight;
        } else {
            newScrollY = floor((newScrollY + lineHeight / 2) / lineHeight) * lineHeight;
        }

        // Clamp the new scrollY to ensure it's within valid content range
        newScrollY = fmax(0, newScrollY); // Don't scroll past the top of the buffer
        newScrollY = fmin(newScrollY, buffer->size * lineHeight - window->height); // Don't scroll past the bottom of the buffer

        // Update the window's scroll position
        window->scroll.y = newScrollY;
    }
}


/* void handleScroll(Window *window) { */
/*     Buffer *buffer = window->buffer; */
/*     Font *font = buffer->font; */
    
/*     // Calculate the height of one line of text */
/*     float lineHeight = font->ascent + font->descent; */

/*     // Calculate the vertical position of the cursor in the buffer */
/*     int cursorLine = 0; */
/*     for (size_t i = 0; i < buffer->point && i < buffer->size; i++) { */
/*         if (buffer->content[i] == '\n') { */
/*             cursorLine++; */
/*         } */
/*     } */
/*     float cursorY = cursorLine * lineHeight; */

/*     // Define the top and bottom of the viewable area, considering the modeline */
/*     float viewTop = window->scroll.y; */
/*     float viewBottom = window->scroll.y + window->height - lineHeight - window->modelineHeight;  // Adjust to consider modeline */

/*     // Check if the cursor is above the view top or below the view bottom */
/*     if (cursorY < viewTop || cursorY > viewBottom) { */
/*         // Center the cursor vertically by adjusting scrollY */
/*         float newScrollY = cursorY - window->height / 2 + lineHeight / 2; */

/*         // Clamp the new scrollY to ensure it's within valid content range */
/*         newScrollY = fmax(0, newScrollY); // Don't scroll past the top of the buffer */
/*         newScrollY = fmin(newScrollY, buffer->size * lineHeight - window->height); // Don't scroll past the bottom of the buffer */

/*         // Update the window's scroll position */
/*         window->scroll.y = newScrollY; */
/*     } */
/* } */


/* void handleScroll(Window *window) { */
/*     Buffer *buffer = window->buffer; */
/*     Font *font = buffer->font; */
    
/*     // Calculate the height of one line of text */
/*     float lineHeight = font->ascent + font->descent; */

/*     // Calculate the vertical position of the cursor in the buffer */
/*     int cursorLine = 0; */
/*     for (size_t i = 0; i < buffer->point && i < buffer->size; i++) { */
/*         if (buffer->content[i] == '\n') { */
/*             cursorLine++; */
/*         } */
/*     } */
/*     float cursorY = cursorLine * lineHeight; */

/*     // Define the top and bottom of the viewable area, considering the modeline */
/*     float viewTop = window->scroll.y; */
/*     float viewBottom = window->scroll.y + window->height - lineHeight;  // Subtract lineHeight for the modeline height */

/*     // Check if the cursor is above the view top or below the view bottom */
/*     if (cursorY < viewTop || cursorY > viewBottom) { */
/*         // Center the cursor vertically by adjusting scrollY */
/*         float newScrollY = cursorY - window->height / 2 + lineHeight / 2; */

/*         // Clamp the new scrollY to ensure it's within valid content range */
/*         newScrollY = fmax(0, newScrollY); // Don't scroll past the top of the buffer */
/*         newScrollY = fmin(newScrollY, buffer->size * lineHeight - window->height); // Don't scroll past the bottom of the buffer */

/*         // Update the window's scroll position */
/*         window->scroll.y = newScrollY; */
/*     } */
/* } */


/* void handleScroll(Window *window) { */
/*     Buffer *buffer = window->buffer; */
/*     Font *font = buffer->font; */
    
/*     // Calculate the line height using the current font settings */
/*     float lineHeight = font->ascent + font->descent; */

/*     // Determine the cursor's vertical position by counting line breaks before the cursor */
/*     int cursorLine = 0; */
/*     for (size_t i = 0; i < buffer->point; i++) { */
/*         if (buffer->content[i] == '\n') { */
/*             cursorLine++; */
/*         } */
/*     } */
/*     float cursorY = cursorLine * lineHeight; */

/*     // Viewport boundaries considering the modeline's height if present */
/*     float viewTop = window->scroll.y; */
/*     float viewBottom = window->scroll.y + window->height - lineHeight; // Subtract modeline height if it affects viewable area */

/*     // Check if the cursor is outside the viewable area and adjust if necessary */
/*     if (cursorY < viewTop || cursorY >= viewBottom) { */
/*         // Center the cursor in the window */
/*         float newScrollY = cursorY - (window->height / 2) + (lineHeight / 2); */

/*         // Clamp the scroll position to valid values: no negative scroll and no scroll beyond content */
/*         newScrollY = fmax(0, newScrollY); */
/*         newScrollY = fmin(newScrollY, buffer->size * lineHeight - window->height); */

/*         // Apply the clamped scroll position */
/*         window->scroll.y = newScrollY; */
/*     } */
/* } */
