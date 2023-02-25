#ifndef MC_H
#define MC_H

#include <glad/glad.h>
#include <cglm/cglm.h>
#include <stb_image.h>
#include <FastNoiseLite.h>

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
#define MC_MIN(a,b) ( ((a) < (b)) ? (a) : (b) )

enum mc_Status {
	MC_BAD = 0,
	MC_OK
};

struct mc_CrosshairVertex {
	float x, y;
	float tx, ty; // texture coords
};

/*
 *
 * File
 * 
 */

enum mc_Status mc_file_read (const char* fn, char** buffPtr);

/*
 *
 * Program
 * 
 */

GLuint mc_program_create (const char *name, const char *vertPath, const char *fragPath);
void mc_program_delete (GLuint ID);
void mc_program_set_int  (GLuint ID, const char *name, GLint x);

/*
 *
 * Camera
 * 
 */

struct mc_Camera {
	vec3 pos;
	vec3 front, right;
	float yaw, pitch;
};

void mc_camera_init 	  (struct mc_Camera *cam);
void mc_camera_mousemov   (struct mc_Camera *cam, float ofsx, float ofsy);
void mc_camera_viewmatrix (struct mc_Camera *cam, mat4 dest);

/*
 *
 * Block
 * World
 * 
 */

#define MC_BLOCK_FACES           (6)
#define MC_BLOCK_FACE_VERTICES   (6)
#define MC_BLOCK_VERTICES        (MC_BLOCK_FACES * MC_BLOCK_FACE_VERTICES)
#define MC_BLOCK_VERTEX_ELEMENTS (sizeof(struct mc_BlockVertex) / sizeof(float))

struct mc_BlockVertex {
	float x, y, z;
    float tx, ty; // texture coords
	float a; // alpha
};

enum mc_BlockType {
    MC_BLOCK_TYPE_NONE,
    MC_BLOCK_TYPE_AIR,
    MC_BLOCK_TYPE_GRASS
};

struct mc_Block {
    MC_BOOL exists;
    size_t face_idx_left;
    size_t face_idx_right;
    size_t face_idx_top;
    size_t face_idx_bottom;
    size_t face_idx_front;
    size_t face_idx_back;
    enum mc_BlockType type;
};

int mc_block_coord (float xyz);

/*
 *
 * World
 * 
 */

#define MC_WORLD_MAX_BLOCKS (MC_RENDER_DISTANCE * MC_RENDER_DISTANCE * MC_WORLD_HEIGHT)
#define MC_WORLD_MAX_FACES (MC_WORLD_MAX_BLOCKS * MC_BLOCK_FACES)
#define MC_WORLD_MAX_VERTICES (MC_WORLD_MAX_BLOCKS * MC_BLOCK_VERTICES)

struct mc_World {
	GLuint VAO, VBO;
    ivec3 offset;
	struct mc_Block * blocks;
    fnl_state fnl;

    GLintptr face_indices_top;
    GLintptr * free_face_indices;
    GLintptr free_face_indices_top;
};

void mc_world_init (struct mc_World * wd, size_t reserved_blocks_count);
void mc_world_free (struct mc_World * wd);
void mc_world_draw (struct mc_World * wd, GLint block_index, GLsizei block_count);

void             mc_world_move                 (struct mc_World * wd, int x, int y, int z);
struct mc_Block* mc_world_block_at             (struct mc_World * wd, int x, int y, int z);
void             mc_world_destroy_block_at     (struct mc_World * wd, int x, int y, int z);
void             mc_world_place_block_at       (struct mc_World * wd, int x, int y, int z, enum mc_BlockType type);

void             mc_world_destroy_block_at_idx (struct mc_World * wd, int ix, int iy, int iz, int x, int y, int z);
void             mc_world_place_block_at_idx   (struct mc_World * wd, int ix, int iy, int iz, int x, int y, int z, enum mc_BlockType type);

/*
 *
 * Texture
 * 
 */

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

/*
 *
 * Text
 * 
 */

#define MC_TEXT_VERTICES          (6)
#define MC_TEXT_VERTEX_ELEMENTS   (sizeof(struct mc_TextVertex) / sizeof(float))
#define MC_TEXT_ATLAS_CHAR_WIDTH  (1. / MC_TEXT_ATLAS_COLS) // 0 .. 1
#define MC_TEXT_ATLAS_CHAR_HEIGHT (1. / MC_TEXT_ATLAS_ROWS) // 0 .. 1
#define MC_TEXT_CHAR_WIDTH        (1. / MC_TEXT_MAX_CHARS)
#define MC_TEXT_CHAR_HEIGHT       (MC_TEXT_ATLAS_CHAR_WIDTH / MC_TEXT_ATLAS_CHAR_HEIGHT * MC_TEXT_CHAR_WIDTH)

struct mc_TextVertex {
    float x, y;
    float tx, ty; // texture coords
};
struct mc_TextRenderer {
    GLuint VAO, VBO, prog;
    struct mc_Texture font;
};

void mc_textr_create  (struct mc_TextRenderer *textr);
void mc_textr_draw    (struct mc_TextRenderer *textr, float x, float y, const char * src);
void mc_textr_destroy (struct mc_TextRenderer *textr);

#endif // MC_H