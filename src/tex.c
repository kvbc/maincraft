#include "mc.h"

#include <assert.h>

enum mc_Status mc_tex_load (struct mc_Texture *tex, const char *fn) {
    assert(tex != NULL);
    assert(fn != NULL);

	tex->data = stbi_load(fn, &tex->width, &tex->height, &tex->channels, 0);
	if (tex->data == NULL) {
		MC_PERR("Failed to load texture \"%s\"\n", fn);
		return MC_BAD;
	}

    if (tex->channels == 3) {
        tex->intfrmt = GL_RGB8;
		tex->datafrmt = GL_RGB;
	}
	else if (tex->channels == 4) {
        tex->intfrmt = GL_RGBA8;
		tex->datafrmt = GL_RGBA;
	}
    else {
        MC_PERR("Failed to load texture \"%s\", unsupported file format\n", fn);
        mc_tex_unload(tex);
		return MC_BAD;
    }

    return MC_OK;
}

void mc_tex_unload (struct mc_Texture *tex) {
    assert(tex != NULL);
    stbi_image_free(tex->data);
}

enum mc_Status mc_tex_create (struct mc_Texture *tex, const char *fn) {
    assert(tex != NULL);
    assert(fn != NULL);

	glGenTextures(1, &tex->id);

	glBindTexture(GL_TEXTURE_2D, tex->id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (mc_tex_load(tex, fn) == MC_BAD)
        return MC_BAD;

	glTexImage2D(GL_TEXTURE_2D, 0, tex->intfrmt, tex->width, tex->height, 0, tex->datafrmt, GL_UNSIGNED_BYTE, tex->data);
	glGenerateMipmap(GL_TEXTURE_2D);

	mc_tex_unload(tex);

	return MC_OK;
}