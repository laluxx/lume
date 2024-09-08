#include <stdio.h>
#include <string.h>
#include "theme.h"

int currentThemeIndex = 0;
Theme themes[9];

Color hexToColor(const char* hex) {
    int r, g, b;
    sscanf(hex, "#%02x%02x%02x", &r, &g, &b);
    return (Color){r / 255.0f, g / 255.0f, b / 255.0f, 1.0f}; // Alpha is set to 1.0f (fully opaque)
}

void initThemes() {
    themes[0] = (Theme){
        .name = "dark",
        .bg = hexToColor("#18181B"),
        .cursor = hexToColor("#cd9575"),
        .text = hexToColor("#e4e4e8"),
        .minibuffer = hexToColor("#18181B"),
        .modeline = hexToColor("#222225"),
        .modeline_highlight = hexToColor("#222225"),
    };

    themes[1] = (Theme){
        .name = "Catppuccin",
        .bg = hexToColor("#1E1E2E"),
        .cursor = hexToColor("#B4BEFE"),
        .text = hexToColor("#CDD6F4"),
        .minibuffer = hexToColor("#1E1E2E"),
        .modeline = hexToColor("#181825"),
        .modeline_highlight = hexToColor("#B4BEFE"),
    };


    themes[2] = (Theme){
        .name = "Gum",
        .bg = hexToColor("#14171E"),
        .cursor = hexToColor("#D6A0D1"),
        .text = hexToColor("#D4D4D6"),
        .minibuffer = hexToColor("#14171E"),
        .modeline = hexToColor("#191D26"),
        .modeline_highlight = hexToColor("#9587DE"),
    };

    themes[3] = (Theme){
        .name = "Tokyonight",
        .bg = hexToColor("#1A1B26"),
        .cursor = hexToColor("#7AA2F7"),
        .text = hexToColor("#A9B1D6"),
        .minibuffer = hexToColor("#1A1B26"),
        .modeline = hexToColor("#161620"),
        .modeline_highlight = hexToColor("#7AA2F7"),
    };
  
    themes[4] = (Theme){
        .name = "Nature",
        .bg = hexToColor("#070707"),
        .cursor = hexToColor("#658B5F"),
        .text = hexToColor("#C0ACD1"),
        .minibuffer = hexToColor("#090909"),
        .modeline = hexToColor("#050505"),
        .modeline_highlight = hexToColor("#658B5F"),
    };

    themes[5] = (Theme){
        .name = "Doom-one",
        .bg = hexToColor("#282C34"),
        .cursor = hexToColor("#51AFEF"),
        .text = hexToColor("#BBC2CF"),
        .minibuffer = hexToColor("#21242B"),
        .modeline = hexToColor("#1D2026"),
        .modeline_highlight = hexToColor("#51AFEF"),
    };

    themes[6] = (Theme){
        .name = "City-lights",
        .bg = hexToColor("#1D252C"),
        .cursor = hexToColor("#51AFEF"),
        .text = hexToColor("#A0B3C5"),
        .minibuffer = hexToColor("#181E24"),
        .modeline = hexToColor("#181F25"),
        .modeline_highlight = hexToColor("#5EC4FF"),
    };

    themes[7] = (Theme){
        .name = "Molokai",
        .bg = hexToColor("#1C1E1F"),
        .cursor = hexToColor("#FB2874"),
        .text = hexToColor("#D6D6D4"),
        .minibuffer = hexToColor("#222323"),
        .modeline = hexToColor("#2D2E2E"),
        .modeline_highlight = hexToColor("#B6E63E"),
    };

    themes[8] = (Theme){
        .name = "Sunset",
        .bg = hexToColor("#0C0D12"),
        .cursor = hexToColor("#D9A173"),
        .text = hexToColor("#CCCCC5"),
        .minibuffer = hexToColor("#0C0D12"),
        .modeline = hexToColor("#08090C"),
        .modeline_highlight = hexToColor("#D9A173"),
    };
}


void nextTheme() {
    currentThemeIndex++;
    if (currentThemeIndex >= sizeof(themes) / sizeof(Theme)) {
        currentThemeIndex = 0;
    }
    printf("Switched to next theme: %s\n", CT.name);
}

void previousTheme() {
    currentThemeIndex--;
    if (currentThemeIndex < 0) {
        currentThemeIndex = sizeof(themes) / sizeof(Theme) - 1;
    }
    printf("Switched to previous theme: %s\n", CT.name);
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
