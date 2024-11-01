#include "faces.h"
#include "wm.h"

// TODO actually implement faces we can have syntax highlighting and font weight and slant
// TODO implement a way to cicle between the fonts

Font *font;
Font *minifont;
int fontincrement = 2.0; // NOTE not used yes use it to have precise control over the text size
int fontsize = 15;
int minifontsize = 15;
char *fontname = "jetb.ttf";
/* char *fontname = "fira.ttf"; */
Font *globalFontCache[MAX_FONT_SCALE_INDEX] = {NULL};

void initScale(Scale *scale) {
    int sizes[] = {3, 4, 5, 6, 7, 9, 10, 12, 15, 18, 22, 26, 31, 37, 45, 54, 64, 77, 93, 111, 134, 161, 193, 231, 277, 333, 400, 480, 575, 690, 829, 994, 1194, 1432, 1718, 2062};
    for (int i = 0; i < MAX_FONT_SCALE_INDEX; i++) {
        scale->fontSizes[i] = sizes[i];
    }
    scale->index = SCALE_ZERO_INDEX;
}


Font* updateFont(Scale *scale, int newIndex, char *fontname) {
    if (newIndex < 0 || newIndex >= MAX_FONT_SCALE_INDEX) {
        fprintf(stderr, "Font scale index out of range!\n");
        return NULL;
    }
    scale->index = newIndex;
    if (!globalFontCache[scale->index]) {  // Check if font is not already loaded
        globalFontCache[scale->index] = loadFont(fontname , scale->fontSizes[scale->index]);
    }
    return globalFontCache[scale->index];
}

// TODO fix the scroll
void increaseFontSize(BufferManager *bm, char *fontname, WindowManager *wm, int sh, int arg) {
    Buffer *buffer = getActiveBuffer(bm);
    Scale *scale = &buffer->scale;
    int nextIndex = scale->index + arg;
    if (arg == 0) {
        nextIndex = SCALE_ZERO_INDEX; // Reset to default scale if arg is 0
    }

    if (nextIndex >= SCALE_ZERO_INDEX + MIN_FONT_SCALE && nextIndex <= SCALE_ZERO_INDEX + MAX_FONT_SCALE) {
        Font *oldFont = buffer->font;  // Keep the old font to calculate the height difference
        buffer->font = updateFont(scale, nextIndex, fontname);

        // Adjust y position and height of all windows showing the same buffer
        Window *win = wm->head;
        while (win) {
            if (win->buffer == buffer) {
                int oldY = win->y;  // Store old y position
                win->y = sh - buffer->font->ascent + buffer->font->descent;  // Update the y position for the new font size
                int deltaY = oldY - win->y;  // Calculate how much y has moved
                win->height -= deltaY;  // Adjust height to keep the bottom boundary fixed
            }
            win = win->next;
        }
    } else {
        message(bm, "Cannot increase the font size any further");
    }
}

void decreaseFontSize(BufferManager *bm, char *fontname, WindowManager *wm, int sh, int arg) {
    Buffer *buffer = getActiveBuffer(bm);
    Scale *scale = &buffer->scale;
    int nextIndex = scale->index - arg;
    if (arg == 0) {
        nextIndex = SCALE_ZERO_INDEX; // Reset to default scale if arg is 0
    }

    if (nextIndex >= SCALE_ZERO_INDEX + MIN_FONT_SCALE && nextIndex <= SCALE_ZERO_INDEX + MAX_FONT_SCALE) {
        Font *oldFont = buffer->font;  // Keep the old font to calculate the height difference
        buffer->font = updateFont(scale, nextIndex, fontname);
        
        // Adjust y position and height of all windows showing the same buffer
        Window *win = wm->head;
        while (win) {
            if (win->buffer == buffer) {
                int oldY = win->y;  // Store old y position
                win->y = sh - buffer->font->ascent + buffer->font->descent;  // Update the y position for the new font size
                int deltaY = win->y - oldY;  // Calculate how much y has moved
                win->height += deltaY;  // Adjust height to compensate the y movement to keep the bottom boundary fixed

                // This line ensures the modeline does not move by correcting any discrepancies
                if (deltaY < 0) {
                    win->height -= deltaY;  // Decrease height further if deltaY was negative (y moved down)
                }
            }
            win = win->next;
        }
    } else {
        message(bm, "Cannot decrease the font size any further");
    }
}









// TODO use the win->height instead of the sh
/* void increaseFontSize(Buffer *buffer, char *fontname, WindowManager *wm, int sh, int arg) { */
/*     Scale *scale = &buffer->scale; */
/*     int nextIndex = scale->index + 1; */
/*     if (nextIndex <= SCALE_ZERO_INDEX + MAX_FONT_SCALE) { */
/*         Font *oldFont = buffer->font;  // Keep the old font to calculate the height difference */
/*         buffer->font = updateFont(scale, nextIndex, fontname); */
        
/*         // Adjust y position and height of all windows showing the same buffer */
/*         Window *win = wm->head; */
/*         while (win) { */
/*             if (win->buffer == buffer) { */
/*                 int oldY = win->y;  // Store old y position */
/*                 win->y = sh - buffer->font->ascent + buffer->font->descent;  // Update the y position for the new font size */
/*                 int deltaY = oldY - win->y;  // Calculate how much y has moved */
/*                 win->height -= deltaY;  // Adjust height to keep the bottom boundary fixed */
/*             } */
/*             win = win->next; */
/*         } */
/*     } */
/* } */

/* // TODO use the win->height instead of the sh */
/* void decreaseFontSize(Buffer *buffer, char *fontname, WindowManager *wm, int sh) { */
/*     Scale *scale = &buffer->scale; */
/*     int nextIndex = scale->index - 1; */
/*     if (nextIndex >= SCALE_ZERO_INDEX + MIN_FONT_SCALE) { */
/*         Font *oldFont = buffer->font;  // Keep the old font to calculate the height difference */
/*         buffer->font = updateFont(scale, nextIndex, fontname); */
        
/*         // Adjust y position and height of all windows showing the same buffer */
/*         Window *win = wm->head; */
/*         while (win) { */
/*             if (win->buffer == buffer) { */
/*                 int oldY = win->y;  // Store old y position */
/*                 win->y = sh - buffer->font->ascent + buffer->font->descent;  // Update the y position for the new font size */
/*                 int deltaY = win->y - oldY;  // Calculate how much y has moved */
/*                 win->height += deltaY;  // Adjust height to compensate the y movement to keep the bottom boundary fixed */

/*                 // This line ensures the modeline does not move by correcting any discrepancies */
/*                 if (deltaY < 0) { */
/*                     win->height -= deltaY;  // Decrease height further if deltaY was negative (y moved down) */
/*                 } */
/*             } */
/*             win = win->next; */
/*         } */
/*     } */
/* } */

