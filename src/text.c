/*
 *
 * Texture-atlas text batch renderer
 * Used *only* to display debug information
 * 
 * res/shaders/text.vert
 * res/shaders/text.frag
 * res/img/font.png
 *
 */

#include "mc.h"

void mc_textr_create (struct mc_TextRenderer *textr) {
    assert(textr != NULL);

    // program
    textr->prog = mc_program_create("TEXT", "C:/Users/Win10/Desktop/projects/mc/res/shaders/text.vert", "C:/Users/Win10/Desktop/projects/mc/res/shaders/text.frag");
    assert(textr->prog != 0);
    glUseProgram(textr->prog);
    mc_program_set_int(textr->prog, "tex", 0);
    mat4 proj;
	glm_ortho(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f, proj);
	glUniformMatrix4fv(glGetUniformLocation(textr->prog, "proj"), 1, GL_FALSE, proj);

    // font
    mc_tex_create(&textr->font, "res/img/font.png");

    glGenVertexArrays(1, &textr->VAO);
    glGenBuffers(1, &textr->VBO);
    glBindVertexArray(textr->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, textr->VBO);
	glBufferData(GL_ARRAY_BUFFER, MC_TEXT_MAX_CHARS * MC_TEXT_VERTICES * sizeof(struct mc_TextVertex), NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(struct mc_TextVertex), offsetof(struct mc_TextVertex, x));
    glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(struct mc_TextVertex), offsetof(struct mc_TextVertex, tx));
}

void mc_textr_destroy (struct mc_TextRenderer *textr) {
    assert(textr != NULL);
    glDeleteVertexArrays(1, &textr->VAO);
	glDeleteBuffers(1, &textr->VBO);
}

void mc_textr_draw (struct mc_TextRenderer *textr, float x, float y, const char * src) {
    assert(textr != NULL);
    assert(x >= 0.0f);
    assert(y >= 0.0f);
    assert(src != NULL);

    int len = strlen(src);
    assert(len <= MC_TEXT_MAX_CHARS);

    static GLfloat vertices[MC_TEXT_MAX_CHARS * MC_TEXT_VERTICES * MC_TEXT_VERTEX_ELEMENTS];
    GLfloat * v = &vertices;

    float x2 = x + MC_TEXT_CHAR_WIDTH;
    float y2 = y - MC_TEXT_CHAR_HEIGHT;
    for (size_t i = 0; i < len; i++) {
        int ci = src[i] - ' ';
        float tx =       (ci % MC_TEXT_ATLAS_COLS) * MC_TEXT_ATLAS_CHAR_WIDTH;
        float ty = 1.0 - (ci / MC_TEXT_ATLAS_COLS) * MC_TEXT_ATLAS_CHAR_HEIGHT;
        float tx2 = tx + MC_TEXT_ATLAS_CHAR_WIDTH;
        float ty2 = ty - MC_TEXT_ATLAS_CHAR_HEIGHT;

        v[0]=x2; v[1]=y;  v[2 ]=tx2; v[3 ]=ty;
        v[4]=x;  v[5]=y;  v[6 ]=tx;  v[7 ]=ty;
        v[8]=x;  v[9]=y2; v[10]=tx;  v[11]=ty2;

        v[12]=x2; v[13]=y;  v[14]=tx2; v[15]=ty;
        v[16]=x;  v[17]=y2; v[18]=tx;  v[19]=ty2;
        v[20]=x2; v[21]=y2; v[22]=tx2; v[23]=ty2;

        x  += MC_TEXT_CHAR_WIDTH;
        x2 += MC_TEXT_CHAR_WIDTH;
        v += MC_TEXT_VERTICES * MC_TEXT_VERTEX_ELEMENTS;
    }

    glBindBuffer(GL_ARRAY_BUFFER, textr->VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0,
        len * MC_TEXT_VERTICES * sizeof(struct mc_TextVertex),
        vertices
    );

    glUseProgram(textr->prog);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textr->font.id);
    glBindVertexArray(textr->VAO);
	glDrawArrays(GL_TRIANGLES, 0, len * MC_TEXT_VERTICES);
}