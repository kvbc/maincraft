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

enum mc_Status mc_textr_create (struct mc_TextRenderer *textr) {
    assert(textr != NULL);

    textr->totallen = 0;
    textr->tmpvert = NULL;
    textr->tmpvertcap = 0;

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

    return MC_OK;
}

void mc_textr_destroy (struct mc_TextRenderer *textr) {
    assert(textr != NULL);

    free(textr->tmpvert);
    glDeleteVertexArrays(1, &textr->VAO);
	glDeleteBuffers(1, &textr->VBO);

}

void mc_textr_push (struct mc_TextRenderer *textr, float x, float y, const char *src) {
    assert(textr != NULL);
    assert(x >= 0.0f);
    assert(y >= 0.0f);
    assert(src != NULL);

    int len = strlen(src);
    assert(len <= MC_TEXT_MAX_CHARS);

    if (textr->tmpvert == NULL) {
        textr->tmpvertcap = len;
        textr->tmpvert = malloc(len * MC_TEXT_VERTICES * sizeof(struct mc_TextVertex));
        assert(textr->tmpvert != NULL);
    }
    else if (len > textr->tmpvertcap) {
        textr->tmpvertcap = len;
        textr->tmpvert = realloc(textr->tmpvert, len * MC_TEXT_VERTICES * sizeof(struct mc_TextVertex));
        assert(textr->tmpvert != NULL);
    }

    float x2 = x + MC_TEXT_CHAR_WIDTH;
    float y2 = y - MC_TEXT_CHAR_HEIGHT;

    for (size_t i = 0; i < len; i++) {
        int ci = src[i] - ' ';
        float tx =       (ci % MC_TEXT_ATLAS_COLS) * MC_TEXT_ATLAS_CHAR_WIDTH;
        float ty = 1.f - (ci / MC_TEXT_ATLAS_ROWS) * MC_TEXT_ATLAS_CHAR_HEIGHT;
        float tx2 = tx + MC_TEXT_ATLAS_CHAR_WIDTH;
        float ty2 = ty - MC_TEXT_ATLAS_CHAR_HEIGHT;

        float *b = (float*)&textr->tmpvert[i * MC_TEXT_VERTICES];

        b[0]=x2; b[1]=y;  b[2 ]=tx2; b[3 ]=ty;
        b[4]=x;  b[5]=y;  b[6 ]=tx;  b[7 ]=ty;
        b[8]=x;  b[9]=y2; b[10]=tx;  b[11]=ty2;

        b[12]=x2; b[13]=y;  b[14]=tx2; b[15]=ty;
        b[16]=x;  b[17]=y2; b[18]=tx;  b[19]=ty2;
        b[20]=x2; b[21]=y2; b[22]=tx2; b[23]=ty2;

        x  += MC_TEXT_CHAR_WIDTH;
        x2 += MC_TEXT_CHAR_WIDTH;
    }

    glBindBuffer(GL_ARRAY_BUFFER, textr->VBO);
	glBufferSubData(GL_ARRAY_BUFFER,
        textr->totallen * MC_TEXT_VERTICES * sizeof(struct mc_TextVertex),
        len * MC_TEXT_VERTICES * sizeof(struct mc_TextVertex),
        textr->tmpvert
    );

    textr->totallen += len;
}

void mc_textr_draw (struct mc_TextRenderer *textr) {
    assert(textr != NULL);

    glUseProgram(textr->prog);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textr->font.id);
    glBindVertexArray(textr->VAO);
	glDrawArrays(GL_TRIANGLES, 0, textr->totallen * MC_TEXT_VERTICES);

    textr->totallen = 0;
}