#include "mc.h"

/*============================================================================================================
 *
 *
 * 
 *==========================================================================================================*/

static inline MC_BOOL is_idx_in_render_distance (int ix, int iy, int iz) {
    if (ix < 0) return MC_FALSE;
    if (iy < 0) return MC_FALSE;
    if (iz < 0) return MC_FALSE;
    if (ix >= MC_RENDER_DISTANCE) return MC_FALSE;
    if (iy >= MC_WORLD_HEIGHT   ) return MC_FALSE;
    if (iz >= MC_RENDER_DISTANCE) return MC_FALSE;
    return MC_TRUE;
}

static inline MC_BOOL coord_to_idx (struct mc_World * wd, int x, int y, int z, int * ixPtr, int * iyPtr, int * izPtr) {
    assert(wd != NULL);
    assert(ixPtr != NULL);
    assert(iyPtr != NULL);
    assert(izPtr != NULL);
    *ixPtr = abs(x) % MC_RENDER_DISTANCE;
    *iyPtr = abs(y) % MC_WORLD_HEIGHT;
    *izPtr = abs(z) % MC_RENDER_DISTANCE;
    if (x < 0) *ixPtr = MC_RENDER_DISTANCE - *ixPtr;
    if (y < 0) *iyPtr = MC_WORLD_HEIGHT    - *iyPtr;
    if (z < 0) *izPtr = MC_RENDER_DISTANCE - *izPtr;
    return is_idx_in_render_distance(
        x - wd->offset[0],
        y - wd->offset[1],
        z - wd->offset[2]
    );
}

static inline struct mc_Block * block_at_idx (struct mc_World * wd, int ix, int iy, int iz) {
    return &wd->blocks[
        (ix * MC_RENDER_DISTANCE * MC_WORLD_HEIGHT) +
        (iy * MC_RENDER_DISTANCE) +
        (iz)
    ];
}

static GLintptr next_face_index (struct mc_World * wd) {
    assert(wd != NULL);
    if (wd->free_face_indices_top > 0)
        return wd->free_face_indices[--wd->free_face_indices_top];
    return wd->face_indices_top++;
}

static void free_face_index (struct mc_World * wd, GLintptr face_idx) {
    assert(wd != NULL);
    assert(wd->free_face_indices_top < MC_WORLD_MAX_FACES);
    wd->free_face_indices[wd->free_face_indices_top++] = face_idx;
}

static void send_block (struct mc_World * wd, struct mc_Block * block, float x_, float y_, float z_, float a_left, float a_right, float a_top, float a_bottom, float a_front, float a_back) {
    assert(wd != NULL);
    assert(block != NULL);

    float x = (float)x_ * MC_BLOCK_SIZE;
    float y = (float)y_ * MC_BLOCK_SIZE;
    float z = (float)z_ * MC_BLOCK_SIZE;
    float x2 = x + MC_BLOCK_SIZE;
	float y2 = y + MC_BLOCK_SIZE;
	float z2 = z + MC_BLOCK_SIZE;

    static float face[][6] = { // front, back, top, bottom, left, right
        {0, 0, 1, 2, 0, 0} // grass
    };
    size_t i = (size_t)block->type;
    float t = 1.0f / (float)MC_BLOCKTEX_BLOCKS;
    float tex_front = (MC_BLOCKTEX_BLOCKS - 1 - face[i][0]) * t;
    float tex_back  = (MC_BLOCKTEX_BLOCKS - 1 - face[i][1]) * t;
    float tex_top   = (MC_BLOCKTEX_BLOCKS - 1 - face[i][2]) * t;
    float tex_bot   = (MC_BLOCKTEX_BLOCKS - 1 - face[i][3]) * t;
    float tex_left  = (MC_BLOCKTEX_BLOCKS - 1 - face[i][4]) * t;
    float tex_right = (MC_BLOCKTEX_BLOCKS - 1 - face[i][5]) * t;

    glBindBuffer(GL_ARRAY_BUFFER, wd->VBO);

    if (block->face_idx_back != 0) { // -Z
        float vertices[] = {
            x,  y,  z, /**/ 0,(tex_back+0), /**/ a_back,
            x2, y2, z, /**/ 1,(tex_back+t), /**/ a_back,
            x2, y,  z, /**/ 1,(tex_back+0), /**/ a_back,
            x2, y2, z, /**/ 1,(tex_back+t), /**/ a_back,
            x,  y,  z, /**/ 0,(tex_back+0), /**/ a_back,
            x,  y2, z, /**/ 0,(tex_back+t), /**/ a_back
        };
        glBufferSubData(GL_ARRAY_BUFFER, block->face_idx_back * MC_BLOCK_FACE_VERTICES * sizeof(struct mc_BlockVertex), sizeof(vertices), vertices);
    }
    if (block->face_idx_front != 0) { // +Z
        float vertices[] = {
            x,  y,  z2, /**/ 0,(tex_front+0), /**/ a_front,
            x2, y,  z2, /**/ 1,(tex_front+0), /**/ a_front,
            x2, y2, z2, /**/ 1,(tex_front+t), /**/ a_front,
            x2, y2, z2, /**/ 1,(tex_front+t), /**/ a_front,
            x,  y2, z2, /**/ 0,(tex_front+t), /**/ a_front,
            x,  y,  z2, /**/ 0,(tex_front+0), /**/ a_front
        };
        glBufferSubData(GL_ARRAY_BUFFER, block->face_idx_front * MC_BLOCK_FACE_VERTICES * sizeof(struct mc_BlockVertex), sizeof(vertices), vertices);
    }
    if (block->face_idx_left != 0) { // -X
        float vertices[] = {
            x, y2, z2, /**/ 1,(tex_left+t), /**/ a_left,
            x, y2, z,  /**/ 0,(tex_left+t), /**/ a_left,
            x, y,  z,  /**/ 0,(tex_left+0), /**/ a_left,
            x, y,  z,  /**/ 0,(tex_left+0), /**/ a_left,
            x, y,  z2, /**/ 1,(tex_left+0), /**/ a_left,
            x, y2, z2, /**/ 1,(tex_left+t), /**/ a_left
        };
        glBufferSubData(GL_ARRAY_BUFFER, block->face_idx_left * MC_BLOCK_FACE_VERTICES * sizeof(struct mc_BlockVertex), sizeof(vertices), vertices);
    }
    if (block->face_idx_right != 0) { // +X
        float vertices[] = {
            x2, y2, z2, /**/ 1,(tex_right+t), /**/ a_right,
            x2, y,  z,  /**/ 0,(tex_right+0), /**/ a_right,
            x2, y2, z,  /**/ 0,(tex_right+t), /**/ a_right,
            x2, y,  z,  /**/ 0,(tex_right+0), /**/ a_right,
            x2, y2, z2, /**/ 1,(tex_right+t), /**/ a_right,
            x2, y,  z2, /**/ 1,(tex_right+0), /**/ a_right
        };
        glBufferSubData(GL_ARRAY_BUFFER, block->face_idx_right * MC_BLOCK_FACE_VERTICES * sizeof(struct mc_BlockVertex), sizeof(vertices), vertices);
    }
    if (block->face_idx_bottom != 0) { // -Y
        float vertices[] = {
            x,  y, z,  /**/ 0,(tex_bot+t), /**/ a_bottom,
            x2, y, z,  /**/ 1,(tex_bot+t), /**/ a_bottom,
            x2, y, z2, /**/ 1,(tex_bot+0), /**/ a_bottom,
            x2, y, z2, /**/ 1,(tex_bot+0), /**/ a_bottom,
            x,  y, z2, /**/ 0,(tex_bot+0), /**/ a_bottom,
            x,  y, z,  /**/ 0,(tex_bot+t), /**/ a_bottom
        };
        glBufferSubData(GL_ARRAY_BUFFER, block->face_idx_bottom * MC_BLOCK_FACE_VERTICES * sizeof(struct mc_BlockVertex), sizeof(vertices), vertices);
    }
    if (block->face_idx_top != 0) { // +Y
        float vertices[] = {
            x,  y2, z,  /**/ 0,(tex_top+t), /**/ a_top,
            x2, y2, z2, /**/ 1,(tex_top+0), /**/ a_top,
            x2, y2, z,  /**/ 1,(tex_top+t), /**/ a_top,
            x2, y2, z2, /**/ 1,(tex_top+0), /**/ a_top,
            x,  y2, z,  /**/ 0,(tex_top+t), /**/ a_top,
            x,  y2, z2, /**/ 0,(tex_top+0), /**/ a_top
        };
        glBufferSubData(GL_ARRAY_BUFFER, block->face_idx_top * MC_BLOCK_FACE_VERTICES * sizeof(struct mc_BlockVertex), sizeof(vertices), vertices);
    }
}

/*============================================================================================================
 *
 *
 * 
 *==========================================================================================================*/

void mc_world_init (struct mc_World *wd, size_t reserved_blocks_count) {
    assert(wd != NULL);

    wd->offset[0] = 0;
    wd->offset[1] = 0;
    wd->offset[2] = 0;
    wd->fnl = fnlCreateState();
    wd->fnl.noise_type = FNL_NOISE_PERLIN;
    wd->blocks = malloc(sizeof(*wd->blocks) * MC_WORLD_MAX_BLOCKS);
    
    wd->face_indices_top = reserved_blocks_count;
    wd->free_face_indices_top = 0;
    wd->free_face_indices = malloc(sizeof(*wd->free_face_indices) * MC_WORLD_MAX_FACES);

    for (int ix = 0; ix < MC_RENDER_DISTANCE; ix++)
    for (int iy = 0; iy < MC_WORLD_HEIGHT;    iy++)
    for (int iz = 0; iz < MC_RENDER_DISTANCE; iz++) {
        struct mc_Block * block = block_at_idx(wd, ix, iy, iz);
        block->exists = MC_FALSE;
        block->face_idx_left = 0;
        block->face_idx_right = 0;
        block->face_idx_top = 0;
        block->face_idx_bottom = 0;
        block->face_idx_front = 0;
        block->face_idx_back = 0;
        block->type = MC_BLOCK_TYPE_GRASS;
    }

	glGenVertexArrays(1, &wd->VAO);
	glBindVertexArray(wd->VAO);

	glGenBuffers(1, &wd->VBO);
	glBindBuffer(GL_ARRAY_BUFFER, wd->VBO);
	glBufferData(GL_ARRAY_BUFFER, MC_WORLD_MAX_VERTICES * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(struct mc_BlockVertex), offsetof(struct mc_BlockVertex, x));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(struct mc_BlockVertex), offsetof(struct mc_BlockVertex, tx));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(struct mc_BlockVertex), offsetof(struct mc_BlockVertex, a));
}

void mc_world_free (struct mc_World *wd) {
    assert(wd != NULL);

    free(wd->blocks);
    free(wd->free_face_indices);

	glDeleteVertexArrays(1, &wd->VAO);
	glDeleteBuffers(1, &wd->VBO);
}

/*============================================================================================================
 *
 *
 * 
 *==========================================================================================================*/

void mc_world_draw (struct mc_World * wd, GLint blocks_index, GLsizei blocks_count) {
    assert(wd != NULL);
    assert(blocks_count > 0);
	glBindVertexArray(wd->VAO);
	glDrawArrays(GL_TRIANGLES, blocks_index * MC_BLOCK_VERTICES, blocks_count * MC_BLOCK_VERTICES);
}

struct mc_Block * mc_world_block_at (struct mc_World * wd, int x, int y, int z) {
    assert(wd != NULL);
    int ix, iy, iz;
    if (!coord_to_idx(wd, x, y, z, &ix, &iy, &iz))
        return NULL;
    struct mc_Block * block = block_at_idx(wd, ix, iy, iz);
    if (!block->exists)
        return NULL;
    return block;
}

void mc_world_place_block_at (struct mc_World * wd, int x, int y, int z, enum mc_BlockType type) {
    assert(wd != NULL);
    int ix, iy, iz;
    if (coord_to_idx(wd, x, y, z, &ix, &iy, &iz))
        mc_world_place_block_at_idx(wd, ix, iy, iz, x, y, z, type);
}

void mc_world_destroy_block_at (struct mc_World * wd, int x, int y, int z) {
    assert(wd != NULL);
    int ix, iy, iz;
    if (coord_to_idx(wd, x, y, z, &ix, &iy, &iz))
        mc_world_destroy_block_at_idx(wd, ix, iy, iz, x, y, z);
}

void mc_world_move (struct mc_World * wd, int dx, int dy, int dz) {
    assert(wd != NULL);
    assert((dx != 0) || (dy != 0) || (dz != 0));

    int ox = wd->offset[0];
    int oy = wd->offset[1];
    int oz = wd->offset[2];
    wd->offset[0]++;

    for (int iz = 0; iz < MC_RENDER_DISTANCE; iz++)
    for (int iy = 0; iy < MC_WORLD_HEIGHT; iy++) {
        int y = iy + oy;
        int z = iz + oz;
        int ix = ox % MC_RENDER_DISTANCE;
        if (block_at_idx(wd, ix, iy, iz)->exists) {
            // mc_save_block(wd, ox, y, z, MC_BLOCK_TYPE_GRASS);
            mc_world_destroy_block_at_idx(wd,
                ix, iy, iz,
                ox, y, z
            );
        }
    }

    for (int iz = 0; iz < MC_RENDER_DISTANCE; iz++)
    for (int iy = 0; iy < MC_WORLD_HEIGHT; iy++) {
        int y = iy + oy;
        int z = iz + oz;
        int ix = ox % MC_RENDER_DISTANCE;
        int x = MC_RENDER_DISTANCE + ox;

        // enum mc_BlockType type = mc_load_block(wd, x, y, z);
        // if (type == MC_BLOCK_TYPE_AIR)
        //     continue;
        // if (type == MC_BLOCK_TYPE_NONE) {
            float noise = fnlGetNoise3D(&wd->fnl, x, y, z);
            if (noise > 0) {
                mc_world_place_block_at_idx(wd,
                    ix, iy, iz,
                    x, y, z, MC_BLOCK_TYPE_GRASS
                );
            }
        // }
        // else {
        //     mc_world_place_block_at_idx(wd,
        //         ix, iy, iz,
        //         x, y, z, type
        //     );
        // }
    }
}

/*============================================================================================================
 *
 * 
 * 
 *==========================================================================================================*/

void mc_world_place_block_at_idx (struct mc_World * wd, int ix, int iy, int iz, int x, int y, int z, enum mc_BlockType type) {
    assert(wd != NULL);
    assert(is_idx_in_render_distance(ix, iy, iz));

    struct mc_Block * block = block_at_idx(wd, ix, iy, iz);
    assert(!block->exists);
    block->type = type;
    block->exists = MC_TRUE;

    struct mc_Block * block_left  = mc_world_block_at(wd, x - 1, y, z);
    struct mc_Block * block_right = mc_world_block_at(wd, x + 1, y, z);
    struct mc_Block * block_bot   = mc_world_block_at(wd, x, y - 1, z);
    struct mc_Block * block_top   = mc_world_block_at(wd, x, y + 1, z);
    struct mc_Block * block_back  = mc_world_block_at(wd, x, y, z - 1);
    struct mc_Block * block_front = mc_world_block_at(wd, x, y, z + 1);

    if (block_left != NULL)
    if (block_left->face_idx_right != 0) {
        free_face_index(wd, block_left->face_idx_right);
        block_left->face_idx_right = 0;
    }
    if (block_right != NULL)
    if (block_right->face_idx_left != 0) {
        free_face_index(wd, block_right->face_idx_left);
        block_right->face_idx_left = 0;
    }
    if (block_bot != NULL)
    if (block_bot->face_idx_top != 0) {
        free_face_index(wd, block_bot->face_idx_top);
        block_bot->face_idx_top = 0;
    }
    if (block_top != NULL)
    if (block_top->face_idx_bottom != 0) {
        free_face_index(wd, block_top->face_idx_bottom);
        block_top->face_idx_bottom = 0;
    }
    if (block_back != NULL)
    if (block_back->face_idx_front != 0) {
        free_face_index(wd, block_back->face_idx_front);
        block_back->face_idx_front = 0;
    }
    if (block_front != NULL)
    if (block_front->face_idx_back != 0) {
        free_face_index(wd, block_front->face_idx_back);
        block_front->face_idx_back = 0;
    }

    if (block_left  == NULL) block->face_idx_left   = next_face_index(wd);
    if (block_right == NULL) block->face_idx_right  = next_face_index(wd);
    if (block_bot   == NULL) block->face_idx_bottom = next_face_index(wd);
    if (block_top   == NULL) block->face_idx_top    = next_face_index(wd);
    if (block_back  == NULL) block->face_idx_back   = next_face_index(wd);
    if (block_front == NULL) block->face_idx_front  = next_face_index(wd);

    send_block(wd, block, x, y, z, 1,1,1,1,1,1);
}

void mc_world_destroy_block_at_idx (struct mc_World *wd, int ix, int iy, int iz, int x, int y, int z) {
    assert(wd != NULL);
    assert(is_idx_in_render_distance(ix, iy, iz));

    struct mc_Block *block = block_at_idx(wd, ix, iy, iz);
    assert(block->exists);
    block->exists = MC_FALSE;

    send_block(wd, block, 0, 0, 0, 0,0,0,0,0,0);
    if (block->face_idx_back   != 0) free_face_index(wd, block->face_idx_back);
    if (block->face_idx_front  != 0) free_face_index(wd, block->face_idx_front);
    if (block->face_idx_left   != 0) free_face_index(wd, block->face_idx_left);
    if (block->face_idx_right  != 0) free_face_index(wd, block->face_idx_right);
    if (block->face_idx_top    != 0) free_face_index(wd, block->face_idx_top);
    if (block->face_idx_bottom != 0) free_face_index(wd, block->face_idx_bottom);
    block->face_idx_back   = 0;
    block->face_idx_front  = 0;
    block->face_idx_left   = 0;
    block->face_idx_right  = 0;
    block->face_idx_top    = 0;
    block->face_idx_bottom = 0;

    struct mc_Block * block_left  = mc_world_block_at(wd, x - 1, y, z);
    struct mc_Block * block_right = mc_world_block_at(wd, x + 1, y, z);
    struct mc_Block * block_bot   = mc_world_block_at(wd, x, y - 1, z);
    struct mc_Block * block_top   = mc_world_block_at(wd, x, y + 1, z);
    struct mc_Block * block_back  = mc_world_block_at(wd, x, y, z - 1);
    struct mc_Block * block_front = mc_world_block_at(wd, x, y, z + 1);

    if (block_left != NULL) {
        block_left->face_idx_right = next_face_index(wd);
        send_block(wd, block_left, x - 1, y, z, 1,1,1,1,1,1);
    }
    if (block_right != NULL) {
        block_right->face_idx_left = next_face_index(wd);
        send_block(wd, block_right, x + 1, y, z, 1,1,1,1,1,1);
    }
    if (block_bot != NULL) {
        block_bot->face_idx_top = next_face_index(wd);
        send_block(wd, block_bot, x, y - 1, z, 1,1,1,1,1,1);
    }
    if (block_top != NULL) {
        block_top->face_idx_bottom = next_face_index(wd);
        send_block(wd, block_top, x, y + 1, z, 1,1,1,1,1,1);
    }
    if (block_back != NULL) {
        block_back->face_idx_front = next_face_index(wd);
        send_block(wd, block_back, x, y, z - 1, 1,1,1,1,1,1);
    }
    if (block_front != NULL) {
        block_front->face_idx_back = next_face_index(wd);
        send_block(wd, block_front, x, y, z + 1, 1,1,1,1,1,1);
    }
}