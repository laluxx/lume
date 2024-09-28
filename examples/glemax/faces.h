#ifndef FACES_H
#define FACES_H

#include "font.h"
#include "buffer.h"

extern Font *font;
extern Font *minifont;

extern int fontincrement;
extern int fontsize;
extern int minifontsize;
extern char *fontname;


#define MAX_FONT_SCALE 27
#define MIN_FONT_SCALE -8
#define MAX_FONT_SCALE_INDEX 36
#define SCALE_ZERO_INDEX 8

extern Font *globalFontCache[MAX_FONT_SCALE_INDEX];  // Global cache of rasterized fonts

void initScale(Scale *scale);
Font* updateFont(Scale *scale, int newIndex, char *fontname);
void increaseFontSize(Buffer *buffer, char *fontname, WindowManager *wm, int sh);
void decreaseFontSize(Buffer *buffer, char *fontname, WindowManager *wm, int sh);
/* void increaseFontSize(Buffer *buffer, char *fontname, WindowManager *wm); */
/* void decreaseFontSize(Buffer *buffer, char *fontname, WindowManager *wm); */

#endif
