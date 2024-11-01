#include "completion.h"
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// TODO it segfault when i complete a [NO MATCH]

void fetch_completions(const char* input, CompletionEngine *ce) {
    DIR* dir;
    struct dirent* entry;
    char fullPath[PATH_MAX];
    char dirPath[PATH_MAX];
    const char* homeDir = getenv("HOME");

    // Handle home directory expansion
    if (input[0] == '~') {
        snprintf(fullPath, PATH_MAX, "%s%s", homeDir, input + 1);
    } else {
        strncpy(fullPath, input, PATH_MAX);
    }
    fullPath[PATH_MAX - 1] = '\0';

    // Extract directory path
    char* lastSlash = strrchr(fullPath, '/');
    if (lastSlash) {
        strncpy(dirPath, fullPath, lastSlash - fullPath);
        dirPath[lastSlash - fullPath] = '\0';
    } else {
        // Handle the case where no directory is specified (use current directory)
        strcpy(dirPath, ".");
    }

    dir = opendir(dirPath);
    if (!dir) {
        perror("Failed to open directory");
        return;
    }

    // Clear previous completions
    for (int i = 0; i < ce->count; i++) {
        free(ce->items[i]);
    }
    free(ce->items);
    ce->items = NULL;
    ce->count = 0;

    // Collect new completions
    while ((entry = readdir(dir)) != NULL) {
        const char* lastPart = lastSlash ? lastSlash + 1 : input;
        if (strncmp(entry->d_name, lastPart, strlen(lastPart)) == 0) {
            char formattedPath[PATH_MAX];

            if (entry->d_type == DT_DIR) {
                snprintf(formattedPath, PATH_MAX, "%s/%s/", dirPath, entry->d_name);
            } else {
                snprintf(formattedPath, PATH_MAX, "%s/%s", dirPath, entry->d_name);
            }

            if (strncmp(formattedPath, homeDir, strlen(homeDir)) == 0) {
                snprintf(formattedPath, PATH_MAX, "~%s", formattedPath + strlen(homeDir));
            }

            ce->items = realloc(ce->items, sizeof(char*) * (ce->count + 1));
            ce->items[ce->count++] = strdup(formattedPath);
        }
    }

    closedir(dir);
    ce->isActive = ce->count > 0; // Activate only if there are completions
    ce->currentIndex = -1; // Reset index for new session
}

/* void fetch_completions(const char* input, CompletionEngine *ce) { */
/*     DIR* dir; */
/*     struct dirent* entry; */
/*     char fullPath[PATH_MAX]; */
/*     char dirPath[PATH_MAX]; */
/*     const char* homeDir = getenv("HOME"); */

/*     // Resolve path, considering '~' and extracting directory path */
/*     if (input[0] == '~') { */
/*         snprintf(fullPath, PATH_MAX, "%s%s", homeDir, input + 1); */
/*     } else { */
/*         strncpy(fullPath, input, PATH_MAX); */
/*     } */
/*     fullPath[PATH_MAX - 1] = '\0'; */

/*     char* lastSlash = strrchr(fullPath, '/'); */
/*     if (lastSlash) { */
/*         memcpy(dirPath, fullPath, lastSlash - fullPath); */
/*         dirPath[lastSlash - fullPath] = '\0'; */
/*     } else { */
/*         strcpy(dirPath, fullPath);  // Full path is the directory if no slash */
/*     } */

/*     dir = opendir(dirPath); */
/*     if (!dir) { */
/*         perror("Failed to open directory"); */
/*         return; */
/*     } */

/*     // Free previous completions */
/*     for (int i = 0; i < ce->count; i++) { */
/*         free(ce->items[i]); */
/*     } */
/*     free(ce->items); */
/*     ce->items = NULL; */
/*     ce->count = 0; */

/*     // Collect new completions */
/*     while ((entry = readdir(dir)) != NULL) { */
/*         const char* lastPart = lastSlash ? lastSlash + 1 : input; */
/*         if (strncmp(entry->d_name, lastPart, strlen(lastPart)) == 0) { */
/*             char formattedPath[PATH_MAX]; */

/*             // Format the path with a tilde for user-friendly display */
/*             if (entry->d_type == DT_DIR) { */
/*                 snprintf(formattedPath, PATH_MAX, "%s/%s/", dirPath, entry->d_name); */
/*             } else { */
/*                 snprintf(formattedPath, PATH_MAX, "%s/%s", dirPath, entry->d_name); */
/*             } */

/*             if (strncmp(formattedPath, homeDir, strlen(homeDir)) == 0) { */
/*                 snprintf(formattedPath, PATH_MAX, "~%s", formattedPath + strlen(homeDir)); */
/*             } */

/*             ce->items = realloc(ce->items, sizeof(char*) * (ce->count + 1)); */
/*             ce->items[ce->count++] = strdup(formattedPath); */
/*         } */
/*     } */

/*     closedir(dir); */
/*     ce->isActive = true; */
/*     ce->currentIndex = -1; // Reset index for new session */
/* } */
