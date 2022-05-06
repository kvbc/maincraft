#include "mc.h"

inline void mc_vec3_set (vec3 v, float x, float y, float z) {
	v[0] = x;
	v[1] = y;
	v[2] = z;
}

inline void mc_vec4_set (vec4 v, float x, float y, float z, float w) {
	v[0] = x;
	v[1] = y;
	v[2] = z;
    v[3] = w;
}

inline void mc_ivec3_set (ivec3 v, int x, int y, int z) {
	v[0] = x;
	v[1] = y;
	v[2] = z;
}

void mc_ivec4_set (ivec4 v, int x, int y, int z, int w) {
    v[0] = x;
    v[1] = y;
    v[2] = z;
    v[3] = w;
}