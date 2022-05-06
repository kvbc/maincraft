#include "mc.h"

#include <math.h>

static void update_vectors (struct mc_Camera *cam) {
    assert(cam != NULL);

	cam->front[0] = cosf(glm_rad(cam->yaw)) * cosf(glm_rad(cam->pitch));
	cam->front[1] = sinf(glm_rad(cam->pitch));
	cam->front[2] = sinf(glm_rad(cam->yaw)) * cosf(glm_rad(cam->pitch));
	glm_normalize(cam->front);

	glm_cross(cam->front, (vec3){0,1,0}, cam->right);
	glm_normalize(cam->right);
}

void mc_camera_init (struct mc_Camera *cam) {
    assert(cam != NULL);

	cam->pitch = 0.0f;
	cam->yaw = -90.0f;
	update_vectors(cam);
}

void mc_camera_mousemov (struct mc_Camera *cam, float ofsx, float ofsy) {
    assert(cam != NULL);

	cam->yaw   += ofsx * MC_MOUSE_SENS;
	cam->pitch += ofsy * MC_MOUSE_SENS;
    cam->pitch = glm_clamp(cam->pitch, -89.0f, 89.0f);
	update_vectors(cam);
}

void mc_camera_viewmatrix (struct mc_Camera *cam, mat4 dest) {
    assert(cam != NULL);
    assert(dest != NULL);

	static vec3 center;
	glm_vec3_add(cam->pos, cam->front, center);
	glm_lookat(cam->pos, center, (vec3){0,1,0}, dest);
}