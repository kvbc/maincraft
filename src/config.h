#ifndef MC_CONFIG_H
#define MC_CONFIG_H

#define MC_WINDOW_WIDTH         (1920)
#define MC_WINDOW_HEIGHT        (1080)
#define MC_WINDOW_TITLE         ("Test")
#define MC_GLFW_CTX_V_MAJOR     (3)
#define MC_GLFW_CTX_V_MINOR     (3)
#define MC_GLFW_GL_PROFILE      (GLFW_OPENGL_CORE_PROFILE)
#define MC_MOUSE_SENS           (0.1f)
#define MC_BLOCK_SIZE           (0.1f)
#define MC_WORLD_DATA           (1000000000)
#define MC_WORLD_CPU_MAX_BLOCKS (10000000)
#define MC_CROSSHAIR_SIZE       (0.03f)
#define MC_REACH                (8)     // in blocks
#define MC_RAY_PRECISION        (0.01f) // lower - more precise
#define MC_SPEED                (0.02f) // lower - slower 
#define MC_FOV                  (90.0f)

#define MC_TEXT_ATLAS_ROWS        (16)
#define MC_TEXT_ATLAS_COLS        (16)
#define MC_TEXT_ATLAS_CHAR_WIDTH  (32.0f / 512.0f)
#define MC_TEXT_ATLAS_CHAR_HEIGHT (64.0f / 512.0f)
#define MC_TEXT_CHAR_WIDTH        (0.007f)
#define MC_TEXT_CHAR_HEIGHT       (MC_TEXT_ATLAS_CHAR_HEIGHT / MC_TEXT_ATLAS_CHAR_WIDTH * MC_TEXT_CHAR_WIDTH)
#define MC_TEXT_MAX_CHARS         ((int)(1.0f / MC_TEXT_CHAR_WIDTH))

#define MC_BLOCKTEX_BLOCKS    (3)
#define MC_BLOCKTEX_BLOCKSIZE (256)

#endif // MC_CONFIG_H