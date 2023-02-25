#include "mc.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <FastNoiseLite.h>

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
        char offset[MC_TEXT_MAX_CHARS];
        char vertices[MC_TEXT_MAX_CHARS];
        char mem_vertices[MC_TEXT_MAX_CHARS];
        char mem_blocks[MC_TEXT_MAX_CHARS];
        char mem_total[MC_TEXT_MAX_CHARS];
    } txt;

    struct mc_TextRenderer textr;
    
    vec3 rayhitpos;
    vec3 rayprehitpos;
} G = {0};

/*
 *
 * Block
 * 
 */

static struct mc_Block* hit_block_in_reach (void) {
	// Standard unoptimized ray marching
	glm_vec3_copy(G.camera.pos, G.rayhitpos);
	for (size_t r = 0; r < MC_REACH / MC_RAY_PRECISION; r++) {
        for (int x = mc_block_coord(G.camera.pos[0]) - MC_REACH; x < mc_block_coord(G.camera.pos[0]) + MC_REACH; x++)
        for (int y = mc_block_coord(G.camera.pos[1]) - MC_REACH; y < mc_block_coord(G.camera.pos[1]) + MC_REACH; y++)
        for (int z = mc_block_coord(G.camera.pos[2]) - MC_REACH; z < mc_block_coord(G.camera.pos[2]) + MC_REACH; z++) {
            struct mc_Block *block = mc_world_block_at(&G.world, x, y, z);
            if (block == NULL)
                continue;
            if (!block->exists)
                continue;

            if (mc_block_coord(G.rayhitpos[0]) == x)
            if (mc_block_coord(G.rayhitpos[1]) == y)
            if (mc_block_coord(G.rayhitpos[2]) == z) {
                // revert last step
                G.rayprehitpos[0] = G.rayhitpos[0] - G.camera.front[0] * MC_BLOCK_SIZE * MC_RAY_PRECISION;
                G.rayprehitpos[1] = G.rayhitpos[1] - G.camera.front[1] * MC_BLOCK_SIZE * MC_RAY_PRECISION;
                G.rayprehitpos[2] = G.rayhitpos[2] - G.camera.front[2] * MC_BLOCK_SIZE * MC_RAY_PRECISION;
                return block;
            }
        }
        // step
        G.rayhitpos[0] += G.camera.front[0] * MC_BLOCK_SIZE * MC_RAY_PRECISION;
        G.rayhitpos[1] += G.camera.front[1] * MC_BLOCK_SIZE * MC_RAY_PRECISION;
        G.rayhitpos[2] += G.camera.front[2] * MC_BLOCK_SIZE * MC_RAY_PRECISION;
	}
	return NULL;
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

	// glfwSwapInterval(0); // VSYNC
	// glViewport(0, 0, MC_WINDOW_WIDTH, MC_WINDOW_HEIGHT);

    stbi_set_flip_vertically_on_load(1);

    // glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


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
     * 
     */
    GLuint block_texatlas;
    {
        struct mc_Texture texatlas;
        mc_tex_load(&texatlas, "texatlas.jpg");
        glGenTextures(1, &block_texatlas);
        glBindTexture(GL_TEXTURE_2D, block_texatlas);
        glTexImage2D(GL_TEXTURE_2D, 0, texatlas.intfrmt, MC_BLOCKTEX_BLOCKSIZE, MC_BLOCKTEX_BLOCKS * MC_BLOCKTEX_BLOCKSIZE, 0, texatlas.datafrmt, GL_UNSIGNED_BYTE, texatlas.data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
	mc_world_init(&G.world, 0);
    G.mouse.moved = MC_TRUE;

    MC_BOOL reset_indicator_block = MC_FALSE;

    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_FRONT);
    // glFrontFace(GL_CW);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // struct mc_Block indicator_block = {
    //     .type = MC_BLOCK_TYPE_GRASS,
    //     .vertofs_back  = MC_BLOCK_FACE_VERTICES * 0,
    //     .vertofs_front = MC_BLOCK_FACE_VERTICES * 1,
    //     .vertofs_top   = MC_BLOCK_FACE_VERTICES * 2,
    //     .vertofs_bot   = MC_BLOCK_FACE_VERTICES * 3,
    //     .vertofs_left  = MC_BLOCK_FACE_VERTICES * 4,
    //     .vertofs_right = MC_BLOCK_FACE_VERTICES * 5
    // };
    // G.world.blockvc += MC_BLOCK_VERTICES;

    /*
     *
     * Terrain Generation
     * 
     */
    {
        fnl_state fnl = fnlCreateState();
        fnl.noise_type = FNL_NOISE_PERLIN;

        for (int x = 0; x < MC_RENDER_DISTANCE; x++)
        for (int z = 0; z < MC_RENDER_DISTANCE; z++)
        for (int y = 0; y < MC_WORLD_HEIGHT;    y++) {
            float noise = fnlGetNoise3D(&fnl, x, y, z);
            if (noise > 0.0f)
                mc_world_place_block_at(&G.world, x, y, z, MC_BLOCK_TYPE_GRASS);
        }

        // mc_world_place_block_at(&G.world, 0, 0, 0, MC_BLOCK_TYPE_GRASS);
        // mc_world_place_block_at(&G.world, 1, 0, 0, MC_BLOCK_TYPE_GRASS);
        // mc_world_place_block_at(&G.world, 2, 0, 0, MC_BLOCK_TYPE_GRASS);

        // mc_world_update(&G.world);
    }
    // mc_world_place_block_at(&G.world, 0, 0, 0, MC_BLOCK_TYPE_GRASS);
    // mc_update_vertices(&G.world);

    // printf("%d, %d\n", G.world.vertices_top, G.world.indices_top);
    // for (int i = 0; i < G.world.vertices_top; i++) {
    //     printf("%f, ", G.world.vertices[i]);
    // }
    // puts("");
    // for (int i = 0; i < MC_MIN(100, G.world.indices_top); i++) {
    //     printf("%d, ", G.world.indices[i]);
    // }
    // puts("");

    puts("done!");
    glClearColor(0.67f, 0.84f, 1.0f, 1.0f);
    double last_time = glfwGetTime();
    size_t fps = 0;

	while (!glfwWindowShouldClose(G.window)) {
        fps++;
        double cur_time = glfwGetTime();
        if (cur_time - last_time >= 1.0) {
            printf("fps: %d\n", fps);
            fps = 0;
            last_time = cur_time;
        }
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        /*
         *
         * Input handling
         * 
         */
        {
            float speed = MC_SPEED;
            if (glfwGetKey(G.window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
                speed *= 5;

                 if (glfwGetKey(G.window, GLFW_KEY_A)          == GLFW_PRESS) glm_vec3_muladds(G.camera.right,  -speed, G.camera.pos), player_moved = MC_TRUE;
            else if (glfwGetKey(G.window, GLFW_KEY_D)          == GLFW_PRESS) glm_vec3_muladds(G.camera.right,   speed, G.camera.pos), player_moved = MC_TRUE;
                 if (glfwGetKey(G.window, GLFW_KEY_SPACE)      == GLFW_PRESS) glm_vec3_muladds((vec3){0,1,0},    speed, G.camera.pos), player_moved = MC_TRUE;
            else if (glfwGetKey(G.window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) glm_vec3_muladds((vec3){0,1,0},   -speed, G.camera.pos), player_moved = MC_TRUE;
                 if (glfwGetKey(G.window, GLFW_KEY_W)          == GLFW_PRESS) glm_vec3_muladds(G.camera.front,   speed, G.camera.pos), player_moved = MC_TRUE;
            else if (glfwGetKey(G.window, GLFW_KEY_S)          == GLFW_PRESS) glm_vec3_muladds(G.camera.front,  -speed, G.camera.pos), player_moved = MC_TRUE;
            
            if (glfwGetKey(G.window, GLFW_KEY_TAB) == GLFW_PRESS)
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            else
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            if (glfwGetKey(G.window, GLFW_KEY_L) == GLFW_PRESS)
                mc_world_move(&G.world, 1, 0, 0);
        }

        /*
         *
         * Player
         * 
         */
        {
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
        }

        /*
         *
         * Block
         * 
         */
        struct mc_Block *block_hit = hit_block_in_reach();
        {
            if (block_hit != NULL) {
                // mc_world_send_faces(&G.world, &indicator_block,
                //     mc_block_coord(G.rayprehitpos[0]),
                //     mc_block_coord(G.rayprehitpos[1]),
                //     mc_block_coord(G.rayprehitpos[2]),
                //     MC_INDICATOR_BLOCK_ALPHA,
                //     MC_BLOCK_FACE_ALL
                // );
                reset_indicator_block = MC_TRUE;

                if (can_place_block)
                if (glfwGetMouseButton(G.window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
                    // COMMENT
                    int x = mc_block_coord(G.rayprehitpos[0]);
                    int y = mc_block_coord(G.rayprehitpos[1]);
                    int z = mc_block_coord(G.rayprehitpos[2]);
                    mc_world_place_block_at(&G.world, x, y, z, MC_BLOCK_TYPE_GRASS);
                    // mc_world_update(&G.world);
                    can_place_block = MC_FALSE;
                }
                if (can_destroy_block)
                if (glfwGetMouseButton(G.window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                    // COMMENT
                    int x = mc_block_coord(G.rayhitpos[0]);
                    int y = mc_block_coord(G.rayhitpos[1]);
                    int z = mc_block_coord(G.rayhitpos[2]);
                    // struct mc_Block * block = mc_world_block_at(&G.world, x, y, z);
                    // if (block != NULL)
                    //     mc_save_block(&G.world, x, y, z, block->type);
                    mc_world_destroy_block_at(&G.world, x, y, z);
                    // mc_world_update(&G.world);
                    can_destroy_block = MC_FALSE;
                } 

            }
            else if (reset_indicator_block) {
                // mc_world_send_faces(&G.world, &indicator_block, 0, 0, 0, 0.0f, MC_BLOCK_FACE_ALL);
                reset_indicator_block = MC_FALSE;
            }
        }

        /*
         *
         * Setting up the text stack
         * 
         */
        {
            if (block_hit == NULL)
                snprintf(G.txt.block, MC_TEXT_MAX_CHARS, "block hit           : NONE");
            else
                snprintf(G.txt.block, MC_TEXT_MAX_CHARS, "block hit           : %d, %d, %d", mc_block_coord(G.rayhitpos[0]), mc_block_coord(G.rayhitpos[1]), mc_block_coord(G.rayhitpos[2]));

            unsigned long long mem_vertices      = G.world.face_indices_top * sizeof(float) / 1024 / 1024;
            unsigned long long mem_blocks        = MC_WORLD_MAX_BLOCKS * sizeof(struct mc_Block) / 1024 / 1024;
            unsigned long long mem_total         = mem_vertices + mem_blocks;

            snprintf(G.txt.pos,               MC_TEXT_MAX_CHARS, "pos                 : %d, %d, %d",           mc_block_coord(G.camera.pos[0]), mc_block_coord(G.camera.pos[1]), mc_block_coord(G.camera.pos[2]));
            snprintf(G.txt.look,              MC_TEXT_MAX_CHARS, "look                : %d, %d, %d",           G.camera.front[0] < 0 ? -1 : 1, G.camera.front[1] < 0 ? -1 : 1, G.camera.front[2] < 0 ? -1 : 1);
            snprintf(G.txt.offset,            MC_TEXT_MAX_CHARS, "offset              : %d, %d, %d",           G.world.offset[0], G.world.offset[1], G.world.offset[2]);
            snprintf(G.txt.vertices,          MC_TEXT_MAX_CHARS, "vertices            : %llu / %llu (%.1f%%)", G.world.face_indices_top / 7, MC_WORLD_MAX_VERTICES / 7, G.world.face_indices_top / (double)MC_WORLD_MAX_VERTICES * 100);
            snprintf(G.txt.mem_vertices,      MC_TEXT_MAX_CHARS, "mem - vertices      : %llu MB (%.1f%%)",     mem_vertices, mem_vertices / (double)mem_total * 100);
            snprintf(G.txt.mem_blocks,        MC_TEXT_MAX_CHARS, "mem - blocks        : %llu MB (%.1f%%)",     mem_blocks,   mem_blocks   / (double)mem_total * 100);
            snprintf(G.txt.mem_total,         MC_TEXT_MAX_CHARS, "mem - total         : %llu MB",              mem_total);
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
            glBindTexture(GL_TEXTURE_2D, block_texatlas);
            // mc_world_draw(&G.world, 1, (G.world.face_indices_top - MC_BLOCK_FACES) / MC_BLOCK_FACES);
            // mc_world_draw(&G.world, 0, 1);
            mc_world_draw(&G.world, 0, G.world.face_indices_top / MC_BLOCK_FACES);

            // Crosshair
            glUseProgram(G.ch.prog);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, G.ch.tex.id);
            glBindVertexArray(G.ch.VAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            mc_textr_draw(&G.textr, 0.0f, 1.0f - MC_TEXT_CHAR_HEIGHT * 0, G.txt.pos);
            mc_textr_draw(&G.textr, 0.0f, 1.0f - MC_TEXT_CHAR_HEIGHT * 1, G.txt.look);
            mc_textr_draw(&G.textr, 0.0f, 1.0f - MC_TEXT_CHAR_HEIGHT * 2, G.txt.block);
            mc_textr_draw(&G.textr, 0.0f, 1.0f - MC_TEXT_CHAR_HEIGHT * 3, G.txt.offset);
            mc_textr_draw(&G.textr, 0.0f, 1.0f - MC_TEXT_CHAR_HEIGHT * 4, G.txt.vertices);
            mc_textr_draw(&G.textr, 0.0f, 1.0f - MC_TEXT_CHAR_HEIGHT * 5, G.txt.mem_vertices);
            mc_textr_draw(&G.textr, 0.0f, 1.0f - MC_TEXT_CHAR_HEIGHT * 6, G.txt.mem_blocks);
            mc_textr_draw(&G.textr, 0.0f, 1.0f - MC_TEXT_CHAR_HEIGHT * 7, G.txt.mem_total);
        }

		glfwSwapBuffers(G.window);
		glfwPollEvents();
	}

    mc_world_free(&G.world);
	mc_program_delete(prog);

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