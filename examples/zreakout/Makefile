CC = gcc
CFLAGS = -Wall -Wextra -g -I/usr/include/freetype2 -I.
LIBS = -llume
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

TARGET = breakout

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
