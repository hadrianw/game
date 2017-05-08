#OPTIM = -g -O0 -Werror
OPTIM = -Ofast

CFLAGS = -std=c99 $(OPTIM) -Wall -Wextra -pedantic \
	-fno-omit-frame-pointer \
	-Wno-missing-field-initializers \
	-Wno-missing-braces \
	-D_XOPEN_SOURCE \
	-D_REENTRANT
LDFLAGS =

CONFIG ?= config-linux.mk
include $(CONFIG)

SRC = game.c
OBJ = $(SRC:.c=.$(OBJ_SUFFIX))

all: $(TARGET)

%.$(OBJ_SUFFIX): %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJ): Makefile

$(TARGET): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

clean:
	rm -f $(TARGET) $(OBJ)

.PHONY: all clean
