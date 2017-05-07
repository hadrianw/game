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

extern const char vertex_glsl[];
extern const int vertex_glsl_size;

extern const char fragment_glsl[];
extern const int fragment_glsl_size;

typedef struct {
	GLfloat x;
	GLfloat y;
} vec2;

typedef struct {
	vec2 col1;
	vec2 col2;
	vec2 pos;
} mat3x2;

static const mat3x2 mat3x2_identity = {{1.f, 0.f}, {0.f, 1.f}};

struct projection {
	float left;
	float top;
	float right;
	float bottom;
};

struct scene {
	struct projection projection;
	mat3x2 projection_matrix;
	struct {
		GLuint program;
		GLint transform;
	} shader;
};

void
projection_base_diagonal(struct projection *proj, int width, int height)
{
	proj->right = 100.f * 2.f / sqrtf(width * width + height * height);
	proj->top = proj->right;

	proj->right *= width;
	proj->left = -proj->right;
	proj->top *= height;
	proj->bottom = -proj->top;
}

void
projection_set_matrix(struct projection *proj, mat3x2 *mat)
{
	float invx = 1.f / (proj->right - proj->left);
	float invy = 1.f / (proj->top - proj->bottom);

	mat->col1.x = 2.f * invx;
	mat->col2.y = 2.f * invy;

	mat->pos.x = -(proj->right + proj->left) * invx;;
	mat->pos.y = -(proj->top + proj->bottom) * invy;;
}

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

int
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
compile_program(GLuint *program_out)
{
	int rc = -1;
	GLuint vertex_shader;
	GLuint fragment_shader;

	if(compile_shader(
		GL_VERTEX_SHADER, vertex_glsl, vertex_glsl_size,
		&vertex_shader
	)) {
		return -1;
	}

	if(compile_shader(
		GL_FRAGMENT_SHADER, fragment_glsl, fragment_glsl_size,
		&fragment_shader
	)) {
		goto out_delete_vs;
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

			glDeleteProgram(program);
			goto out_delete_fs;
		}
	}

	rc = 0;
	*program_out = program;
out_delete_fs:
	glDeleteShader(fragment_shader);
out_delete_vs:
	glDeleteShader(vertex_shader);

	return rc;
}

void
resize(struct scene *scene, int width, int height)
{
	glViewport(0, 0, width, height);
	projection_base_diagonal(&scene->projection, width, height);
	projection_set_matrix(&scene->projection, &scene->projection_matrix);
	glUseProgram(scene->shader.program);
	glUniformMatrix3x2fv(
		scene->shader.transform, 1, GL_FALSE,
		(GLfloat*)&scene->projection_matrix
	);
}

int
loop(SDL_Window *win, SDL_GLContext ctx)
{
	(void)ctx;

	struct scene scene = { .projection_matrix = mat3x2_identity };

	if(compile_program(&scene.shader.program) < 0) {
		return -1;
	}

	scene.shader.transform = glGetUniformLocation(
		scene.shader.program, "transform"
	);
	if(scene.shader.transform < 0) {
		fputs("glGetUniformLocation transform failed\n", stderr);
		return -1;
	}

	GLuint vbo = gen_disc_vbo();

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
					resize(
						&scene,
						ev.window.data1, ev.window.data2
					);
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

		glUseProgram(scene.shader.program);
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
