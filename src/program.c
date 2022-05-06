#include "mc.h"

#include <stdlib.h>
#include <assert.h>

static GLuint create_shader (const char *name, const char *src, GLenum type) {
    assert(name != NULL);
    assert(src != NULL);
    assert((type == GL_VERTEX_SHADER) || (type == GL_FRAGMENT_SHADER));

	GLuint ID = glCreateShader(type);
	if (ID == 0) {
		MC_PERR("Failed to create shader \"%s\"\n", name);
		MC_PGLERR();
		return 0;
	}

	glShaderSource(ID, 1, &src, NULL);
	glCompileShader(ID);

	GLint success;
	glGetShaderiv(ID, GL_COMPILE_STATUS, &success);
	if (success == GL_FALSE) {
		GLint infologlen;
		glGetShaderiv(ID, GL_INFO_LOG_LENGTH, &infologlen);

		GLchar *infolog = malloc(sizeof(GLchar) * infologlen);
		glGetShaderInfoLog(ID, (GLsizei)infologlen, NULL, infolog);
		MC_PERR("Failed to compile shader \"%s\":\n%s\n", name, infolog);
		free(infolog);

		return 0;
	}

	return ID;
}

GLuint mc_program_create (const char *name, const char *vertPath, const char *fragPath) {
    assert(name != NULL);
    assert(vertPath != NULL);
    assert(fragPath != NULL);

	GLuint ID = glCreateProgram();
	if (ID == 0) {
		MC_PERR("Failed to create program \"%s\"\n", name);
		MC_PGLERR();
		return 0;
	}

    char *src;
    if (mc_file_read(vertPath, &src) == MC_BAD)
        return 0;
    GLuint vert = create_shader("VERTEX", src, GL_VERTEX_SHADER);
    free(src);
    if (vert == 0)
        return 0;

    if (mc_file_read(fragPath, &src) == MC_BAD)
        return 0;
    GLuint frag = create_shader("FRAGMENT", src, GL_FRAGMENT_SHADER);
    free(src);
    if (frag == 0)
        return 0;

	glAttachShader(ID, vert);
	glAttachShader(ID, frag);

	glLinkProgram(ID);
	GLint success;
	glGetProgramiv(ID, GL_LINK_STATUS, &success);
	if (success == GL_FALSE) {
		GLint infologlen;
		glGetProgramiv(ID, GL_INFO_LOG_LENGTH, &infologlen);

		GLchar *infolog = malloc(sizeof(GLchar) * infologlen);
		glGetProgramInfoLog(ID, (GLsizei)infologlen, NULL, infolog);
		MC_PERR("Failed to link program \"%s\":\n%s\n", name, infolog);
		free(infolog);

		return 0;
	}

    glDeleteShader(vert);
    glDeleteShader(frag);

	return ID;
}

inline void mc_program_delete (GLuint ID) {
    assert(ID != 0);
	glDeleteProgram(ID);
}

inline void mc_program_set_int (GLuint ID, const char *name, int x) {
    assert(ID != 0);
    assert(name != NULL);
	glUniform1i(glGetUniformLocation(ID, name), x);
}