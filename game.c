#define GL3_PROTOTYPES 1
#include <GL/glew.h>

#include <SDL2/SDL.h>

#include <stdint.h>
#include <stdbool.h>

#include <math.h>

#define LEN(x) (sizeof(x)/sizeof((x)[0]))

#define FPS 60
#define STEP_MS (1000/(FPS))

#define DISC_SUBDIV 16

extern const char _binary_vertex_glsl_start;
extern const char _binary_vertex_glsl_end;

extern const char _binary_fragment_glsl_start;
extern const char _binary_fragment_glsl_end;

typedef struct {
	GLfloat x;
	GLfloat y;
} vec2;

GLuint
gen_disc_vbo()
{
	GLuint vao;
	GLuint vbo;

	vec2 buf[DISC_SUBDIV + 2] = {{0}};
	size_t i = 1;
	GLfloat angle;

	for(; i < LEN(buf); i++) {
		angle = (i-1) * 2 * M_PI / DISC_SUBDIV;
		buf[i].x = cosf(angle);
		buf[i].y = sinf(angle);
	}

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(buf), buf, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return vbo;
}

GLint
compile_shader(GLenum type, const GLchar *source, GLint size, GLuint *shader_out)
{
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, &size);
	glCompileShader(shader);

	GLint status;
	GLchar log[1024];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if(!status) {
		glGetShaderInfoLog(shader, sizeof(log), NULL, log);
		fprintf(stderr, "glCompileShader failed: %s\n", log);
		return -1;
	}
	*shader_out = shader;
	return 0;
}

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

	GLuint vertex_shader;
	GLuint fragment_shader;
	{
		const char *vertex_glsl = &_binary_vertex_glsl_start;
		size_t vertex_glsl_len = &_binary_vertex_glsl_end - &_binary_vertex_glsl_start;
		if(compile_shader(GL_VERTEX_SHADER, vertex_glsl, vertex_glsl_len, &vertex_shader)) {
			return -1;
		}
	}
	{
		const char *fragment_glsl = &_binary_fragment_glsl_start;
		size_t fragment_glsl_len = &_binary_fragment_glsl_end - &_binary_fragment_glsl_start;
		if(compile_shader(GL_FRAGMENT_SHADER, fragment_glsl, fragment_glsl_len, &fragment_shader)) {
			return -1;
		}
	}
	GLuint program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);
	{
		GLint status;
		GLchar log[1024];
		glGetProgramiv(program, GL_LINK_STATUS, &status);

		if(!status) {
			glGetProgramInfoLog(program, sizeof(log), NULL, log);
			fprintf(stderr, "glLinkProgram failed: %s\n", log);
			return -1;
		}
	}


	GLuint vbo = gen_disc_vbo();

	glDisable(GL_CULL_FACE);

	bool run = true;
	SDL_Event ev;
	Uint32 prevtime = SDL_GetTicks();

	while(run) {
		while(SDL_PollEvent(&ev)) {
			switch(ev.type) {
			case SDL_QUIT:
				return 0;
			case SDL_WINDOWEVENT:
				switch(ev.window.event) {
				case SDL_WINDOWEVENT_RESIZED:
					glViewport(0, 0, ev.window.data1, ev.window.data2);
					break;
				default:
					break;
				}
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

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(0);  

		glUseProgram(program);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glDrawArrays(GL_TRIANGLE_FAN, 0, DISC_SUBDIV + 2);

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

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetSwapInterval(1);

	ctx = SDL_GL_CreateContext(win);
	if(ctx == NULL) {
		fprintf(stderr, "SDL_GL_CreateContext failed %s\n", SDL_GetError());
		goto out_win;
	}

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
