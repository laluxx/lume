#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
#include <stdbool.h>

typedef struct {
    size_t start;   // Start position of the region
    size_t end;     // End position of the region
    bool active;    // Whether the region is currently active
} Region;

typedef struct {
    char *content;   // Text content
    size_t size;     // Current size of content
    size_t capacity; // Allocated capacity
    size_t point;    // Cursor position
    char *name;      // Buffer name
    bool readOnly;   // Read-only flag
    Region region;
} Buffer;

typedef struct {
    Buffer **buffers;    // Array of buffer pointers
    int count;           // Number of buffers
    int capacity;        // Capacity of the buffer list
    int activeIndex;     // Index of the active buffer
    char *activeName;    // Name of the active buffer
} BufferManager;

void initBuffer(Buffer *buffer, const char *name);
void freeBuffer(Buffer *buffer);
void initBufferManager(BufferManager *manager);
void freeBufferManager(BufferManager *manager);
void newBuffer(BufferManager *manager, const char *name);
void switchToBuffer(BufferManager *manager, const char *name);
Buffer *getActiveBuffer(BufferManager *manager);
Buffer *getBuffer(BufferManager *manager, const char *name);
bool isCurrentBuffer(BufferManager *manager, const char *bufferName);
void nextBuffer(BufferManager *manager);
void previousBuffer(BufferManager *manager);

void activateRegion(Buffer *buffer);
void updateRegion(Buffer *buffer, size_t new_point);
void deactivateRegion(Buffer *buffer);



#endif
