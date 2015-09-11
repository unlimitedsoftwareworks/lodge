/**
 * Basic drawing functionality.
 *
 * Authors: Tim Sjöstrand <tim.sjostrand@gmail.com>
 *			Johan Yngman <johan.yngman@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <time.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#ifdef EMSCRIPTEN
	#include <emscripten/emscripten.h>
#endif

#include "graphics.h"
#include "math4.h"
#include "texture.h"

#define VERTICES_RECT_LEN 30

const float vertices_rect[] = {  
	// Vertex			 // Texcoord
	-0.5f,	0.5f,  0.0f, 0.0f, 0.0f, // Top-left?
	-0.5f, -0.5f,  0.0f, 0.0f, 1.0f, // Bottom-left ?
	 0.5f,	0.5f,  0.0f, 1.0f, 0.0f, // Top-right ?
	 0.5f,	0.5f,  0.0f, 1.0f, 0.0f, // Top-right ?
	-0.5f, -0.5f,  0.0f, 0.0f, 1.0f, // Bottom-left ?
	 0.5f, -0.5f,  0.0f, 1.0f, 1.0f, // Bottom-right ?
};

#ifdef EMSCRIPTEN
struct graphics* graphics_global;
#endif

/* TODO: separate into sprite.c */
void sprite_render(struct sprite *sprite, struct shader *s, struct graphics *g)
{
	glEnableVertexAttribArray(0);

	glBindVertexArray(g->vao_rect);

	// Position, rotation and scale
	mat4 transform_position;
	translate(transform_position, xyz(sprite->pos));
	
	mat4 transform_scale;
	scale(transform_scale, xyz(sprite->scale));
	
	mat4 transform_rotation;
	rotate_z(transform_rotation, sprite->rotation);

	mat4 transform_final;
	mult(transform_final, transform_position, transform_rotation);
	mult(transform_final, transform_final, transform_scale);
	transpose_same(transform_final);
	
	// Upload matrices and color
	glUseProgram(s->program);
	glUniformMatrix4fv(s->uniform_transform, 1, GL_FALSE, transform_final);
	glUniformMatrix4fv(s->uniform_projection, 1, GL_FALSE, g->projection);
	glUniform4fv(s->uniform_color, 1, sprite->color);
	glUniform1i(s->uniform_sprite_type, sprite->type);
	glUniform1i(s->uniform_tex, 0);

	// Render it!
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, *(sprite->texture));
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

static int graphics_opengl_init(struct graphics *g, int view_width, int view_height)
{
	/* Global transforms. */
	translate(g->translate, 0.0f, 0.0f, 0.0f);
	scale(g->scale, 10.0f, 10.0f, 1);
	rotate_z(g->rotate, 0);
	ortho(g->projection, 0, view_width, view_height, 0, -1.0f, 1.0f);
	transpose_same(g->projection);

	/* OpenGL. */
	// glViewport( 0, 0, view_width, view_height );
	glClearColor(0.33f, 0.33f, 0.33f, 0.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	//glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	/* Vertex buffer. */
	glGenBuffers(1, &g->vbo_rect);
	glBindBuffer(GL_ARRAY_BUFFER, g->vbo_rect);
	glBufferData(GL_ARRAY_BUFFER, VERTICES_RECT_LEN * sizeof(float),
			vertices_rect, GL_STATIC_DRAW);

	/* Vertex array. */
	glGenVertexArrays(1, &g->vao_rect);
	glBindVertexArray(g->vao_rect);

	return GRAPHICS_OK;
}

void graphics_glfw_resize_callback(GLFWwindow *window, int width, int height)
{
	glViewport(0, 0, width, height);
}

int graphics_libraries_init(struct graphics *g, int view_width, int view_height,
		int windowed)
{
	/* Initialize the library */
	if(!glfwInit()) {
		return GRAPHICS_GLFW_ERROR;
	}

#ifdef EMSCRIPTEN
	g->window = glfwCreateWindow(view_width, view_height, "glpong", NULL, NULL);
	if(!g->window) {
		glfwTerminate();
		return GRAPHICS_GLFW_ERROR;
	}
#else
	/* QUIRK: Mac OSX */
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWmonitor *monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode *video_mode = glfwGetVideoMode(monitor);

	/* Create a windowed mode window and its OpenGL context */
	if(windowed) {
		g->window = glfwCreateWindow(view_width, view_height, "glpong", NULL, NULL);
	} else {
		g->window = glfwCreateWindow(video_mode->width, video_mode->height, "glpong", monitor, NULL);
	}
	if(!g->window) {
		glfwTerminate();
		return GRAPHICS_GLFW_ERROR;
	}
#endif

	/* Be notified when window size changes. */
	glfwSetFramebufferSizeCallback(g->window, &graphics_glfw_resize_callback);

	/* Select the current OpenGL context. */
	glfwMakeContextCurrent(g->window);

	/* Init GLEW. */
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if(err != GLEW_OK) {
		return GRAPHICS_GLEW_ERROR;
	}

	return GRAPHICS_OK;
}

/**
 * @param g						A graphics struct to fill in.
 * @param view_width			The width of the view, used for ortho().
 * @param view_height			The height of the view, used for ortho().
 * @param windowed				If applicable, whether to start in windowed mode.
 */
int graphics_init(struct graphics *g, think_func_t think, render_func_t render,
		int view_width, int view_height, int windowed)
{
	int ret = 0;

#ifdef EMSCRIPTEN
	/* HACK: Need to store reference for emscripten. */
	graphics_global = g;
#endif

	/* Set up the graphics struct properly. */
	g->delta_time_factor = 1.0f;
	g->think = think;
	g->render = render;

	/* Set up GLEW and glfw. */
	ret = graphics_libraries_init(g, view_width, view_height, windowed);

	if(ret != GRAPHICS_OK) {
		return ret;
	}

	/* Set up OpenGL. */
	ret = graphics_opengl_init(g, view_width, view_height);

	if(ret != GRAPHICS_OK) {
		return ret;
	}

	return GRAPHICS_OK;
}

void graphics_free(struct graphics *g)
{
	/* Free resources. */
	glDeleteVertexArrays(1, &g->vao_rect);
	glDeleteBuffers(1, &g->vbo_rect);

	/* Shut down glfw. */
	glfwTerminate();
}

static void graphics_count_frame(struct frames *f)
{
	f->frames ++;
	if(now() - f->last_frame_report >= 1000.0) {
		graphics_debug("FPS: % 6d, Frame-Time (min/max/avg): % 5.1f /% 5.1f /% 5.1f ms\n",
				f->frames, f->frame_time_min, f->frame_time_max,
				f->frame_time_sum/f->frames);
		f->frame_time_max = FLT_MIN;
		f->frame_time_min = FLT_MAX;
		f->frame_time_sum = 0;
		f->last_frame_report = now();
		f->frames = 0;
	}
}

static void graphics_frames_register(struct frames *f, float delta_time)
{
	f->frame_time_min = fmin(delta_time, f->frame_time_min);
	f->frame_time_max = fmax(delta_time, f->frame_time_max);
	f->frame_time_sum += delta_time;
}

/**
 * Returns the number of milliseconds since the program was started.
 */
double now()
{
	return glfwGetTime() * 1000.0;
}

void graphics_do_frame(struct graphics *g)
{	  
	double before = now();

	/* Delta-time. */
	float delta_time = 0;
	if(g->frames.last_frame != 0) {
		delta_time = (now() - g->frames.last_frame) * g->delta_time_factor;
	}
	g->frames.last_frame = now();

	/* Game loop. */
	g->think(g, delta_time);
	g->render(g, delta_time);

	/* Swap front and back buffers */
	glfwSwapBuffers(g->window);

	/* Poll for and process events */
	glfwPollEvents();

	/* Register that a frame has been drawn. */
	graphics_frames_register(&g->frames, now() - before);
	graphics_count_frame(&g->frames);
}

#ifdef EMSCRIPTEN
void graphics_do_frame_emscripten()
{
	graphics_do_frame(graphics_global);
}
#endif

void graphics_loop(struct graphics *g)
{
	/* Sanity check. */
	if(!g->render || !g->think) {
		graphics_error("g->render() or g->think() not set!\n");
		return;
	}

#ifdef EMSCRIPTEN
	emscripten_set_main_loop(graphics_do_frame_emscripten, 0, 1);
#else
	while(!glfwWindowShouldClose(g->window)) {
		graphics_do_frame(g);
	}
#endif
}
