CC := clang
CFLAGS := -Wall -Wextra -I/usr/include/freetype2 -I. -g
LDLIBS := -lglfw -lGL -lGLEW -lm -lfreetype
TARGET := lume
SOURCES := $(wildcard ./*.c)
OBJECTS := $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDLIBS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean remove

clean:
	rm -f $(OBJECTS) $(TARGET)

remove: clean
	rm -f $(TARGET)
