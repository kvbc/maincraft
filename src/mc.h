#ifndef MC_H
#define MC_H

#include <glad/glad.h>
#include <cglm/cglm.h>
#include <stb_image.h>

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#include "config.h"

#define MC_BOOL  uint8_t
#define MC_TRUE  1
#define MC_FALSE 0

#define MC_PINFO(frmt,...) ( printf("[%s:%u] Info : "  frmt "\n", __FILE__, __LINE__ __VA_OPT__(,) __VA_ARGS__) )
#define MC_PERR(frmt,...)  ( printf("[%s:%u] Error : " frmt, __FILE__, __LINE__ __VA_OPT__(,) __VA_ARGS__) )
#define MC_PGLERR()        ( printf("[%s:%u] OpenGL Error : (%u)\n", __FILE__, __LINE__, glGetError()) )
#define MC_GLANYERR()      ( glGetError() != GL_NO_ERROR )

#define MC_MAX(a,b) ( ((a) > (b)) ? (a) : (b) )

#define MC_BLOCK_VERTICES (6 * 6)
#define MC_TEXT_VERTICES  (6)

#define MC_WORLD_VERTICES (MC_WORLD_DATA / sizeof(struct mc_BlockVertex))

enum mc_Status {
	MC_BAD = 0,
	MC_OK
};

struct mc_CrosshairVertex {
	float x, y;
	float tx, ty; // texture coords
};

struct mc_BlockVertex {
	float x, y, z;
	float tx, ty; // texture coords
	float a; // alpha
    float type;
};

enum mc_BlockType {
    MC_BLOCK_TYPE_GRASS = 0
};

struct mc_Block {
	size_t vertofs;
	ivec3 pos;
	ivec3 sz;
    enum mc_BlockType type;
};

enum mc_Status mc_file_read (const char* fn, char** buffPtr);

GLuint mc_program_create (const char *name, const char *vertPath, const char *fragPath);
void mc_program_delete (GLuint ID);
void mc_program_set_int  (GLuint ID, const char *name, GLint x);

struct mc_Camera {
	vec3 pos;
	vec3 front, right;
	float yaw, pitch;
};

void mc_camera_init 	  (struct mc_Camera *cam);
void mc_camera_mousemov   (struct mc_Camera *cam, float ofsx, float ofsy);
void mc_camera_viewmatrix (struct mc_Camera *cam, mat4 dest);

struct mc_World {
	GLuint VAO, VBO, EBO;
	size_t blockvc; // vertices count
	struct mc_Block blocks[MC_WORLD_CPU_MAX_BLOCKS];
	size_t blockstop;
};

void mc_world_init (struct mc_World *wd);
void mc_world_free (struct mc_World *wd);
void mc_world_draw (struct mc_World *wd, GLint vertofs, GLsizei vertsz);
void mc_world_set_union  (struct mc_World *wd, GLintptr vertofs, enum mc_BlockType type, int x, int y, int z, int w, int h, int d, float a);
void mc_world_set_block  (struct mc_World *wd, GLintptr vertofs, enum mc_BlockType type, int x, int y, int z, float a);
void mc_world_push_block (struct mc_World *wd, enum mc_BlockType type, int x, int y, int z, float a);
void mc_world_push_union (struct mc_World *wd, enum mc_BlockType type, int x, int y, int z, int w, int h, int d, float a);

struct mc_Texture {
    GLenum intfrmt; // internal format
    GLenum datafrmt; // data format
    stbi_uc *data;
	int channels;
    int width;
	int height;
	GLuint id;
};
enum mc_Status mc_tex_create (struct mc_Texture *tex, const char *fn);
enum mc_Status mc_tex_load (struct mc_Texture *tex, const char *fn);
void mc_tex_unload (struct mc_Texture *tex);

struct mc_TextVertex {
    float x, y;
    float tx, ty; // texture coords
};
struct mc_TextRenderer {
    GLuint VAO, VBO, prog;
    int totallen;
    struct mc_TextVertex *tmpvert;
    size_t tmpvertcap;
    struct mc_Texture font;
};
enum mc_Status mc_textr_create (struct mc_TextRenderer *text);
void mc_textr_push (struct mc_TextRenderer *text, float x, float y, const char *src);
void mc_textr_draw (struct mc_TextRenderer *text);
void mc_textr_destroy (struct mc_TextRenderer *text);

void mc_vec3_set  (vec3 v, float x, float y, float z);
void mc_vec4_set  (vec4 v, float x, float y, float z, float w);
void mc_ivec3_set (ivec3 v, int x, int y, int z);
void mc_ivec4_set (ivec4 v, int x, int y, int z, int w);

#endif // MC_H