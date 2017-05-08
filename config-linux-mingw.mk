CFLAGS += \
	-Imingw/SDL2/x86_64-w64-mingw32/include \
	-Imingw/glew/include/
LDFLAGS += \
	-Lmingw/SDL2/x86_64-w64-mingw32/lib \
	-Lmingw/glew/lib/ \
	-mwindows -lmingw32 -lm -lopengl32 -lglew32 -lSDL2main -lSDL2

CC = x86_64-w64-mingw32-gcc

TARGET = game.exe
OBJ_SUFFIX=win.o
