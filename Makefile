CC := clang
CFLAGS := -Wall -Wextra -I/usr/include/freetype2 -I. -g -fPIC # -O3
LDLIBS := -lglfw -lGL -lGLEW -lm -lfreetype -lfontconfig
TARGET := lume
LIB_NAME := liblume
SOURCES := $(wildcard ./*.c)
OBJECTS := $(SOURCES:.c=.o)
INSTALL_DIR := /usr

# Default target
all: $(TARGET) $(LIB_NAME).a $(LIB_NAME).so

# Executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDLIBS) -o $@

# Static library
$(LIB_NAME).a: $(OBJECTS)
	ar rcs $@ $(OBJECTS)

# Dynamic library
$(LIB_NAME).so: $(OBJECTS)
	$(CC) -shared -o $@ $(OBJECTS) $(LDLIBS)

# Rule for building object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Phony targets for cleaning up and installation
.PHONY: clean remove install

clean:
	rm -f $(OBJECTS) $(TARGET) $(LIB_NAME).a $(LIB_NAME).so

remove: clean
	rm -f $(TARGET)

install:
	install -d $(INSTALL_DIR)/lib
	install -m 644 $(LIB_NAME).a $(INSTALL_DIR)/lib
	install -m 755 $(LIB_NAME).so $(INSTALL_DIR)/lib
	install -d $(INSTALL_DIR)/include
	install -m 644 ./*.h $(INSTALL_DIR)/include

# CC := clang
# CFLAGS := -Wall -O3 -Wextra -I/usr/include/freetype2 -I. -g -fPIC
# LDLIBS := -lglfw -lGL -lGLEW -lm -lfreetype -lfontconfig
# TARGET := lume
# LIB_NAME := liblume
# SOURCES := $(wildcard ./*.c)
# OBJECTS := $(SOURCES:.c=.o)
# INSTALL_DIR := /usr/local

# # Default target
# all: $(TARGET) $(LIB_NAME).a $(LIB_NAME).so

# # Executable
# $(TARGET): $(OBJECTS)
# 	$(CC) $(OBJECTS) $(LDLIBS) -o $@

# # Static library
# $(LIB_NAME).a: $(OBJECTS)
# 	ar rcs $@ $(OBJECTS)

# # Dynamic library
# $(LIB_NAME).so: $(OBJECTS)
# 	$(CC) -shared -o $@ $(OBJECTS) $(LDLIBS)

# # Rule for building object files
# %.o: %.c
# 	$(CC) $(CFLAGS) -c $< -o $@

# # Phony targets for cleaning up and installation
# .PHONY: clean remove install

# clean:
# 	rm -f $(OBJECTS) $(TARGET) $(LIB_NAME).a $(LIB_NAME).so

# remove: clean
# 	rm -f $(TARGET)

# install:
# 	install -d $(INSTALL_DIR)/lib
# 	install -m 644 $(LIB_NAME).a $(INSTALL_DIR)/lib
# 	install -m 755 $(LIB_NAME).so $(INSTALL_DIR)/lib
# 	install -d $(INSTALL_DIR)/include
# 	install -m 644 ./*.h $(INSTALL_DIR)/include
