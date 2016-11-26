/*
	This file is part of Iceball.

	Iceball is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Iceball is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Iceball.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "common.h"

#include <stdbool.h>

// the max number of quads in our buffer
#define DRAW_MAX_QUADS 1024
#define QUAD_VERTEX_BUFFER_CAPACITY (DRAW_MAX_QUADS * 4)
#define QUAD_INDEX_BUFFER_CAPACITY (DRAW_MAX_QUADS * 6)

// buffered draw call count
#define QUAD_MAX_DRAW_CALLS 64

static const char *VSH =
		"#version 120\n"

		"attribute vec4 a_posuv;\n"
		"attribute vec4 a_color;\n"

		"varying vec4 v_color;\n"
		"varying vec2 v_uv;\n"

		"uniform mat4 u_mvp;\n"

		"void main() {\n"
		"   v_color = a_color;\n"
		"   v_uv = a_posuv.zw;\n"
		"   gl_Position = u_mvp * vec4(a_posuv.xy, 0.0, 1.0);\n"
		"}\n";

static const char *FSH =
		"#version 120\n"

		"uniform sampler2D sampler;\n"

		"varying vec4 v_color;\n"
		"varying vec2 v_uv;\n"

		"void main() {\n"
		"   gl_FragColor = texture2D(sampler, v_uv) * v_color;\n"
		"}\n";

typedef struct draw_call {
	GLuint page;
	GLuint len;
} draw_call_t;

typedef struct draw_vertex {
	float x, y, s, t;
	uint32_t color;
} draw_vertex_t;

static bool drawing = false;

static draw_vertex_t *quad_vertex_mem;
static GLuint data_len = 0;

static GLuint quad_indices = 0;
static GLuint quad_vertex_buffer = 0;

static draw_call_t draw_calls[QUAD_MAX_DRAW_CALLS] = {{0}};
static GLuint draw_count = 0;

static GLuint blank_texture;
static GLuint draw_prog;

static GLint attr_posuv;
static GLint attr_color;
static GLint uniform_sampler;
static GLint uniform_mvp;
static GLuint current_texture;

// TODO: replace
static GLfloat mvp[4][4] = {{0}};

static void _ortho(float left, float right, float top, float bottom, float near, float far, GLfloat out[4][4])
{
	out[0][0] = 2.f / (right - left);
	out[0][1] = 0.f;
	out[0][2] = 0.f;
	out[0][3] = 0.f;

	out[1][0] = 0.f;
	out[1][1] = 2.f / (top - bottom);
	out[1][2] = 0.f;
	out[1][3] = 0.f;

	out[2][0] = 0.f;
	out[2][1] = 0.f;
	out[2][2] = -2.f / (far - near);
	out[2][3] = 0.f;

	out[3][0] = -(right + left) / (right - left);
	out[3][1] = -(top + bottom) / (top - bottom);
	out[3][2] = -(far + near) / (far - near);
	out[3][3] = 1.f;
}

// TODO: more error checking
static void init_buffers()
{
	quad_vertex_mem = malloc(sizeof(draw_vertex_t) * QUAD_VERTEX_BUFFER_CAPACITY);

	glGenBuffers(1, &quad_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vertex_buffer);
	glBufferData(
			GL_ARRAY_BUFFER,
			sizeof(*quad_vertex_mem) * QUAD_VERTEX_BUFFER_CAPACITY,
			NULL,
			GL_STREAM_DRAW);

	uint16_t *indices = malloc(sizeof(uint16_t) * QUAD_INDEX_BUFFER_CAPACITY);

	int i = 0, index = 0;
	while (i + 6 < QUAD_INDEX_BUFFER_CAPACITY) {
		indices[i + 0] = index + 0;
		indices[i + 1] = index + 1;
		indices[i + 2] = index + 2;

		indices[i + 3] = index + 0;
		indices[i + 4] = index + 2;
		indices[i + 5] = index + 3;

		i += 6;
		index += 4;
	}

	glGenBuffers(1, &quad_indices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_indices);
	glBufferData(
			GL_ELEMENT_ARRAY_BUFFER,
			sizeof(uint16_t) * QUAD_INDEX_BUFFER_CAPACITY,
			indices,
			GL_STATIC_DRAW);

	free(indices);

	draw_prog = glCreateProgram();
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &VSH, NULL);
	glCompileShader(vs);
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &FSH, NULL);
	glCompileShader(fs);
	glAttachShader(draw_prog, vs);
	glAttachShader(draw_prog, fs);
	glLinkProgram(draw_prog);
	glUseProgram(draw_prog);

	attr_posuv = glGetAttribLocation(draw_prog, "a_posuv");
	attr_color = glGetAttribLocation(draw_prog, "a_color");

	uniform_sampler = glGetUniformLocation(draw_prog, "sampler");
	uniform_mvp = glGetUniformLocation(draw_prog, "u_mvp");
	glUniform1i(uniform_sampler, 0);

	_ortho(0, screen_width, 0, screen_height, -1, 1, mvp);

	glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, mvp);

	uint32_t pixel = 0xffffffff;
	glGenTextures(1, &blank_texture);
	glBindTexture(GL_TEXTURE_2D, blank_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_BGRA, GL_UNSIGNED_BYTE, &pixel);
}

void draw_blank_quad(float x, float y, float width, float height,
	uint32_t color)
{
	draw_quad(x, y, width, height, 0.0, 0.0, 1.0, 1.0, blank_texture, color);
}

void draw_quad(float x, float y, float width, float height, float s, float t,
	float u, float v, GLuint texture, uint32_t color)
{
	if (!quad_vertex_buffer) {
		init_buffers();
	}

	if (current_texture == 0)
		current_texture = texture;

	if (current_texture != texture) {
		if (draw_count + 1 > QUAD_MAX_DRAW_CALLS) {
			draw_flush();
			current_texture = texture;
		}

		draw_calls[draw_count].page = current_texture;
		draw_count++;

		current_texture = texture;
	}

	drawing = true;

	// top left
	draw_vertex_t tl = { x,         y,          s, t, color };

	// bottom left
	draw_vertex_t bl = { x,         y + height, s, v, color };

	// bottom right
	draw_vertex_t br = { x + width, y + height, u, v, color };

	// top right
	draw_vertex_t tr = { x + width, y,          u, t, color };

	quad_vertex_mem[data_len++] = tl;
	quad_vertex_mem[data_len++] = bl;
	quad_vertex_mem[data_len++] = br;
	quad_vertex_mem[data_len++] = tr;

	draw_calls[draw_count].len += 6;
}

void draw_flush()
{
	if (data_len == 0)
		return;

	// TODO: better way to flush or demand explicit?
	//
	// finalize in progress draws
	if (drawing && draw_calls[draw_count].len > 0) {
		draw_calls[draw_count].page = current_texture;
		draw_count++;
	}

	glUseProgram(draw_prog);

	glBindBuffer(GL_ARRAY_BUFFER, quad_vertex_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(*quad_vertex_mem) * data_len, quad_vertex_mem);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_indices);

	glEnableVertexAttribArray(attr_posuv);
	glEnableVertexAttribArray(attr_color);

	glVertexAttribPointer(attr_posuv, 4, GL_FLOAT, GL_FALSE, sizeof(draw_vertex_t), 0);
	glVertexAttribPointer(attr_color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(draw_vertex_t), 4 * sizeof(GLfloat));

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	//glEnable(GL_ALPHA_TEST);

	// TODO: do we have side-effects here?
	//glAlphaFunc(GL_GREATER, 0);
	//glDepthFunc(GL_ALWAYS);

	size_t offset = 0;
	for (int i = 0; i < draw_count; ++i) {
		draw_call_t draw_call = draw_calls[i];
		//printf("(%d/%d): %d [%d]\n", i, draw_count, draw_call.len, draw_call.page);

		glBindTexture(GL_TEXTURE_2D, draw_call.page);
		glDrawElements(GL_TRIANGLES, draw_call.len, GL_UNSIGNED_SHORT, (const void *) (offset * sizeof(GLushort)));

		offset += draw_call.len;
	}

	glDisableVertexAttribArray(attr_posuv);
	glDisableVertexAttribArray(attr_color);

	glUseProgram(0);

	current_texture = 0;
	drawing = false;
	draw_count = 0;
	data_len = 0;
	memset(draw_calls, 0, sizeof(draw_calls));
}
