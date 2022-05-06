#include "mc.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>

static void error_callback (int err, const char *desc);
static void key_callback (GLFWwindow *window, int key, int scancode, int action, int mods);
static void cursor_pos_callback (GLFWwindow *window, double xpos, double ypos);

/*
 *
 * Global
 * 
 */

#define MC_DESTROYED_BLOCKS_QUEUE_MAX (5)

static struct {
    struct mc_Camera camera;
    struct mc_World world;
    GLFWwindow *window;

    struct {
        double last_x;
        double last_y;
        MC_BOOL moved;
    } mouse;

    struct {
        GLuint prog;
        struct mc_Texture tex;
        GLuint VAO, VBO;
        mat4 proj;
    } ch;

    struct mc_Texture texfont;

    struct {
        char pos[MC_TEXT_MAX_CHARS];
        char look[MC_TEXT_MAX_CHARS];
        char block[MC_TEXT_MAX_CHARS];
        char blocks_in_reach[MC_TEXT_MAX_CHARS];
        char blocks_in_world[MC_TEXT_MAX_CHARS];
    } txt;

    struct mc_TextRenderer textr;
    
    vec3 rayhitpos;
    vec3 rayprehitpos;
    size_t destroyed_blocks_top;
    struct mc_Block *destroyed_blocks_queue[MC_DESTROYED_BLOCKS_QUEUE_MAX];
    size_t blocks_in_reach_top;
} G = {
    .mouse.last_x = 0.0,
    .mouse.last_y = 0.0,
    .mouse.moved = MC_TRUE,
    .destroyed_blocks_top = 0
};

/*
 *
 * Block
 * 
 */

// real to block coordinates
static inline int blockpos (float xyz) {
	return (int)floorf(xyz / MC_BLOCK_SIZE);
}

// mark the block as destroyed
static inline void block_destroy (struct mc_Block *block) {
	block->sz[0] = -1;
}

static inline MC_BOOL block_isdestroyed (struct mc_Block *block) {
	return block->sz[0] < 0;
}

static inline MC_BOOL block_isunion (struct mc_Block *block) {
	return
	(block->sz[0] > 1) ||
	(block->sz[1] > 1) ||
	(block->sz[2] > 1);
}

static struct mc_Block* hit_block_in_reach (void) {
	G.blocks_in_reach_top = 0;
	static struct mc_Block *blocks_in_reach[MC_REACH * MC_REACH * MC_REACH * 10]; // FIXME

	G.destroyed_blocks_top = 0;
	for (size_t i = 0; i < G.world.blockstop; i++) {
		struct mc_Block *block = &G.world.blocks[i];

		if (block_isdestroyed(block)) {
			if (G.destroyed_blocks_top < MC_DESTROYED_BLOCKS_QUEUE_MAX)
				G.destroyed_blocks_queue[G.destroyed_blocks_top++] = block;
			continue;
		}

		/*
		* Rather than iterate through every block in the world,
		* find out which blocks are within player's reach and later on iterate through those
		*/
		if (blockpos(G.camera.pos[0]) > block->pos[0] - MC_REACH)
		if (blockpos(G.camera.pos[1]) > block->pos[1] - MC_REACH)
		if (blockpos(G.camera.pos[2]) > block->pos[2] - MC_REACH)
		if (blockpos(G.camera.pos[0]) < block->pos[0] + MC_MAX(block->sz[0], MC_REACH) + MC_REACH)
		if (blockpos(G.camera.pos[1]) < block->pos[1] + MC_MAX(block->sz[1], MC_REACH) + MC_REACH)
		if (blockpos(G.camera.pos[2]) < block->pos[2] + MC_MAX(block->sz[2], MC_REACH) + MC_REACH)
			blocks_in_reach[G.blocks_in_reach_top++] = block;
	}

	// Standard unoptimized ray marching
	struct mc_Block *block_hit;
	glm_vec3_copy(G.camera.pos, G.rayhitpos);
	for (size_t r = 0; r < MC_REACH / MC_RAY_PRECISION; r++) {
		block_hit = NULL;
		for (size_t i = 0; i < G.blocks_in_reach_top; i++) {
			struct mc_Block *block = blocks_in_reach[i];
			if (blockpos(G.rayhitpos[0]) >= block->pos[0])
			if (blockpos(G.rayhitpos[1]) >= block->pos[1])
			if (blockpos(G.rayhitpos[2]) >= block->pos[2])
			if (blockpos(G.rayhitpos[0]) <  block->pos[0] + block->sz[0])
			if (blockpos(G.rayhitpos[1]) <  block->pos[1] + block->sz[1])
			if (blockpos(G.rayhitpos[2]) <  block->pos[2] + block->sz[2]) {
				block_hit = block;
				break;
			}
		}
		if (block_hit == NULL) { // step
			G.rayhitpos[0] += G.camera.front[0] * MC_BLOCK_SIZE * MC_RAY_PRECISION;
			G.rayhitpos[1] += G.camera.front[1] * MC_BLOCK_SIZE * MC_RAY_PRECISION;
			G.rayhitpos[2] += G.camera.front[2] * MC_BLOCK_SIZE * MC_RAY_PRECISION;
		} else { // revert last step
			G.rayprehitpos[0] = G.rayhitpos[0] - G.camera.front[0] * MC_BLOCK_SIZE * MC_RAY_PRECISION;
			G.rayprehitpos[1] = G.rayhitpos[1] - G.camera.front[1] * MC_BLOCK_SIZE * MC_RAY_PRECISION;
			G.rayprehitpos[2] = G.rayhitpos[2] - G.camera.front[2] * MC_BLOCK_SIZE * MC_RAY_PRECISION;
			break;
		}
	}

	return block_hit;
}

static void place_union (enum mc_BlockType type, int x, int y, int z, int w, int h, int d) {
	if (G.destroyed_blocks_top == 0) {
		mc_world_push_union(&G.world, type, x, y, z, w, h, d, 1.0f);
	} else {
		struct mc_Block *block = G.destroyed_blocks_queue[--G.destroyed_blocks_top];
		mc_world_set_union(&G.world, block->vertofs, type, x, y, z, w, h, d, 1.0f);
		mc_ivec3_set(block->pos, x, y, z);
		mc_ivec3_set(block->sz, w, h, d);
	}
}

static inline void place_block (enum mc_BlockType type, int x, int y, int z) {
	place_union(type, x, y, z, 1, 1, 1);
}

static void deunion_around_block (struct mc_Block *un, int x, int y, int z) {
	if (x != un->pos[0])          			place_union(un->type, un->pos[0], un->pos[1], un->pos[2], x - un->pos[0], un->sz[1], un->sz[2]);
	if (x != un->pos[0] + un->sz[0] - 1) 	place_union(un->type, x + 1, un->pos[1], un->pos[2], (un->pos[0] + un->sz[0] - 1) - x, un->sz[1], un->sz[2]);
	if (z != un->pos[2])					place_union(un->type, x, un->pos[1], un->pos[2], 1, un->sz[1], z - un->pos[2]);
	if (z != un->pos[2] + un->sz[2] - 1)	place_union(un->type, x, un->pos[1], z + 1, 1, un->sz[1], (un->pos[2] + un->sz[2] - 1) - z);
	if (un->sz[1] > 1)						place_union(un->type, x, un->pos[1], z, 1, un->sz[1] - 1, 1);
}

/*
 *
 * Main
 *
 */

int main (void) {
	glfwSetErrorCallback(error_callback);

	if (glfwInit() == GLFW_FALSE) {
		printf("Failed to initialize GLFW\n");
		return EXIT_FAILURE;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, MC_GLFW_CTX_V_MAJOR);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, MC_GLFW_CTX_V_MINOR);
	glfwWindowHint(GLFW_OPENGL_PROFILE,        MC_GLFW_GL_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	G.window = glfwCreateWindow(MC_WINDOW_WIDTH, MC_WINDOW_HEIGHT, MC_WINDOW_TITLE, NULL, NULL);
	if (G.window == NULL) {
		printf("Failed to create a GLFW window\n");
		glfwTerminate();
		return EXIT_FAILURE;
	}

	glfwGetCursorPos(G.window, &G.mouse.last_x, &G.mouse.last_y);

	glfwMakeContextCurrent(G.window);
	glfwSetKeyCallback(G.window, key_callback);
	glfwSetCursorPosCallback(G.window, cursor_pos_callback);
	glfwSetInputMode(G.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0) {
		printf("Failed to initialize GLAD\n");
		glfwTerminate();
		return EXIT_FAILURE;
	}

	glfwSwapInterval(1);
	// glViewport(0, 0, MC_WINDOW_WIDTH, MC_WINDOW_HEIGHT);

    stbi_set_flip_vertically_on_load(1);

    glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /*
     *
     * Crosshair
     *
     */ 
    {
        G.ch.prog = mc_program_create("CROSSHAIR", "C:/Users/Win10/Desktop/projects/mc/res/shaders/crosshair.vert", "C:/Users/Win10/Desktop/projects/mc/res/shaders/crosshair.frag");
        if (G.ch.prog == 0)
            goto saferet;
        mc_tex_create(&G.ch.tex, "crosshair.png");
        glGenVertexArrays(1, &G.ch.VAO);
        glGenBuffers(1, &G.ch.VBO);
        float wc = G.ch.tex.width  / 2.0f / MC_WINDOW_WIDTH  * MC_CROSSHAIR_SIZE;
        float hc = G.ch.tex.height / 2.0f / MC_WINDOW_HEIGHT * MC_CROSSHAIR_SIZE;
        float chVert[] = {
            0.5f - wc, 0.5f - hc, 0.0f, 0.0f,
            0.5f + wc, 0.5f - hc, 1.0f, 0.0f,
            0.5f - wc, 0.5f + hc, 0.0f, 1.0f,
            0.5f + wc, 0.5f + hc, 1.0f, 1.0f,
            0.5f - wc, 0.5f + hc, 0.0f, 1.0f,
            0.5f + wc, 0.5f - hc, 1.0f, 0.0f
        };
        glBindVertexArray(G.ch.VAO);
        glBindBuffer(GL_ARRAY_BUFFER, G.ch.VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(chVert), chVert, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(struct mc_CrosshairVertex), offsetof(struct mc_CrosshairVertex, x));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(struct mc_CrosshairVertex), offsetof(struct mc_CrosshairVertex, tx));
        glUseProgram(G.ch.prog);
        mc_program_set_int(G.ch.prog, "texcrosshair", 1);
        glm_ortho(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f, G.ch.proj);
        glUniformMatrix4fv(glGetUniformLocation(G.ch.prog, "proj"), 1, GL_FALSE, G.ch.proj);
    }

    /*
     *
     * Block Texture Atlas
     * 2D Texture Array
     * 
     */
    GLuint block_texatlas_2darray;
    {
        struct mc_Texture texatlas;
        mc_tex_load(&texatlas, "texatlas.jpg");
        glGenTextures(1, &block_texatlas_2darray);
        glBindTexture(GL_TEXTURE_2D_ARRAY, block_texatlas_2darray);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, texatlas.intfrmt, MC_BLOCKTEX_BLOCKSIZE, MC_BLOCKTEX_BLOCKSIZE, MC_BLOCKTEX_BLOCKS, 0, texatlas.datafrmt, GL_UNSIGNED_BYTE, NULL);
        for (size_t i = 0; i < MC_BLOCKTEX_BLOCKS; i++) {
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i,
                MC_BLOCKTEX_BLOCKSIZE, MC_BLOCKTEX_BLOCKSIZE, MC_BLOCKTEX_BLOCKS,
                texatlas.datafrmt, GL_UNSIGNED_BYTE,
                texatlas.data + i * MC_BLOCKTEX_BLOCKSIZE * MC_BLOCKTEX_BLOCKSIZE * texatlas.channels * sizeof(GLubyte)
            );
        }
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        mc_tex_unload(&texatlas);
    }

    /*
     *
     * World Shader
     * 
     */
    GLuint prog;
	mat4 view;
    GLuint uView;
	mat4 proj;
	glm_perspective(glm_rad(MC_FOV), (float)MC_WINDOW_WIDTH / (float)MC_WINDOW_HEIGHT, 0.1f, 1000.0f, proj);
    {
        prog = mc_program_create("MAIN", "C:/Users/Win10/Desktop/projects/mc/res/shaders/vertex.glsl", "C:/Users/Win10/Desktop/projects/mc/res/shaders/frag.glsl");
        if (prog == 0)
            goto saferet;
        glUseProgram(prog);
        glUniformMatrix4fv(glGetUniformLocation(prog, "proj"), 1, GL_FALSE, proj);
        mc_program_set_int(prog, "tex", 0);
        uView = glGetUniformLocation(prog, "view");
    }

    mc_tex_create(&G.texfont, "res/img/font.png");
	MC_BOOL can_place_block = MC_TRUE;
	MC_BOOL can_destroy_block = MC_TRUE;
	MC_BOOL player_moved = MC_FALSE;
	mc_camera_init(&G.camera);
    mc_textr_create(&G.textr);
	mc_world_init(&G.world);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	mc_world_push_block(&G.world, MC_BLOCK_TYPE_GRASS, 0, 0, 0, 0.0f);
	mc_world_push_union(&G.world, MC_BLOCK_TYPE_GRASS, 0, 0, 0, 15, 1, 15, 1.0f);

	while (!glfwWindowShouldClose(G.window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		     if (glfwGetKey(G.window, GLFW_KEY_A)          == GLFW_PRESS) glm_vec3_muladds(G.camera.right,  -MC_SPEED, G.camera.pos), player_moved = MC_TRUE;
		else if (glfwGetKey(G.window, GLFW_KEY_D)          == GLFW_PRESS) glm_vec3_muladds(G.camera.right,   MC_SPEED, G.camera.pos), player_moved = MC_TRUE;
			 if (glfwGetKey(G.window, GLFW_KEY_SPACE)      == GLFW_PRESS) glm_vec3_muladds((vec3){0,1,0},    MC_SPEED, G.camera.pos), player_moved = MC_TRUE;
		else if (glfwGetKey(G.window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) glm_vec3_muladds((vec3){0,1,0},   -MC_SPEED, G.camera.pos), player_moved = MC_TRUE;
		     if (glfwGetKey(G.window, GLFW_KEY_W)          == GLFW_PRESS) glm_vec3_muladds(G.camera.front,   MC_SPEED, G.camera.pos), player_moved = MC_TRUE;
		else if (glfwGetKey(G.window, GLFW_KEY_S)          == GLFW_PRESS) glm_vec3_muladds(G.camera.front,  -MC_SPEED, G.camera.pos), player_moved = MC_TRUE;
		
		if (player_moved || G.mouse.moved) {
            glUseProgram(prog);
			mc_camera_viewmatrix(&G.camera, view);
			glUniformMatrix4fv(uView, 1, GL_FALSE, view);
            if (player_moved)  player_moved  = MC_FALSE;
            if (G.mouse.moved) G.mouse.moved = MC_FALSE;
		}

		if (!can_place_block)
		if (glfwGetMouseButton(G.window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE)
			can_place_block = MC_TRUE;
		if (!can_destroy_block)
		if (glfwGetMouseButton(G.window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
			can_destroy_block = MC_TRUE;

		if (glfwGetKey(G.window, GLFW_KEY_TAB) == GLFW_PRESS)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		struct mc_Block *block_hit = hit_block_in_reach();
		if (block_hit != NULL) {
			mc_world_set_block(&G.world, 0,
                MC_BLOCK_TYPE_GRASS,
				blockpos(G.rayprehitpos[0]),
				blockpos(G.rayprehitpos[1]),
				blockpos(G.rayprehitpos[2]),
				0.6f
			);
			if (can_place_block)
			if (glfwGetMouseButton(G.window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
				place_block(
                    MC_BLOCK_TYPE_GRASS,
					blockpos(G.rayprehitpos[0]),
					blockpos(G.rayprehitpos[1]),
					blockpos(G.rayprehitpos[2])
				);
				can_place_block = MC_FALSE;
			}
			if (can_destroy_block)
			if (glfwGetMouseButton(G.window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
				if (block_isunion(block_hit)) {
					deunion_around_block(block_hit,
						blockpos(G.rayhitpos[0]),
						blockpos(G.rayhitpos[1]),
						blockpos(G.rayhitpos[2])
					);
				}
				mc_world_set_union(&G.world, block_hit->vertofs, MC_BLOCK_TYPE_GRASS, 0, 0, 0, 0, 0, 0, 0.0f);
				block_destroy(block_hit);
				can_destroy_block = MC_FALSE;
			} 

		}
		else {
			mc_world_set_block(&G.world, MC_BLOCK_TYPE_GRASS, 0, 0, 0, 0, 0.0f);
		}

        {
            snprintf(G.txt.pos, MC_TEXT_MAX_CHARS, "pos  %d %d %d (%.1f %.1f %.1f)", blockpos(G.camera.pos[0]), blockpos(G.camera.pos[1]), blockpos(G.camera.pos[2]), G.camera.pos[0], G.camera.pos[1], G.camera.pos[2]);
            snprintf(G.txt.look, MC_TEXT_MAX_CHARS, "look %d %d %d", G.camera.front[0] < 0 ? -1 : 1, G.camera.front[1] < 0 ? -1 : 1, G.camera.front[2] < 0 ? -1 : 1);
            if (block_hit != NULL)
            snprintf(G.txt.block, MC_TEXT_MAX_CHARS, "block hit %d %d %d", blockpos(G.rayhitpos[0]), blockpos(G.rayhitpos[1]), blockpos(G.rayhitpos[2]));
            snprintf(G.txt.blocks_in_reach, MC_TEXT_MAX_CHARS, "%u blocks in reach", G.blocks_in_reach_top);
            snprintf(G.txt.blocks_in_world, MC_TEXT_MAX_CHARS, "%u blocks in world", G.world.blockstop);
            mc_textr_push(&G.textr, 0.0f, 1.0f - MC_TEXT_CHAR_HEIGHT * 0, G.txt.pos);
            mc_textr_push(&G.textr, 0.0f, 1.0f - MC_TEXT_CHAR_HEIGHT * 1, G.txt.look);
            if (block_hit != NULL)
            mc_textr_push(&G.textr, 0.0f, 1.0f - MC_TEXT_CHAR_HEIGHT * 2, G.txt.block);
            mc_textr_push(&G.textr, 0.0f, 1.0f - MC_TEXT_CHAR_HEIGHT * 3, G.txt.blocks_in_reach);
            mc_textr_push(&G.textr, 0.0f, 1.0f - MC_TEXT_CHAR_HEIGHT * 4, G.txt.blocks_in_world);
        }

        /*
         *
         * Rendering
         * 
         */
        {
            // World
            glUseProgram(prog);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D_ARRAY, block_texatlas_2darray);
            mc_world_draw(&G.world, MC_BLOCK_VERTICES, G.world.blockvc - MC_BLOCK_VERTICES);
            mc_world_draw(&G.world, 0, MC_BLOCK_VERTICES);

            // Crosshair
            glUseProgram(G.ch.prog);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, G.ch.tex.id);
            glBindVertexArray(G.ch.VAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            mc_textr_draw(&G.textr);
        }

		glfwSwapBuffers(G.window);
		glfwPollEvents();
	}

	mc_program_delete(&prog);

saferet:
	glfwDestroyWindow(G.window);
	glfwTerminate();
	return EXIT_SUCCESS;
}

static void error_callback (int err, const char *desc) {
	printf("GLFW Error: \"%s\"\n", desc);
}

static void key_callback (GLFWwindow *window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS)
	if (key == GLFW_KEY_ESCAPE)
		glfwSetWindowShouldClose(window, 1);
}

static void cursor_pos_callback (GLFWwindow *window, double xpos, double ypos) {
	G.mouse.moved = MC_TRUE;

	double xofs = xpos - G.mouse.last_x;
	double yofs = G.mouse.last_y - ypos;
	
	G.mouse.last_x = xpos;
	G.mouse.last_y = ypos;

	mc_camera_mousemov(&G.camera, xofs, yofs);
}