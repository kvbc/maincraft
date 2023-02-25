#include "mc.h"

inline int mc_block_coord (float xyz) {
    return (int)floorf(xyz / MC_BLOCK_SIZE);
}