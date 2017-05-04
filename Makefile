OPTIM = -g -O0 -Werror
#OPTIM = -O2
CFLAGS = -std=c99 $(OPTIM) -Wall -Wextra -pedantic \
	-fno-omit-frame-pointer \
	-Wno-missing-field-initializers \
	-Wno-missing-braces \
	-D_XOPEN_SOURCE \
	-D_REENTRANT
LDFLAGS = -lm -lGL -lGLEW -lSDL2

CC = gcc

SRC = game.c
SHDR = vertex.glsl fragment.glsl
OBJ = $(SRC:.c=.o) $(SHDR:.glsl=.o)

all: game

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.glsl
	./bin2o.sh $< $@

$(OBJ): Makefile

game: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

clean:
	rm -f game $(OBJ)

.PHONY: all clean
