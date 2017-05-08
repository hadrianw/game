#define GL3_PROTOTYPES 1
#include <GL/glew.h>

#include <SDL2/SDL.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <math.h>

#define LEN(x) (sizeof(x)/sizeof((x)[0]))

#define FPS 60
#define STEP_MS (1000/(FPS))

#define DISC_SUBDIV 16

//*
const char vertex_glsl[] =
"#version 330 core\n"
"\n"
"layout (location = 0) in vec2 position;\n"
"layout (location = 1) in vec2 offset;\n"
"\n"
"uniform mat3x2 transform;\n"
"\n"
"void main()\n"
"{\n"
"	gl_Position = vec4(\n"
"		(transform * vec3(position + offset, 1)).xy, 0.0, 1.0\n"
"	);\n"
"}\n";
const int vertex_glsl_size = sizeof(vertex_glsl);

const char fragment_glsl[] =
"#version 330 core\n"
"\n"
"out vec4 color;\n"
"\n"
"void main()\n"
"{\n"
"	color = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
"}\n";
const int fragment_glsl_size = sizeof(fragment_glsl);

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

struct vao {
	GLuint vao;
	GLenum mode;
	GLsizei count;
	GLsizei primcount;
};

struct scene {
	struct projection projection;
	mat3x2 projection_matrix;
	struct {
		GLuint program;
		GLint transform;
	} shader;
	struct {
		struct vao disc;
	} vao;
};

void
projection_base_diagonal(struct projection *proj, int width, int height)
{
	proj->right = 100.f * 1.f / sqrtf(width * width + height * height);
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

void
gen_disc_vao(struct vao *vao_out)
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
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0, 2, GL_FLOAT, GL_FALSE,
		sizeof(buf[0]), (GLvoid*)0
	);

	glBindVertexArray(0);

	vao_out->vao = vao;
	vao_out->mode = GL_TRIANGLE_FAN;
	vao_out->count = LEN(buf);
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

void
render(struct scene *scene)
{
	glBindVertexArray(scene->vao.disc.vao);
	glUseProgram(scene->shader.program);
	glDrawArraysInstanced(
		scene->vao.disc.mode,
		0, scene->vao.disc.count,
		scene->vao.disc.primcount
	);
	glBindVertexArray(0);
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

	gen_disc_vao(&scene.vao.disc);

	vec2 pos[2000];
	vec2 prev_pos[LEN(pos)];
	vec2 acc[LEN(pos)];

	for(size_t i = 0; i < LEN(pos); i++) {
		int width = 800;
		int height = 600;
		pos[i].x = scene.projection.right + (rand() % width);
		pos[i].y = scene.projection.bottom + (rand() % height);
	}
	scene.vao.disc.primcount = LEN(pos);

	GLuint instances_vbo;
	glGenBuffers(1, &instances_vbo);

	glBindVertexArray(scene.vao.disc.vao);

	glBindBuffer(GL_ARRAY_BUFFER, instances_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(pos), pos, GL_STREAM_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
		1, 2, GL_FLOAT, GL_FALSE,
		sizeof(pos[0]), (GLvoid*)0
	);

	glVertexAttribDivisor(1, 1);

	glBindVertexArray(0);

	bool run = true;
	SDL_Event ev;
	Uint32 prevtime = SDL_GetTicks();
	srand(prevtime);

	float delta = 1.f / FPS;

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
					for(size_t i = 0; i < LEN(pos); i++) {
						int width = scene.projection.right - scene.projection.left - 4.f;
						int height = scene.projection.top - scene.projection.bottom - 4.f;
						pos[i].x = scene.projection.left + (rand() % width) + 2.f;
						pos[i].y = scene.projection.bottom + (rand() % height) + 2.f;
					}
					memset(prev_pos, 0, sizeof(prev_pos));
					memset(acc, 0, sizeof(acc));
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

		// gravity
		for(size_t i = 0; i < LEN(acc); i++) {
		    acc[i].y += -2.5f;
		}

		// acc
		for(size_t i = 0; i < LEN(pos); i++) {
		    pos[i].x += acc[i].x * delta * delta;
		    pos[i].y += acc[i].y * delta * delta;
		}
		memset(acc, 0, sizeof(acc));

		// collide
		for(size_t i = 0; i < LEN(pos); i++){
			for(size_t j = i + 1; j < LEN(pos); j++){
				vec2 diff = {
					pos[i].x - pos[j].x,
					pos[i].y - pos[j].y
				};
				float sqlen = diff.x * diff.x + diff.y * diff.y;
				float radius = 1.f;
				float target = radius + radius;
				float sqtarget = target * target;

				if(sqlen < sqtarget) {
					float len = sqrtf(sqlen);
					float mv = 0.5f * (len - target) / len;
					pos[i].x -= diff.x * mv;
					pos[i].y -= diff.y * mv;
					pos[j].x += diff.x * mv;
					pos[j].y += diff.y * mv;
				}
			}
		}

		// border collide
		for(size_t i = 0; i < LEN(pos); i++) {
			float radius = 1.f;

			if(pos[i].x - radius < scene.projection.left){
				pos[i].x = scene.projection.left + radius;
			}
			else if(pos[i].x + radius > scene.projection.right){
				pos[i].x = scene.projection.right - radius;
			}

			if(pos[i].y - radius < scene.projection.bottom){
				pos[i].y = scene.projection.bottom + radius;
			}
			else if(pos[i].y + radius > scene.projection.top){
				pos[i].y = scene.projection.top - radius;
			}
		}

		// inertia
		for(size_t i = 0; i < LEN(pos); i++) {
			vec2 new_pos = {
				pos[i].x * 2 - prev_pos[i].x,
				pos[i].y * 2 - prev_pos[i].y
			};
			prev_pos[i] = pos[i];
			pos[i] = new_pos;
		}
		glBindBuffer(GL_ARRAY_BUFFER, instances_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(pos), NULL, GL_STREAM_DRAW);
		glBufferData(GL_ARRAY_BUFFER, sizeof(pos), pos, GL_STREAM_DRAW);

		glClear(GL_COLOR_BUFFER_BIT);

		render(&scene);

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
