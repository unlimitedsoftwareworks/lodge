/**
* Spritebatch implementation with asynchronous vertex streaming using glMapBufferRange
*
* Author: Johan Yngman <johan.yngman@gmail.com>
*/

#include <stdlib.h>
#include <stdio.h>
#include <GL/glew.h>

#include "spritebatch.h"
#include "color.h"

#define STRIDE 5
#define CURRENT_SPRITE batch->sprite_count * STRIDE * 6

void spritebatch_create(struct spritebatch* batch)
{
	batch->offset_stream = 0;
	batch->offset_draw = 0;

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glGenBuffers(1, &batch->vbo);
	glGenVertexArrays(1, &batch->vao);
	glBindVertexArray(batch->vao);

	glBindBuffer(GL_ARRAY_BUFFER, batch->vbo);
	glBufferData(GL_ARRAY_BUFFER, SPRITEBATCH_BUFFER_CAPACITY, 0, GL_STREAM_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * STRIDE, (void *) 0);						// Vertex
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * STRIDE, (void *) (sizeof(GLfloat) * 3));	// Texcoord

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void spritebatch_destroy(struct spritebatch* batch)
{
	glDeleteBuffers(1, &batch->vbo);
	glDeleteVertexArrays(1, &batch->vao);
}

void spritebatch_begin(struct spritebatch* batch)
{
	batch->sprite_count = 0;

	// Cycle buffer
	if (batch->offset_stream + SPRITEBATCH_CHUNK >= SPRITEBATCH_BUFFER_CAPACITY)
	{
		glBindVertexArray(batch->vao);

		glBindBuffer(GL_ARRAY_BUFFER, batch->vbo);
		glBufferData(GL_ARRAY_BUFFER, SPRITEBATCH_BUFFER_CAPACITY, 0, GL_STREAM_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * STRIDE, (void *) 0);						// Vertex
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * STRIDE, (void *) (sizeof(GLfloat) * 3));	// Texcoord

		glBindVertexArray(0);

		batch->offset_stream = 0;
	}

	// Get GPU memory pointer
	glBindBuffer(GL_ARRAY_BUFFER, batch->vbo);
	batch->gpu_vertices = (GLfloat*)glMapBufferRange(GL_ARRAY_BUFFER, batch->offset_stream, SPRITEBATCH_CHUNK, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);

	if (batch->gpu_vertices == 0)
	{
		printf("glMapBufferRange failed\n");
		exit(0);
	}

	batch->offset_draw = batch->offset_stream / SPRITEBATCH_VERTEX_SIZE;
	batch->offset_stream += SPRITEBATCH_CHUNK;
}

void spritebatch_end(struct spritebatch* batch)
{
	glUnmapBuffer(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void spritebatch_add(struct spritebatch* batch, vec3 pos, vec2 scale, vec2 tex_pos, vec2 tex_bounds)
{
	// Top-left
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 0 + 0] = -0.5f * scale[0] + pos[0];
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 0 + 1] = 0.5f  * scale[1] + pos[1];
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 0 + 2] = 0.0f + pos[2];
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 0 + 3] = tex_pos[0];
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 0 + 4] = tex_pos[1];

	// Bottom-left
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 1 + 0] = -0.5f * scale[0] + pos[0];
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 1 + 1] = -0.5f * scale[1] + pos[1];
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 1 + 2] = 0.0f + pos[2];
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 1 + 3] = tex_pos[0];
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 1 + 4] = tex_pos[1] + tex_bounds[1];

	// Top-right
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 2 + 0] = 0.5f * scale[0] + pos[0];
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 2 + 1] = 0.5f * scale[1] + pos[1];
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 2 + 2] = 0.0f + pos[2];
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 2 + 3] = tex_pos[0] + tex_bounds[0];
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 2 + 4] = tex_pos[1];

	// Top-right
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 3 + 0] = 0.5f * scale[0] + pos[0];
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 3 + 1] = 0.5f * scale[1] + pos[1];
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 3 + 2] = 0.0f + pos[2];
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 3 + 3] = tex_pos[0] + tex_bounds[0];
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 3 + 4] = tex_pos[1];

	// Bottom-left 
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 4 + 0] = -0.5f * scale[0] + pos[0];
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 4 + 1] = -0.5f * scale[1] + pos[1];
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 4 + 2] = 0.0f + pos[2];
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 4 + 3] = tex_pos[0];
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 4 + 4] = tex_pos[1] + tex_bounds[1];

	// Bottom-right
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 5 + 0] = 0.5f  * scale[0] + pos[0];
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 5 + 1] = -0.5f * scale[1] + pos[1];
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 5 + 2] = 0.0f + pos[2];
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 5 + 3] = tex_pos[0] + tex_bounds[0];
	batch->gpu_vertices[CURRENT_SPRITE + STRIDE * 5 + 4] = tex_pos[1] + tex_bounds[1];

	batch->sprite_count++;
}

void spritebatch_sort(struct spritebatch* batch, spritebatch_sort_fn sorting_function)
{
	qsort((void*)batch->gpu_vertices, (size_t)batch->sprite_count, sizeof(GLfloat) * 30, sorting_function);
}

void spritebatch_render(struct spritebatch* batch, struct shader *s)
{
	glUseProgram(s->program);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, batch->vbo);
	glBindVertexArray(batch->vao);

	/* Position stream. */
	GLint posAttrib = glGetAttribLocation(s->program, ATTRIB_NAME_POSITION);
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * STRIDE, (void *)0);

	/* Texcoord stream. */
	GLint texcoordAttrib = glGetAttribLocation(s->program, ATTRIB_NAME_TEXCOORD);
	glEnableVertexAttribArray(texcoordAttrib);
	glVertexAttribPointer(texcoordAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * STRIDE, (void *)(sizeof(GLfloat) * 3));

	glDrawArrays(GL_TRIANGLES, batch->offset_draw, batch->sprite_count * 6);
}

void spritebatch_render_simple(struct spritebatch* batch, struct shader *s, GLuint texture, mat4 projection, mat4 transform)
{
	glUseProgram(s->program);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	GLint uniform_projection = glGetUniformLocation(s->program, "projection");
	GLint uniform_transform = glGetUniformLocation(s->program, "transform");
	GLint uniform_tex = glGetUniformLocation(s->program, "tex");
	GLint uniform_color = glGetUniformLocation(s->program, "color");

	glUniformMatrix4fv(uniform_transform, 1, GL_FALSE, transform);
	glUniformMatrix4fv(uniform_projection, 1, GL_FALSE, projection);
	glUniform1i(uniform_tex, 0);
	glUniform4fv(uniform_color, 1, COLOR_WHITE);

	spritebatch_render(batch, s);
}
