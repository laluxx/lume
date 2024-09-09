#ifndef EDIT_H
#define EDIT_H

#include "buffer.h"

void insertChar(Buffer *buffer, char c);

void right_char(Buffer *buffer);
void left_char(Buffer *buffer);
void previous_line(Buffer *buffer);
void next_line(Buffer *buffer);
void move_end_of_line(Buffer *buffer);
void move_beginning_of_line(Buffer * buffer);
void delete_char(Buffer *buffer);
void kill_line(Buffer *buffer);
void open_line(Buffer *buffer);
void indent(Buffer *buffer);
void delete_indentation(Buffer *buffer);
bool isWordChar(char c);
void backward_kill_word(Buffer *buffer);


#endif
