#if 0
set -e; [ "$0" -nt "$0.bin" ] &&
gcc -Wall -Wextra -pedantic -std=c99 "$0" -lGL -lGLEW -lSDL2 -o "$0.bin"
exec "$0.bin" "$@"
#endif

#define _REENTRANT

#define GL3_PROTOTYPES 1
#include <GL/glew.h>

#include <SDL2/SDL.h>

#include <stdint.h>
#include <stdbool.h>

#define FPS 60
#define STEP_MS (1000/(FPS))

void
render()
{
}

void
framelimit(Uint32 *prevtime)
{
	Uint32 time = SDL_GetTicks();
	Uint32 diff = time - *prevtime;

	if(diff < STEP_MS) {
		diff = STEP_MS - diff;
		SDL_Delay(diff);
	}

	*prevtime = time;
}

int
loop(SDL_Window *win, SDL_GLContext ctx)
{
	(void)ctx;

	bool run = true;
	SDL_Event ev;
	Uint32 prevtime = SDL_GetTicks();

	while(run) {
		while(SDL_PollEvent(&ev)) {
			switch(ev.type) {
			case SDL_QUIT:
				return 0;
			case SDL_KEYDOWN:
				switch(ev.key.keysym.scancode) {
				case SDL_SCANCODE_ESCAPE:
					return 0;
				default:
					break;
				}
			default:
				break;
			}
		}

		glClear(GL_COLOR_BUFFER_BIT);
		render();
		SDL_GL_SwapWindow(win);
		framelimit(&prevtime);
	}

	return 0;
}

int
main(int argc, char **argv)
{
	(void)argc; (void)argv;
	int rc = -1;
	SDL_Window *win;
	SDL_GLContext ctx;

	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
		fprintf(stderr, "SDL_Init failed %s\n", SDL_GetError());
		return -1;
	}

	win = SDL_CreateWindow(
		"verlet",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		800, 600,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
	);
	if(win == NULL) {
		fprintf(stderr, "SDL_CreateWindow failed %s\n", SDL_GetError());
		goto out_sdl;
	}

	ctx = SDL_GL_CreateContext(win);
	if(ctx == NULL) {
		fprintf(stderr, "SDL_GL_CreateContext failed %s\n", SDL_GetError());
		goto out_win;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetSwapInterval(1);

	glewExperimental = GL_TRUE;
	glewInit();

	glClearColor(0.0, 0.25, 0.0, 1.0);

	rc = loop(win, ctx);

	SDL_GL_DeleteContext(ctx);
out_win:
	SDL_DestroyWindow(win);
out_sdl:
	SDL_Quit();
	return rc;
}
