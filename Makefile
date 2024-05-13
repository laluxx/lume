CC = clang
TARGET = lume
CFLAGS = -Wall -Wextra -I.
LFLAGS = -lglfw -lGL -lGLEW
SOURCES = ./*.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) $(LFLAGS) -o $(TARGET)

.PHONEY: clean
clean:
	@rm -f $(TARGET)

.PHONEY: remove
remove: clean
	@rm -f $(TARGET)
