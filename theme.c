#include <stdio.h>
#include <string.h>
#include "theme.h"

int currentThemeIndex = 0;
Theme themes[2];

Color hexToColor(const char* hex) {
    int r, g, b;
    sscanf(hex, "#%02x%02x%02x", &r, &g, &b);
    return (Color){r / 255.0f, g / 255.0f, b / 255.0f, 1.0f}; // Alpha is set to 1.0f (fully opaque)
}

void initializeThemes() {
    themes[0] = (Theme){
        .name = "doom-material-dark",
        .bg = hexToColor("#212121"),
        .color = hexToColor("#FF0000"),
    };

    themes[1] = (Theme) {
        .name = "doom-one",
        .bg = hexToColor("#282C34"),
        .color = hexToColor("#FF0000"),
    };
}

void nextTheme() {
    currentThemeIndex++;
    if (currentThemeIndex >= sizeof(themes) / sizeof(Theme)) {
        currentThemeIndex = 0;
    }
    printf("Switched to next theme: %s\n", CURRENT_THEME.name);
}

void previousTheme() {
    currentThemeIndex--;
    if (currentThemeIndex < 0) {
        currentThemeIndex = sizeof(themes) / sizeof(Theme) - 1;
    }
    printf("Switched to previous theme: %s\n", CURRENT_THEME.name);
}


void loadTheme(const char* themeName) {
    for (int i = 0; i < sizeof(themes) / sizeof(Theme); i++) {
        if (strcmp(themes[i].name, themeName) == 0) {
            currentThemeIndex = i;
            return;
        }
    }
    // Handle the case where the theme is not found
    printf("Theme '%s' not found.\n", themeName);
}
