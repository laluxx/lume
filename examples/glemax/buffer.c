#include "buffer.h"
#include "faces.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "syntax.h"

void initBuffer(Buffer *buffer, const char *name, const char *path) {
    if (!parser) {  // Ensure the global parser is initialized
        fprintf(stderr, "Parser not initialized.\n");
        exit(EXIT_FAILURE);
    }

    buffer->capacity = 1024;
    buffer->content = malloc(buffer->capacity);
    if (!buffer->content) {
        fprintf(stderr, "Failed to allocate memory for buffer content.\n");
        exit(EXIT_FAILURE);
    }
    buffer->content[0] = '\0'; // Initialize content as empty string
    buffer->size = 0;
    buffer->point = 0;
    buffer->readOnly = false;
    buffer->name = strdup(name);
    buffer->path = strdup(path);
    buffer->region.active = false;
    buffer->scale.index = 0;

    // Initialize syntax tree
    buffer->tree = ts_parser_parse_string(parser, NULL, buffer->content, buffer->size);
    initSyntaxArray(&buffer->syntaxArray, 10);
}


/* void initBuffer(Buffer *buffer, const char *name, const char *path) { */
/*     buffer->capacity = 1024;  // Initial capacity */
/*     buffer->content = malloc(buffer->capacity); */
/*     buffer->size = 0; */
/*     buffer->point = 0; */
/*     buffer->readOnly = false; */
/*     buffer->name = strdup(name);  // Duplicate the name for ownership */
/*     buffer->path = strdup(path);  // Duplicate the path for ownership */
/*     buffer->region.active = false; */
/*     buffer->scale.index = 0; */

/*     buffer->tree = ts_parser_parse_string(parser, NULL, buffer->content, buffer->size); */
/*     initSyntaxArray(&buffer->syntaxArray, 10);  // Initial size can be arbitrary; adjust based on expected usage */
    
/*     if (buffer->content == NULL || buffer->name == NULL || buffer->path == NULL || buffer->syntaxArray.items == NULL) { */
/*         fprintf(stderr, "Failed to allocate memory for buffer components.\n"); */
/*         exit(EXIT_FAILURE); */
/*     } else { */
/*         buffer->content[0] = '\0';  // Initialize content as empty string */
/*     } */
/* } */


/* void initBuffer(Buffer *buffer, const char *name, const char *path) { */
/*     buffer->capacity = 1024;  // Initial capacity */
/*     buffer->content = malloc(buffer->capacity); */
/*     buffer->size = 0; */
/*     buffer->point = 0; */
/*     buffer->readOnly = false; */
/*     buffer->name = strdup(name);  // Duplicate the name for ownership */
/*     buffer->path = strdup(path);  // Duplicate the path for ownership */
/*     buffer->region.active = false; */
/*     buffer->scale.index = 0; */
    
/*     if (buffer->content == NULL || buffer->name == NULL || buffer->path == NULL) { */
/*         fprintf(stderr, "Failed to allocate memory for buffer.\n"); */
/*         exit(EXIT_FAILURE); */
/*     } else { */
/*         buffer->content[0] = '\0';  // Initialize content as empty string */
/*     } */
/* } */

void newBuffer(BufferManager *manager, WindowManager *wm,
               const char *name, const char *path, char *fontname,
               int sw, int sh) {
    Buffer *buffer = malloc(sizeof(Buffer));
    if (buffer == NULL) {
        fprintf(stderr, "Failed to allocate memory for new buffer.\n");
        return;
    }
    
    initBuffer(buffer, name, path);
    initScale(&buffer->scale); // Ensure this sets the default index properly
    buffer->font = loadFont(fontname, buffer->scale.fontSizes[buffer->scale.index]); // Make sure this loads the font metrics

    if (manager->count >= manager->capacity) {
        manager->capacity *= 2;
        Buffer **newBuffers = realloc(manager->buffers, sizeof(Buffer*) * manager->capacity);
        if (newBuffers == NULL) {
            fprintf(stderr, "Failed to expand buffer manager capacity.\n");
            free(buffer); // Free allocated buffer on failure
            return;
        }
        manager->buffers = newBuffers;
    }
    
    manager->buffers[manager->count++] = buffer;
    
    // Set the buffer in the active window, ensuring it is immediately visible and correctly positioned
    if (wm->activeWindow) {
        wm->activeWindow->buffer = buffer;
        wm->activeWindow->y = sh - buffer->font->ascent + buffer->font->descent;
        wm->activeWindow->height = wm->activeWindow->y; // Adjust height to maintain text position
    }
    
    // Optionally set the global active buffer if needed
    manager->activeIndex = manager->count - 1;
    free(manager->activeName);
    manager->activeName = strdup(name);
}

void freeBuffer(Buffer *buffer) {
    free(buffer->content);
    free(buffer->name);
    buffer->content = NULL;
    buffer->name = NULL;
    buffer->size = 0;
    buffer->capacity = 0;
    buffer->point = 0;
}

void initBufferManager(BufferManager *manager) {
    manager->capacity = 10;
    manager->buffers = malloc(sizeof(Buffer*) * manager->capacity);
    manager->count = 0;
    manager->activeIndex = -1;
    manager->activeName = NULL;
}

void freeBufferManager(BufferManager *manager) {
    for (int i = 0; i < manager->count; i++) {
        freeBuffer(manager->buffers[i]);
        free(manager->buffers[i]);
    }
    free(manager->buffers);
    free(manager->activeName);
    manager->buffers = NULL;
    manager->activeName = NULL;
    manager->count = 0;
    manager->capacity = 0;
    manager->activeIndex = -1;
}

void switchToBuffer(BufferManager *bm, const char *bufferName) {
    for (int i = 0; i < bm->count; i++) {
        if (strcmp(bm->buffers[i]->name, bufferName) == 0) {
            if (strcmp(getActiveBuffer(bm)->name, "minibuffer") != 0) {
                bm->lastBuffer = getActiveBuffer(bm);
            }
            bm->activeIndex = i;
            return;
        }
    }
}

Buffer *getActiveBuffer(BufferManager *manager) {
    if (manager->activeIndex >= 0) {
        return manager->buffers[manager->activeIndex];
    }
    return NULL;
}

Buffer *getBuffer(BufferManager *manager, const char *name) {
    for (int i = 0; i < manager->count; i++) {
        if (strcmp(manager->buffers[i]->name, name) == 0) {
            return manager->buffers[i];
        }
    }
    return NULL; // Return NULL if no buffer is found
}

bool isCurrentBuffer(BufferManager *manager, const char *bufferName) {
    Buffer *currentBuffer = getActiveBuffer(manager);
    if (currentBuffer != NULL && strcmp(currentBuffer->name, bufferName) == 0) {
        return true;
    }
    return false;
}

void nextBuffer(BufferManager *manager) {
    if (manager->count > 0) {
        manager->activeIndex = (manager->activeIndex + 1) % manager->count;
        free(manager->activeName);
        manager->activeName = strdup(manager->buffers[manager->activeIndex]->name);
        printf("Switched to next buffer: %s\n", manager->activeName);
    }
}

void previousBuffer(BufferManager *manager) {
    if (manager->count > 0) {
        manager->activeIndex = (manager->activeIndex - 1 + manager->count) % manager->count;
        free(manager->activeName);
        manager->activeName = strdup(manager->buffers[manager->activeIndex]->name);
        printf("Switched to previous buffer: %s\n", manager->activeName);
    }
}

void activateRegion(Buffer *buffer) {
    if (!buffer->region.active) {
        buffer->region.start = buffer->region.end = buffer->point;
        buffer->region.active = true;
    }
}

void updateRegion(Buffer *buffer, size_t new_point) {
    if (buffer->region.active) {
        buffer->region.end = new_point;
    }
}

void deactivateRegion(Buffer *buffer) {
    buffer->region.active = false;
}

void setBufferContent(Buffer *buffer, const char *newContent) {
    size_t newContentSize = strlen(newContent) + 1; // +1 for the null terminator

    // Check if buffer's current capacity is insufficient
    if (buffer->capacity < newContentSize) {
        char *newBufferContent = realloc(buffer->content, newContentSize);
        if (!newBufferContent) {
            fprintf(stderr, "Failed to allocate memory for buffer content.\n");
            exit(EXIT_FAILURE);
        }
        buffer->content = newBufferContent;
        buffer->capacity = newContentSize;
    }

    // Copy new content to buffer
    strcpy(buffer->content, newContent);
    buffer->size = newContentSize - 1; // Not counting the null terminator
    buffer->point = buffer->size; // Optionally reset the cursor position
}

void message(BufferManager *bm, const char *message) {
    Buffer *minibuffer = getBuffer(bm, "minibuffer");
    Buffer *messageBuffer = getBuffer(bm, "message");

    // Prepare the message string with square brackets
    size_t messageLen = strlen(message);
    size_t totalLen = messageLen + 3; // For '[' + ']' + '\0'
    char *formattedMessage = malloc(totalLen);

    if (formattedMessage) {
        snprintf(formattedMessage, totalLen, "[%s]", message);

        if (isCurrentBuffer(bm, "minibuffer")) {
              setBufferContent(messageBuffer, formattedMessage);
        } else {
            setBufferContent(minibuffer, message);
        }

        free(formattedMessage);
    } else {
        // Handle memory allocation failure if needed
        fprintf(stderr, "Failed to allocate memory for formatted message.\n");
    }
}


void cleanBuffer(BufferManager *bm, char *name) {
    Buffer *buffer = getBuffer(bm, name);
    buffer->size = 0;
    buffer->point = 0;
    buffer->content[0] = '\0';
}
