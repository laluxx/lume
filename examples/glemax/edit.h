#ifndef EDIT_H
#define EDIT_H

#include "buffer.h"

#define KILL_RING_SIZE 60

typedef struct {
    char** entries;    // Array of strings
    int size;          // Number of entries currently in the kill ring
    int capacity;      // Maximum number of entries
    int index;         // Current index for yanking
} KillRing;


void initKillRing(KillRing* kr, int capacity);
void freeKillRing(KillRing* kr);
void kill(KillRing* kr, const char* text);
void kill_region(Buffer *buffer, KillRing *kr);
void yank(Buffer *buffer, KillRing *kr);
void kill_ring_save(Buffer *buffer, KillRing *kr);



void insertChar(Buffer *buffer, char c);

void right_char(Buffer * buffer, bool shift);
void left_char(Buffer * buffer, bool shift);
void previous_line(Buffer *buffer, bool shift);
void next_line(Buffer *buffer, bool shift);
void move_end_of_line(Buffer *buffer, bool shift);
void move_beginning_of_line(Buffer * buffer, bool shift);

void delete_char(Buffer *buffer);
void kill_line(Buffer *buffer, KillRing *kr);

void open_line(Buffer *buffer);
void indent(Buffer *buffer);
void delete_indentation(Buffer *buffer);
bool isWordChar(char c);
/* void backward_kill_word(Buffer *buffer); */
void addIndentation(Buffer *buffer, int indentation);
void removeIndentation(Buffer *buffer, int indentation);
void duplicate_line(Buffer *buffer);

bool is_word_char(char c);
bool is_punctuation_char(char c);
bool backward_word(Buffer *buffer, int count);
bool forward_word(Buffer *buffer, int count);
void backward_kill_word(Buffer *buffer, KillRing *kr);
void forward_paragraph(Buffer *buffer);
void backward_paragraph(Buffer *buffer);




#endif
