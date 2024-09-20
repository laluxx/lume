#include "wm.h"
#include <stdlib.h>
#include <string.h>

void initWindowManager(WindowManager *wm, BufferManager *bm, Font *font, int sw, int sh) {
    wm->head = malloc(sizeof(Window));
    wm->head->x = 0;
    wm->head->y = sh - font->ascent + font->descent;  // Adjust for font height
    wm->head->width = sw;
    wm->head->height = wm->head->y;  // Adjust height accordingly
    wm->head->buffer = getActiveBuffer(bm);
    wm->head->prev = NULL;
    wm->head->next = NULL;
    wm->head->isActive = true;
    wm->activeWindow = wm->head;
    wm->activeWindow->splitOrientation = HORIZONTAL;
    wm->count = 1;
}



void split_window_right(WindowManager *wm, Font *font, int sw, int sh) {
    Window *active = wm->activeWindow;
    Window *newWindow = malloc(sizeof(Window));
    if (!newWindow) return;

    *newWindow = *active; // Copy the settings from active window
    newWindow->width /= 2;
    active->width -= newWindow->width;
    newWindow->x += active->width;
    active->splitOrientation = VERTICAL; // Set split orientation
    newWindow->splitOrientation = VERTICAL; // Set split orientation

    // Insert new window into the list
    newWindow->next = active->next;
    if (active->next) {
        active->next->prev = newWindow;
    }
    active->next = newWindow;
    newWindow->prev = active;

    wm->count++;
}


void split_window_below(WindowManager *wm, Font *font, int sw, int sh) {
    Window *active = wm->activeWindow;
    Window *newWindow = malloc(sizeof(Window));
    if (!newWindow) return;

    *newWindow = *active; // Copy settings from active window
    newWindow->height = active->height / 2;
    active->height = newWindow->height; // Make active window also half its original height

    // New window should be positioned directly below the active window
    newWindow->y = active->y - newWindow->height; // Subtract height because y increases upward

    active->splitOrientation = HORIZONTAL; // Set split orientation
    newWindow->splitOrientation = HORIZONTAL; // Set split orientation

    // Insert the new window into the list
    newWindow->next = active->next;
    if (active->next) {
        active->next->prev = newWindow;
    }
    active->next = newWindow;
    newWindow->prev = active;

    wm->count++;
}

void delete_window(WindowManager *wm) {
    if (wm->count <= 1) {
        return; // Cannot delete the last remaining window
    }

    Window *active = wm->activeWindow;

    // Reassign active window
    if (active->prev) {
        wm->activeWindow = active->prev;
    } else if (active->next) {
        wm->activeWindow = active->next;
    }

    // Remove the window from the list
    if (active->prev) {
        active->prev->next = active->next;
    } else {
        wm->head = active->next; // Update head if the first window is being deleted
    }
    if (active->next) {
        active->next->prev = active->prev;
    }

    // Adjust size of adjacent windows based on the orientation
    if (active->splitOrientation == HORIZONTAL) {
        // Combine heights if the split was horizontal
        if (active->prev) {
            active->prev->height += active->height;
        } else if (active->next) {
            active->next->y = active->y;
            active->next->height += active->height;
        }
    } else if (active->splitOrientation == VERTICAL) {
        // Combine widths if the split was vertical
        if (active->prev) {
            active->prev->width += active->width;
        } else if (active->next) {
            active->next->x = active->x;
            active->next->width += active->width;
        }
    }

    free(active);
    wm->count--;
}



void other_window(WindowManager *wm, int direction) {
    if (direction == 1) {
        if (wm->activeWindow->next) {
            wm->activeWindow->isActive = false;
            wm->activeWindow = wm->activeWindow->next;
            wm->activeWindow->isActive = true;
        } else if (wm->head) {
            wm->activeWindow->isActive = false;
            wm->activeWindow = wm->head;
            wm->activeWindow->isActive = true;
        }
    } else if (direction == -1) {
        Window *current = wm->head;
        if (current == wm->activeWindow) {
            while (current->next) {
                current = current->next;
            }
            wm->activeWindow->isActive = false;
            wm->activeWindow = current;
            wm->activeWindow->isActive = true;
        } else {
            while (current->next != wm->activeWindow) {
                current = current->next;
            }
            wm->activeWindow->isActive = false;
            wm->activeWindow = current;
            wm->activeWindow->isActive = true;
        }
    }
}

void freeWindowManager(WindowManager *wm) {
    Window *current = wm->head;
    while (current != NULL) {
        Window *next = current->next;
        free(current);
        current = next;
    }
    wm->head = NULL;
    wm->activeWindow = NULL;
    wm->count = 0;
}



void updateWindowDimensions(Window *win, int x, int y, int width, int height) {
    if (win == NULL) return;

    // Set the current window dimensions
    win->x = x;
    win->y = y;
    win->width = width;
    win->height = height;

    // If the window has a next window in the same split orientation, recurse into it
    if (win->next && win->splitOrientation == win->next->splitOrientation) {
        if (win->splitOrientation == HORIZONTAL) {
            // Recurse with adjusted y and height
            updateWindowDimensions(win->next, x, y - height / 2, width, height / 2);
        } else if (win->splitOrientation == VERTICAL) {
            // Recurse with adjusted x and half the width
            updateWindowDimensions(win->next, x + width / 2, y, width / 2, height);
        }
    } else if (win->next) {
        // If the orientation changes, reset dimensions based on the orientation
        if (win->splitOrientation == HORIZONTAL) {
            updateWindowDimensions(win->next, x, y - height / 2, width, height / 2);
        } else {
            updateWindowDimensions(win->next, x + width / 2, y, width / 2, height);
        }
    }
}

void updateWindows(WindowManager *wm, Font *font, int newWidth, int newHeight) {
    // Start the recursive adjustment from the head window
    if (wm->head != NULL) {
        updateWindowDimensions(wm->head, 0, newHeight - (font->ascent - font->descent), newWidth, newHeight);
    }
}












/* void updateWindows(WindowManager *wm, Font *font, int newWidth, int newHeight) { */
/*     Window *win = wm->head; */
/*     int numberOfWindows = 0; */
    
/*     // First, count the number of windows */
/*     while (win) { */
/*         numberOfWindows++; */
/*         win = win->next; */
/*     } */

/*     // Calculate new width for each window if they are arranged horizontally */
/*     int newWindowWidth = newWidth / numberOfWindows; */
    
/*     // Reset to head to update dimensions and positions */
/*     win = wm->head; */
/*     int xOffset = 0;  // Starting x position for the first window */

/*     while (win) { */
/*         win->width = newWindowWidth; */
/*         win->height = newHeight;  // Each window takes full screen height */
/*         win->x = xOffset; */
/*         win->y = newHeight - font->ascent + font->descent;  // Adjust for font height to start text from the top */

/*         xOffset += newWindowWidth;  // Move the x offset for the next window */
/*         win = win->next; */
/*     } */
/* } */


void printActiveWindowDetails(WindowManager *wm) {
    Window *win = wm->activeWindow;
    if (!win) {
        printf("No active window.\n");
        return;
    }

    printf("\nActive Window Details:\n");
    printf("  X Position: %.2f\n", win->x);
    printf("  Y Position: %.2f\n", win->y);
    printf("  Width: %.2f\n", win->width);
    printf("  Height: %.2f\n", win->height);
    printf("  Buffer Name: %s\n", win->buffer->name);
    printf("  Is Active: %s\n", win->isActive ? "Yes" : "No");
    printf("  Split Orientation: %s\n", 
           (win->splitOrientation == HORIZONTAL) ? "Horizontal" : "Vertical");
    printf("  Window Count in Manager: %d\n", wm->count);
    
    // Additional details about linked windows
    printf("  Previous Window: %p\n", (void *)win->prev);
    printf("  Next Window: %p\n", (void *)win->next);
}

