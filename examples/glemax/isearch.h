#ifndef ISEARCH_H
#define ISEARCH_H

#include "buffer.h"
#include <stdbool.h>

typedef struct {
    Buffer *searchBuffer;
    size_t lastMatchIndex;
    size_t startIndex;
    bool searching;
    char *lastSearch;
    bool wrap;
} ISearch;

void isearch_forward(Buffer *buffer, Buffer *minibuffer, bool updateStartIndex, ISearch *is);
void isearch_backward(Buffer *buffer, Buffer *minibuffer, bool updateStartIndex, ISearch *is);

#endif
