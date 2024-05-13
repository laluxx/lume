#ifndef THEME_H
#define THEME_H

#include "common.h"

typedef struct {
    char* name;
    Color bg;
    Color color;
} Theme;

extern Theme themes[];
extern int currentThemeIndex;

#define CURRENT_THEME (themes[currentThemeIndex])

Color hexToColor(const char* hexStr);
void initializeThemes();
void nextTheme();
void previousTheme();

#endif  // THEME_H

