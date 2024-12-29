#include "keychords.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

// TODO This module does not work
// and is not used at all for now

static KeyChordManager manager = {0};

void initKeyChordManager(void) {
    manager.sequence_timeout = 1.0;  // 1 second timeout
    manager.capacity = 32;
    manager.keychords = malloc(sizeof(KeyChord) * manager.capacity);
    manager.num_sequences = 0;
    manager.current_sequence = malloc(sizeof(KeyPress) * 8);
    manager.sequence_length = 0;
}

void freeKeyChordManager(void) {
    for (int i = 0; i < manager.num_sequences; i++) {
        free(manager.keychords[i].sequence);
    }
    free(manager.keychords);
    free(manager.current_sequence);
}

static int parseKey(const char* key_str) {
    struct KeyMap {
        const char* name;
        int keycode;
    };

    static const struct KeyMap special_keys[] = {
        {"RET", GLFW_KEY_ENTER},
        {"SPC", GLFW_KEY_SPACE},
        {"TAB", GLFW_KEY_TAB},
        {"ESC", GLFW_KEY_ESCAPE},
        {"DEL", GLFW_KEY_DELETE},
        {"LEFT", GLFW_KEY_LEFT},
        {"RIGHT", GLFW_KEY_RIGHT},
        {"UP", GLFW_KEY_UP},
        {"DOWN", GLFW_KEY_DOWN},
        {"BACKSPACE", GLFW_KEY_BACKSPACE},
        {"HOME", GLFW_KEY_HOME},
        {"END", GLFW_KEY_END},
        {"PGUP", GLFW_KEY_PAGE_UP},
        {"PGDN", GLFW_KEY_PAGE_DOWN},
    };

    // Check special keys
    for (size_t i = 0; i < sizeof(special_keys) / sizeof(special_keys[0]); i++) {
        if (strcasecmp(key_str, special_keys[i].name) == 0) {
            return special_keys[i].keycode;
        }
    }

    // Function keys
    if (toupper(key_str[0]) == 'F' && isdigit(key_str[1])) {
        int num = atoi(key_str + 1);
        if (num >= 1 && num <= 25) {
            return GLFW_KEY_F1 + (num - 1);
        }
    }

    // Single character
    if (strlen(key_str) == 1) {
        return toupper((unsigned char)key_str[0]);
    }

    return -1;
}

KeyChord* createKeySequenceFromString(const char* sequence_str) {
    KeyChord* chord = malloc(sizeof(KeyChord));
    chord->capacity = 4;
    chord->length = 0;
    chord->sequence = malloc(sizeof(KeyPress) * chord->capacity);
    chord->callback = NULL;

    char* str = strdup(sequence_str);
    char* token = strtok(str, " ");

    while (token != NULL) {
        if (chord->length >= chord->capacity) {
            chord->capacity *= 2;
            chord->sequence = realloc(chord->sequence, 
                                      sizeof(KeyPress) * chord->capacity);
        }

        KeyPress keypress = {0};
        char* ptr = token;

        // Parse modifiers (Emacs style)
        while (*ptr) {
            if (strncmp(ptr, "C-", 2) == 0) {
                keypress.ctrl = true;
                ptr += 2;
            } else if (strncmp(ptr, "M-", 2) == 0) {
                keypress.meta = true;
                ptr += 2;
            } else if (strncmp(ptr, "S-", 2) == 0) {
                keypress.shift = true;
                ptr += 2;
            } else if (strncmp(ptr, "s-", 2) == 0) {
                keypress.super = true;
                ptr += 2;
            } else {
                break;
            }
        }

        keypress.key = parseKey(ptr);
        if (keypress.key != -1) {
            chord->sequence[chord->length++] = keypress;
        }

        token = strtok(NULL, " ");
    }

    free(str);
    return chord;
}

bool registerKey(const char* sequence_str, void (*callback)(void)) {
    if (manager.num_sequences >= manager.capacity) {
        int new_capacity = manager.capacity * 2;
        KeyChord* new_keychords = realloc(manager.keychords, 
                                          sizeof(KeyChord) * new_capacity);
        if (!new_keychords) return false;
        
        manager.keychords = new_keychords;
        manager.capacity = new_capacity;
    }

    KeyChord* chord = createKeySequenceFromString(sequence_str);
    if (!chord) return false;

    chord->callback = callback;
    manager.keychords[manager.num_sequences++] = *chord;
    free(chord);  // Note: sequence array is kept as it's now part of keychords
    return true;
}

void unregisterKey(const char* sequence_str) {
    KeyChord* chord = createKeySequenceFromString(sequence_str);
    if (!chord) return;

    for (int i = 0; i < manager.num_sequences; i++) {
        if (matchKeySequence(chord, &manager.keychords[i])) {
            // Free the sequence array of the removed chord
            free(manager.keychords[i].sequence);
            
            // Move remaining chords down
            if (i < manager.num_sequences - 1) {
                memmove(&manager.keychords[i], &manager.keychords[i + 1],
                        sizeof(KeyChord) * (manager.num_sequences - i - 1));
            }
            
            manager.num_sequences--;
            break;
        }
    }

    free(chord->sequence);
    free(chord);
}

void processKeyInput(int key, int action, int mods) {
    if (action != GLFW_PRESS) return;

    double current_time = glfwGetTime();

    // Check for timeout
    if (current_time - manager.last_key_time > manager.sequence_timeout) {
        manager.sequence_length = 0;
    }

    KeyPress keypress = {
        .key = key,
        .ctrl = (mods & GLFW_MOD_CONTROL) != 0,
        .meta = (mods & GLFW_MOD_ALT) != 0,
        .shift = (mods & GLFW_MOD_SHIFT) != 0,
        .super = (mods & GLFW_MOD_SUPER) != 0
    };

    manager.current_sequence[manager.sequence_length++] = keypress;
    manager.last_key_time = current_time;

    KeyChord current = {
        .sequence = manager.current_sequence,
        .length = manager.sequence_length
    };

    // Check for matches
    for (int i = 0; i < manager.num_sequences; i++) {
        if (matchKeySequence(&current, &manager.keychords[i])) {
            if (manager.keychords[i].callback) {
                manager.keychords[i].callback();
            }
            manager.sequence_length = 0;
            break;
        }
    }
}

bool matchKeySequence(const KeyChord* seq1, const KeyChord* seq2) {
    if (seq1->length != seq2->length) return false;

    for (int i = 0; i < seq1->length; i++) {
        const KeyPress* k1 = &seq1->sequence[i];
        const KeyPress* k2 = &seq2->sequence[i];

        if (k1->key != k2->key || 
            k1->ctrl != k2->ctrl ||
            k1->meta != k2->meta ||
            k1->shift != k2->shift ||
            k1->super != k2->super) {
            return false;
        }
    }

    return true;
}

void freeKeySequence(KeyChord* sequence) {
    if (sequence) {
        free(sequence->sequence);
        free(sequence);
    }
}

void printKeySequence(const KeyChord* sequence) {
    printf("Key Sequence: ");
    for (int i = 0; i < sequence->length; i++) {
        if (i > 0) printf(" ");
        if (sequence->sequence[i].ctrl) printf("C-");
        if (sequence->sequence[i].meta) printf("M-");
        if (sequence->sequence[i].shift) printf("S-");
        if (sequence->sequence[i].super) printf("s-");
        
        // Special key handling
        if (sequence->sequence[i].key == GLFW_KEY_ENTER) printf("RET");
        else if (sequence->sequence[i].key == GLFW_KEY_SPACE) printf("SPC");
        else if (sequence->sequence[i].key == GLFW_KEY_TAB) printf("TAB");
        else printf("%c", sequence->sequence[i].key);
    }
    printf("\n");
}

void listRegisteredSequences(void) {
    printf("Registered Key Sequences:\n");
    for (int i = 0; i < manager.num_sequences; i++) {
        printf("%d. ", i + 1);
        printKeySequence(&manager.keychords[i]);
    }
}
