#ifndef THEME_H
#define THEME_H

#include "common.h"

typedef struct {
    char* name;
    Color bg;
    Color cursor;
    Color text;
    Color minibuffer;
    Color modeline;
    Color modeline_highlight;
} Theme;

extern Theme themes[];
extern int currentThemeIndex;

#define CT (themes[currentThemeIndex])

Color hexToColor(const char* hexStr);
void initThemes();
void nextTheme();
void previousTheme();

#endif  // THEME_H

