/**
 * Render drawables (vertex buffer objects) and utility functions to create them
 * from simple geometric primitives.
 *
 * Author: Tim Sjöstrand <tim.sjostrand@gmail.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <GL/glew.h>
#include <math.h>

#include "drawable.h"
#include "math4.h"
#include "graphics.h"
#include "color.h"

/**
 * Upload vertices to GPU.
 */
static void drawable_set_vbo(GLfloat *vertices, GLuint vertices_count, struct shader *s,
		GLuint *vbo, GLuint *vao)
{
	/* Generate new vertex array? */
	if(*vao == 0) {
		glGenVertexArrays(1, vao);
		GL_OK_OR_RETURN;
	}

	/* Bind vertex array. */
	glBindVertexArray(*vao);
	GL_OK_OR_RETURN;

	/* Generate vertex buffer? */
	if(*vbo == 0) {
		glGenBuffers(1, vbo);
		GL_OK_OR_RETURN;
	}

	/* Bind and upload buffer. */
	glBindBuffer(GL_ARRAY_BUFFER, *vbo);
	GL_OK_OR_RETURN;
	glBufferData(GL_ARRAY_BUFFER, vertices_count * VBO_VERTEX_LEN * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
	GL_OK_OR_RETURN;

	/* Position stream. */
	GLint posAttrib = glGetAttribLocation(s->program, ATTRIB_NAME_POSITION);
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, VBO_VERTEX_LEN * sizeof(GLfloat), NULL);
	GL_OK_OR_RETURN;

	/* Texcoord stream. */
	GLint texcoordAttrib = glGetAttribLocation(s->program, ATTRIB_NAME_TEXCOORD);
	glEnableVertexAttribArray(texcoordAttrib);
	glVertexAttribPointer(texcoordAttrib, 2, GL_FLOAT, GL_FALSE, VBO_VERTEX_LEN * sizeof(GLfloat),
			(void *) (3 * sizeof(GLfloat)));
	GL_OK_OR_RETURN;
}

/**
 * Obtain a matrix of vertices (xyzuv) for a circle with the given segments.
 *
 * Inspired by http://slabode.exofire.net/circle_draw.shtml.
 *
 * @param dst		The buffer that will be filled with the vertices for the circle.
 *					Size should be ((segments + 1) * vertex_length), where
 *					vertex_length is 5 (xyzuv).
 * @param cx		Center X coordinate of the circle.
 * @param cy		Center Y coordinate of the circle.
 * @param r			Radius of the circle.
 * @param segments	The number of line segments to create draw this circle from.
 *					Greater number produces a better approximation of a circle
 *					but worsens performance.
 */
static void drawable_get_vertices_circle(GLfloat *dst, float cx, float cy, float r, int segments)
{
	float theta = 2 * M_PI / (float) (segments - 1);
	float c = cosf(theta); // Precalculate.
	float s = sinf(theta);
	float t;

	float x = r; // Start at angle = 0.
	float y = 0;

	for(int i = 0; i < segments; i++) {
		/* Set vertex. */
		dst[i * VBO_VERTEX_LEN + 0] = x + cx;
		dst[i * VBO_VERTEX_LEN + 1] = y + cy;
		dst[i * VBO_VERTEX_LEN + 2] = 0.0f;
		dst[i * VBO_VERTEX_LEN + 3] = 0.0f;
		dst[i * VBO_VERTEX_LEN + 4] = 0.0f;

		/* Apply the rotation matrix. */
		t = x;
		x = c * x - s * y;
		y = s * t + c * y;
	}
}

/**
 * Produces a matrix of vertices (xyzuv) for a rectangle of the given
 * dimensions.
 *
 * @param dst	Destination buffer. Size should be (6 * vertex_length) where
 *				vertex_length is 5 (xyzuv).
 * @param x		Center X coordinate.
 * @param y		Center Y coordinate.
 * @param w		Width.
 * @param h		Height.
 */
static void drawable_get_vertices_rect_solid(GLfloat *dst, float x, float y, float w, float h)
{
	w /= 2.0f;
	h /= 2.0f;

	/* Top-left */
	dst[0 * VBO_VERTEX_LEN + 0] = x - w;	// x
	dst[0 * VBO_VERTEX_LEN + 1] = y + h;	// y
	dst[0 * VBO_VERTEX_LEN + 2] = 0.0f;		// z
	dst[0 * VBO_VERTEX_LEN + 3] = 0.0f;		// u
	dst[0 * VBO_VERTEX_LEN + 4] = 0.0f;		// v
	/* Bottom-Left */
	dst[1 * VBO_VERTEX_LEN + 0] = x - w;	// x
	dst[1 * VBO_VERTEX_LEN + 1] = y - h;	// y
	dst[1 * VBO_VERTEX_LEN + 2] = 0.0f;		// z
	dst[1 * VBO_VERTEX_LEN + 3] = 0.0f;		// u
	dst[1 * VBO_VERTEX_LEN + 4] = 1.0f;		// v
	/* Top-right */
	dst[2 * VBO_VERTEX_LEN + 0] = x + w;	// x
	dst[2 * VBO_VERTEX_LEN + 1] = y + h;	// y
	dst[2 * VBO_VERTEX_LEN + 2] = 0.0f;		// z
	dst[2 * VBO_VERTEX_LEN + 3] = 1.0f;		// u
	dst[2 * VBO_VERTEX_LEN + 4] = 0.0f;		// v
	/* Top-right */
	dst[3 * VBO_VERTEX_LEN + 0] = x + w;	// x
	dst[3 * VBO_VERTEX_LEN + 1] = y + h;	// y
	dst[3 * VBO_VERTEX_LEN + 2] = 0.0f;		// z
	dst[3 * VBO_VERTEX_LEN + 3] = 1.0f;		// u
	dst[3 * VBO_VERTEX_LEN + 4] = 0.0f;		// v
	/* Bottom-left */
	dst[4 * VBO_VERTEX_LEN + 0] = x - w;	// x
	dst[4 * VBO_VERTEX_LEN + 1] = y - h;	// y
	dst[4 * VBO_VERTEX_LEN + 2] = 0.0f;		// z
	dst[4 * VBO_VERTEX_LEN + 3] = 0.0f;		// u
	dst[4 * VBO_VERTEX_LEN + 4] = 1.0f;		// v
	/* Bottom-right */
	dst[5 * VBO_VERTEX_LEN + 0] = x + w;	// x
	dst[5 * VBO_VERTEX_LEN + 1] = y - h;	// y
	dst[5 * VBO_VERTEX_LEN + 2] = 0.0f;		// z
	dst[5 * VBO_VERTEX_LEN + 3] = 1.0f;		// u
	dst[5 * VBO_VERTEX_LEN + 4] = 1.0f;		// v
}

/**
 * Produces a matrix of vertices (xyzuv) for the outline of a rectangle of
 * the given dimensions.
 *
 * @param dst	Destination buffer. Size should be (5 * vertex_length) where
 *				vertex_length is 5 (xyzuv).
 * @param x		Center X coordinate.
 * @param y		Center Y coordinate.
 * @param w		Width.
 * @param h		Height.
 */
static void drawable_get_vertices_rect_outline(GLfloat *dst, float x, float y, float w, float h)
{
	w /= 2.0f;
	h /= 2.0f;

	/* Top-left */
	dst[0 * VBO_VERTEX_LEN + 0] = x - w;	// x
	dst[0 * VBO_VERTEX_LEN + 1] = y + h;	// y
	dst[0 * VBO_VERTEX_LEN + 2] = 0.0f;		// z
	dst[0 * VBO_VERTEX_LEN + 3] = 0.0f;		// u
	dst[0 * VBO_VERTEX_LEN + 4] = 0.0f;		// v
	/* Top-right */
	dst[1 * VBO_VERTEX_LEN + 0] = x + w;	// x
	dst[1 * VBO_VERTEX_LEN + 1] = y + h;	// y
	dst[1 * VBO_VERTEX_LEN + 2] = 0.0f;		// z
	dst[1 * VBO_VERTEX_LEN + 3] = 0.0f;		// u
	dst[1 * VBO_VERTEX_LEN + 4] = 0.0f;		// v
	/* Bottom-right */
	dst[2 * VBO_VERTEX_LEN + 0] = x + w;	// x
	dst[2 * VBO_VERTEX_LEN + 1] = y - h;	// y
	dst[2 * VBO_VERTEX_LEN + 2] = 0.0f;		// z
	dst[2 * VBO_VERTEX_LEN + 3] = 0.0f;		// u
	dst[2 * VBO_VERTEX_LEN + 4] = 0.0f;		// v
	/* Bottom-left */
	dst[3 * VBO_VERTEX_LEN + 0] = x - w;	// x
	dst[3 * VBO_VERTEX_LEN + 1] = y - h;	// y
	dst[3 * VBO_VERTEX_LEN + 2] = 0.0f;		// z
	dst[3 * VBO_VERTEX_LEN + 3] = 0.0f;		// u
	dst[3 * VBO_VERTEX_LEN + 4] = 0.0f;		// v
	/* Top-left */
	dst[4 * VBO_VERTEX_LEN + 0] = x - w;	// x
	dst[4 * VBO_VERTEX_LEN + 1] = y + h;	// y
	dst[4 * VBO_VERTEX_LEN + 2] = 0.0f;		// z
	dst[4 * VBO_VERTEX_LEN + 3] = 0.0f;		// u
	dst[4 * VBO_VERTEX_LEN + 4] = 0.0f;		// v
}

/**
 * Produces a matrix of vertices (xyzuv) for the outline of a rectangle of
 * the given dimensions.
 *
 * @param dst	Destination buffer. Size should be (2 * vertex_length) where
 *				vertex_length is 5 (xyzuv).
 * @param x1	Start X coordinate.
 * @param y1	Start Y coordinate.
 * @param x2	End X coordinate.
 * @param y2	End Y coordinate.
 */
static void drawable_get_vertices_line(GLfloat *dst, float x1, float y1, float x2, float y2)
{
	/* Start */
	dst[0 * VBO_VERTEX_LEN + 0] = x1;		// x
	dst[0 * VBO_VERTEX_LEN + 1] = y1;		// y
	dst[0 * VBO_VERTEX_LEN + 2] = 0.0f;		// z
	dst[0 * VBO_VERTEX_LEN + 3] = 0.0f;		// u
	dst[0 * VBO_VERTEX_LEN + 4] = 0.0f;		// v
	/* End */
	dst[1 * VBO_VERTEX_LEN + 0] = x2;		// x
	dst[1 * VBO_VERTEX_LEN + 1] = y2;		// y
	dst[1 * VBO_VERTEX_LEN + 2] = 0.0f;		// z
	dst[1 * VBO_VERTEX_LEN + 3] = 0.0f;		// u
	dst[1 * VBO_VERTEX_LEN + 4] = 0.0f;		// v
}

void drawable_render(struct drawable *d)
{
	glBindVertexArray(d->vao);
	glDrawArrays(d->draw_mode, 0, d->vertex_count);
}

void drawable_render_detailed(GLenum mode, GLuint vbo, GLuint vbo_count, GLuint vao, GLuint *tex, vec4 color, struct shader *s, mat4 transform)
{
	/* Bind vertices. */
	glUseProgram(s->program);
	glBindVertexArray(vao);

	/* Upload color */
	GLint uniform_color = glGetUniformLocation(s->program, "color");
	glUniform4fv(uniform_color, 1, color);

	/* Upload transform */
	GLint uniform_transform = glGetUniformLocation(s->program, "transform");
	glUniformMatrix4fv(uniform_transform, 1, GL_FALSE, (GLfloat*)transform);

	/* Render it! */
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, (*tex));
	glDrawArrays(mode, 0, vbo_count);
}

void drawable_render_simple(struct drawable *d, struct shader *s, GLuint *tex, vec4 color, mat4 transform)
{
	drawable_render_detailed(d->draw_mode, d->vbo, d->vertex_count, d->vao, tex, color, s, transform);
}

void drawable_new_rect_outline(struct drawable *dst, struct rect *rect, struct shader *s)
{
	drawable_new_rect_outlinef(dst, rect->pos[0], rect->pos[1], rect->size[0], rect->size[1], s);
}

void drawable_new_rect_outlinef(struct drawable *dst, float x, float y, float w, float h, struct shader *s)
{
	dst->draw_mode = GL_LINE_STRIP;
	/* Allocate memory for vertices. */
	dst->vertex_count = 5;
	GLfloat *vertices = (GLfloat *) calloc(dst->vertex_count * VBO_VERTEX_LEN, sizeof(GLfloat));

	/* OOM? */
	if(vertices == NULL) {
		graphics_error("Out of memory\n");
		return;
	}

	/* Calculate vertices. */
	drawable_get_vertices_rect_outline(vertices, x, y, w, h);

	/* Upload vertices to GPU. */
	drawable_set_vbo(vertices, dst->vertex_count, s, &dst->vbo, &dst->vao);

	/* Cleanup. */
	free(vertices);
}

void drawable_new_rect_solidf(struct drawable *dst, float x, float y, float w, float h, struct shader *s)
{
	dst->draw_mode = GL_TRIANGLES;
	/* Allocate memory for vertices. */
	dst->vertex_count = 6;
	GLfloat *vertices = (GLfloat *) calloc(dst->vertex_count * VBO_VERTEX_LEN, sizeof(GLfloat));

	/* OOM? */
	if(vertices == NULL) {
		graphics_error("Out of memory\n");
		return;
	}

	/* Calculate vertices. */
	drawable_get_vertices_rect_solid(vertices, x, y, w, h);

	/* Upload vertices to GPU. */
	drawable_set_vbo(vertices, dst->vertex_count, s, &dst->vbo, &dst->vao);

	/* Cleanup. */
	free(vertices);
}

void drawable_new_rect_fullscreen(struct drawable *dst, struct shader *s)
{
	dst->draw_mode = GL_TRIANGLES;
	/* Allocate memory for vertices. */
	dst->vertex_count = 6;
	GLfloat *vertices = (GLfloat *)calloc(dst->vertex_count * VBO_VERTEX_LEN, sizeof(GLfloat));

	/* OOM? */
	if (vertices == NULL) {
		graphics_error("Out of memory\n");
		return;
	}

	/* Top-left */
	vertices[0 * VBO_VERTEX_LEN + 0] = -1.0f;	// x
	vertices[0 * VBO_VERTEX_LEN + 1] = 1.0f;	// y
	vertices[0 * VBO_VERTEX_LEN + 2] = 0.0f;	// z
	vertices[0 * VBO_VERTEX_LEN + 3] = 0.0f;	// u
	vertices[0 * VBO_VERTEX_LEN + 4] = 1.0f;	// v

	/* Bottom-right */
	vertices[1 * VBO_VERTEX_LEN + 0] = 1.0f;	// x
	vertices[1 * VBO_VERTEX_LEN + 1] = -1.0f;	// y
	vertices[1 * VBO_VERTEX_LEN + 2] = 0.0f;	// z
	vertices[1 * VBO_VERTEX_LEN + 3] = 1.0f;	// u
	vertices[1 * VBO_VERTEX_LEN + 4] = 0.0f;	// v

	/* Top-right */
	vertices[2 * VBO_VERTEX_LEN + 0] = 1.0f;	// x
	vertices[2 * VBO_VERTEX_LEN + 1] = 1.0f;	// y
	vertices[2 * VBO_VERTEX_LEN + 2] = 0.0f;	// z
	vertices[2 * VBO_VERTEX_LEN + 3] = 1.0f;	// u
	vertices[2 * VBO_VERTEX_LEN + 4] = 1.0f;	// v

	/* Top-left */
	vertices[3 * VBO_VERTEX_LEN + 0] = -1.0f;	// x
	vertices[3 * VBO_VERTEX_LEN + 1] = 1.0f;	// y
	vertices[3 * VBO_VERTEX_LEN + 2] = 0.0f;	// z
	vertices[3 * VBO_VERTEX_LEN + 3] = 0.0f;	// u
	vertices[3 * VBO_VERTEX_LEN + 4] = 1.0f;	// v

	/* Bottom-left */
	vertices[4 * VBO_VERTEX_LEN + 0] = -1.0f;	// x
	vertices[4 * VBO_VERTEX_LEN + 1] = -1.0f;	// y
	vertices[4 * VBO_VERTEX_LEN + 2] = 0.0f;	// z
	vertices[4 * VBO_VERTEX_LEN + 3] = 0.0f;	// u
	vertices[4 * VBO_VERTEX_LEN + 4] = 0.0f;	// v

	/* Bottom-right */
	vertices[5 * VBO_VERTEX_LEN + 0] = 1.0f;	// x
	vertices[5 * VBO_VERTEX_LEN + 1] = -1.0f;	// y
	vertices[5 * VBO_VERTEX_LEN + 2] = 0.0f;	// z
	vertices[5 * VBO_VERTEX_LEN + 3] = 1.0f;	// u
	vertices[5 * VBO_VERTEX_LEN + 4] = 0.0f;	// v

	/* Upload vertices to GPU. */
	drawable_set_vbo(vertices, dst->vertex_count, s, &dst->vbo, &dst->vao);

	/* Cleanup. */
	free(vertices);
}


void drawable_new_circle_outlinef(struct drawable *dst, float x, float y, float r, int segments, struct shader *s)
{
	dst->draw_mode = GL_LINE_STRIP;
	/* The actual required vertex count is segments+1 because the final vertex
	 * needs to reconnect with the first one. */
	dst->vertex_count = segments + 1;

	/* Allocate memory for vertices. */
	GLfloat *vertices = (GLfloat *) calloc(dst->vertex_count * VBO_VERTEX_LEN, sizeof(GLfloat));

	/* OOM? */
	if(vertices == NULL) {
		graphics_error("Out of memory\n");
		return;
	}

	/* Calculate vertices. */
	drawable_get_vertices_circle(vertices, x, y, r, dst->vertex_count);

	/* Upload vertices to GPU. */
	drawable_set_vbo(vertices, dst->vertex_count, s, &dst->vbo, &dst->vao);

	/* Cleanup. */
	free(vertices);
}

/**
 * Creates a vertex buffer for a circle.
 */
void drawable_new_circle_outline(struct drawable *dst, struct circle *circle, int segments, struct shader *s)
{
	drawable_new_circle_outlinef(dst, circle->pos[0], circle->pos[1], circle->r, segments, s);
}

void drawable_new_linef(struct drawable *dst, float x1, float y1, float x2, float y2, struct shader *s)
{
	dst->draw_mode = GL_LINE_STRIP;
	dst->vertex_count = 2;

	/* Allocate memory for vertices. */
	GLfloat *vertices = (GLfloat *) calloc(2 * VBO_VERTEX_LEN, sizeof(GLfloat));

	/* OOM? */
	if(vertices == NULL) {
		graphics_error("Out of memory\n");
		return;
	}

	/* Calculate vertices. */
	drawable_get_vertices_line(vertices, x1, y1, x2, y2);

	/* Upload vertices to GPU. */
	drawable_set_vbo(vertices, dst->vertex_count, s, &dst->vbo, &dst->vao);

	/* Cleanup. */
	free(vertices);
}

void drawable_free(struct drawable *d)
{
	glDeleteVertexArrays(1, &d->vao);
	glDeleteBuffers(1, &d->vbo);
	d->vertex_count = 0;
}

/**** DEPRECATED STUFF ****/

void sprite_render(struct basic_sprite *sprite, struct shader *s, struct graphics* g)
{
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

	drawable_render_detailed(GL_TRIANGLES, g->vbo_rect, VBO_QUAD_VERTEX_COUNT, g->vao_rect, sprite->texture, sprite->color, s, transform_final);
}

void sprite_init(struct basic_sprite *sprite, int type, float x, float y, float z,
		float w, float h, const vec4 color, float rotation, GLuint *texture)
{
	sprite->type = type;
	set4f(sprite->pos, x, y, z, 0.0f);
	set4f(sprite->scale, w, h, 1.0f, 1.0f);
	set4f(sprite->color, rgba(color));
	sprite->rotation = rotation;
	sprite->texture = texture;
}
