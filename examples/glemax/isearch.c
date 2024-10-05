#include "isearch.h"

void isearch_forward(Buffer *buffer, BufferManager *bm, Buffer *minibuffer, bool updateStartIndex, ISearch *is) {
    const char *start = buffer->content + is->startIndex;
    const char *found = strstr(start, minibuffer->content);

    if (found) {
        size_t matchIndex = found - buffer->content;
        buffer->point = matchIndex + strlen(minibuffer->content);
        is->lastMatchIndex = matchIndex + 1;
        if (updateStartIndex) {
            is->startIndex = buffer->point;
        }
        is->wrap = false;
    } else {
        if (!is->wrap) {
            /* printf("Reached end of buffer. Press Ctrl+S again to wrap search.\n"); */
            message(bm, "Reached end of buffer");
            is->wrap = true;
        } else {
            if (updateStartIndex) {
                is->startIndex = 0;
                is->wrap = false;
                isearch_forward(buffer, bm, minibuffer, false, is);
            }
        }
    }
}

void isearch_backward(Buffer *buffer, Buffer *minibuffer, bool updateStartIndex, ISearch *is) {
    if (is->lastMatchIndex == 0 || is->lastMatchIndex > buffer->size) {
        // No previous match or out of bounds, start from end of buffer
        is->lastMatchIndex = buffer->size;
    }

    char *searchEnd = buffer->content + is->lastMatchIndex - 1;
    char *found = NULL;

    // Ensure we do not start a search from a negative index
    if (searchEnd < buffer->content) {
        searchEnd = buffer->content;
    }

    for (char *p = searchEnd; p >= buffer->content; --p) {
        if (strncmp(p, minibuffer->content, strlen(minibuffer->content)) == 0) {
            found = p;
            break;
        }
    }

    if (found) {
        size_t matchIndex = found - buffer->content;
        buffer->point = matchIndex;
        if (updateStartIndex) {
            is->lastMatchIndex = matchIndex;  // Set for next potential search update
        }
        is->wrap = false;
    } else {
        if (!is->wrap) {
            printf("Reached beginning of buffer. Press Ctrl+R again to wrap search.\n");
            is->wrap = true;
        } else {
            if (updateStartIndex) {
                is->lastMatchIndex = buffer->size; // Reset lastMatchIndex for wrapping
                is->wrap = false;
                isearch_backward(buffer, minibuffer, false, is); // Attempt to wrap around
            }
        }
    }
}
