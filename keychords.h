#ifndef KEYCHORDS_H
#define KEYCHORDS_H

#include <stdbool.h>
#include <GLFW/glfw3.h>

typedef struct {
    int key;    // Main key code
    bool ctrl;  // Control modifier
    bool meta;  // Meta/Alt modifier (M- in Emacs notation)
    bool shift; // Shift modifier
    bool super; // Super/Command modifier (s- in Emacs notation)
} KeyPress;

typedef struct {
    KeyPress *sequence;     // Array of keypresses in the sequence
    int length;            // Current length of sequence
    int capacity;          // Allocated capacity
    void (*callback)(void); // Function to call when sequence is matched
} KeyChord;

typedef struct {
    KeyChord *keychords;         // Array of keychords
    int num_sequences;           // Number of registered sequences
    int capacity;                // Allocated capacity
    KeyPress *current_sequence;  // Current input sequence buffer
    int sequence_length;         // Length of current sequence
    double last_key_time;        // Time of last keypress
    double sequence_timeout;     // Maximum time between keypresses
} KeyChordManager;

void initKeyChordManager(void);
void freeKeyChordManager(void);
void updateKeyChords(void);

// Sequence registration
bool registerKey(const char* sequence_str, void (*callback)(void));
void unregisterKey(const char* sequence_str);

// Input processing
void processKeyInput(int key, int action, int mods);

// Utility functions
KeyChord* createKeySequenceFromString(const char* sequence_str);
void freeKeySequence(KeyChord* sequence);
bool matchKeySequence(const KeyChord* seq1, const KeyChord* seq2);
const char* keySequenceToString(const KeyChord* sequence);

// Debug functions
void printKeySequence(const KeyChord* sequence);
void listRegisteredSequences(void);

#endif // KEYCHORDS_H
