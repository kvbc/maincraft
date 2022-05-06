#include "mc.h"

void mc_world_init (struct mc_World *wd) {
    assert(wd != NULL);

	wd->blockvc = 0;
	wd->blockstop = 0;

	glGenVertexArrays(1, &wd->VAO);
	glGenBuffers(1, &wd->VBO);
	glBindVertexArray(wd->VAO);
	glBindBuffer(GL_ARRAY_BUFFER, wd->VBO);
	glBufferData(GL_ARRAY_BUFFER, MC_WORLD_DATA, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(struct mc_BlockVertex), offsetof(struct mc_BlockVertex, x));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(struct mc_BlockVertex), offsetof(struct mc_BlockVertex, tx));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(struct mc_BlockVertex), offsetof(struct mc_BlockVertex, a));
    glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(struct mc_BlockVertex), offsetof(struct mc_BlockVertex, type));
}

void mc_world_free (struct mc_World *wd) {
    assert(wd != NULL);

	glDeleteVertexArrays(1, &wd->VAO);
	glDeleteBuffers(1, &wd->VBO);
}

void mc_world_set_union (struct mc_World *wd, GLintptr vertofs, enum mc_BlockType type, int x_, int y_, int z_, int w, int h, int d, float a) {
    assert(wd != NULL);
    assert(a >= 0.0f);

	float x = x_;
	float y = y_;
	float z = z_;
	x *= MC_BLOCK_SIZE;
	y *= MC_BLOCK_SIZE;
	z *= MC_BLOCK_SIZE;
	float x2 = x + (float)w * MC_BLOCK_SIZE;
	float y2 = y + (float)h * MC_BLOCK_SIZE;
	float z2 = z + (float)d * MC_BLOCK_SIZE;

    static float face[][6] = { // front, back, top, bottom, left, right
        {0, 0, 1, 2, 0, 0} // grass
    };
    size_t t = (size_t)type;
    size_t i = t;
    t = (MC_BLOCKTEX_BLOCKS - 1) - t;
    float frontid = t - face[i][0];
    float backid  = t - face[i][1];
    float topid   = t - face[i][2];
    float botid   = t - face[i][3];
    float leftid  = t - face[i][4];
    float rightid = t - face[i][5];

    float vertices[] = {
		// Back face
		x,  y,  z,  0*w, 0*h, a, backid, // bottom-left
		x2, y2, z,  1*w, 1*h, a, backid, // top-right
		x2, y,  z,  1*w, 0*h, a, backid, // bottom-right         
		x2, y2, z,  1*w, 1*h, a, backid, // top-right
		x,  y,  z,  0*w, 0*h, a, backid, // bottom-left
		x,  y2, z,  0*w, 1*h, a, backid, // top-left
		// Front face
		x,  y,  z2, 0*w, 0*h, a, frontid, // bottom-left
		x2, y,  z2, 1*w, 0*h, a, frontid, // bottom-right
		x2, y2, z2, 1*w, 1*h, a, frontid, // top-right
		x2, y2, z2, 1*w, 1*h, a, frontid, // top-right
		x,  y2, z2, 0*w, 1*h, a, frontid, // top-left
		x,  y,  z2, 0*w, 0*h, a, frontid, // bottom-left
		// Left face
		x,  y2, z2, 1*d, 1*h, a, leftid, // top-right
		x,  y2, z,  0*d, 1*h, a, leftid, // top-left
		x,  y,  z,  0*d, 0*h, a, leftid, // bottom-left
		x,  y,  z,  0*d, 0*h, a, leftid, // bottom-left
		x,  y,  z2, 1*d, 0*h, a, leftid, // bottom-right
		x,  y2, z2, 1*d, 1*h, a, leftid, // top-right
		// Right face
		x2, y2, z2, 1*d, 1*h, a, rightid, // top-left
		x2, y,  z,  0*d, 0*h, a, rightid, // bottom-right
		x2, y2, z,  0*d, 1*h, a, rightid, // top-right         
		x2, y,  z,  0*d, 0*h, a, rightid, // bottom-right
		x2, y2, z2, 1*d, 1*h, a, rightid, // top-left
		x2, y,  z2, 1*d, 0*h, a, rightid, // bottom-left     
		// Bottom face
		x,  y,  z,  0*w, 1*d, a, botid, // top-right
		x2, y,  z,  1*w, 1*d, a, botid, // top-left
		x2, y,  z2, 1*w, 0*d, a, botid, // bottom-left
		x2, y,  z2, 1*w, 0*d, a, botid, // bottom-left
		x,  y,  z2, 0*w, 0*d, a, botid, // bottom-right
		x,  y,  z,  0*w, 1*d, a, botid, // top-right
		// Top face
		x,  y2, z,  0*w, 1*d, a, topid, // top-left
		x2, y2, z2, 1*w, 0*d, a, topid, // bottom-right
		x2, y2, z,  1*w, 1*d, a, topid, // top-right     
		x2, y2, z2, 1*w, 0*d, a, topid, // bottom-right
		x,  y2, z,  0*w, 1*d, a, topid, // top-left
		x,  y2, z2, 0*w, 0*d, a, topid  // bottom-left        
	};

	glBindBuffer(GL_ARRAY_BUFFER, wd->VBO);
	glBufferSubData(GL_ARRAY_BUFFER, vertofs * sizeof(struct mc_BlockVertex), sizeof(vertices), vertices);
}

inline void mc_world_set_block (struct mc_World *wd, GLintptr ofs, enum mc_BlockType type, int x, int y, int z, float a) {
	mc_world_set_union(wd, ofs, type, x, y, z, 1, 1, 1, a);
}

void mc_world_push_union (struct mc_World *wd, enum mc_BlockType type, int x, int y, int z, int w, int h, int d, float a) {
	mc_world_set_union(wd, wd->blockvc, type, x, y, z, w, h, d, a);

    assert(wd->blockstop < MC_WORLD_CPU_MAX_BLOCKS);
	struct mc_Block *block = &wd->blocks[wd->blockstop++];
	block->pos[0] = x;
	block->pos[1] = y;
	block->pos[2] = z;
	block->sz[0] = w;
	block->sz[1] = h;
	block->sz[2] = d;
	block->vertofs = wd->blockvc;
    block->type = type;

	wd->blockvc += MC_BLOCK_VERTICES;
}

inline void mc_world_push_block (struct mc_World *wd, enum mc_BlockType type, int x, int y, int z, float a) {
	mc_world_push_union(wd, type, x, y, z, 1, 1, 1, a);
}

void mc_world_draw (struct mc_World *wd, GLint vertofs, GLsizei vertsz) {
	glBindVertexArray(wd->VAO);
	glDrawArrays(GL_TRIANGLES, vertofs, vertsz);
}