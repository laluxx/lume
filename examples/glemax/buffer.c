#include "buffer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void initBuffer(Buffer *buffer, const char *name) {
    buffer->capacity = 1024; // Initial capacity
    buffer->content = malloc(buffer->capacity);
    buffer->size = 0;
    buffer->point = 0;
    buffer->readOnly = false; // Default to writable
    buffer->name = strdup(name); // Duplicate the name for ownership
    buffer->region.active = false;
    if (buffer->content && buffer->name) {
        buffer->content[0] = '\0';
    } else {
        fprintf(stderr, "Failed to allocate memory for buffer.\n");
        exit(EXIT_FAILURE);
    }
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

void newBuffer(BufferManager *manager, const char *name) {
    Buffer *buffer = malloc(sizeof(Buffer));
    initBuffer(buffer, name);
    if (manager->count >= manager->capacity) {
        manager->capacity *= 2;
        manager->buffers = realloc(manager->buffers, sizeof(Buffer*) * manager->capacity);
    }
    manager->buffers[manager->count++] = buffer;
    manager->activeIndex = manager->count - 1; // Set new buffer as active
    free(manager->activeName); // Free old name
    manager->activeName = strdup(name); // Set new active name
}

void switchToBuffer(BufferManager *manager, const char *name) {
    for (int i = 0; i < manager->count; i++) {
        if (strcmp(manager->buffers[i]->name, name) == 0) {
            manager->activeIndex = i;
            free(manager->activeName);
            manager->activeName = strdup(name);
            return;
        }
    }
    printf("Buffer named '%s' not found.\n", name);
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
    printf("ACTIVATED REGION\n");
}

void updateRegion(Buffer *buffer, size_t new_point) {
    if (buffer->region.active) {
        buffer->region.end = new_point;
    }
}

void deactivateRegion(Buffer *buffer) {
    buffer->region.active = false;
    printf("DEACTIVATED REGION\n");
}














