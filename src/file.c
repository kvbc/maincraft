#include "mc.h"

#include <stdlib.h>
#include <assert.h>

enum mc_Status mc_file_read (const char* fn, char** buffPtr) {
    assert(fn != NULL);
    assert(buffPtr != NULL);
    *buffPtr = NULL;

    FILE *f = fopen(fn, "rb");
    if (f == NULL) {
        MC_PERR("Failed to open file \"%s\" for reading", fn);
        return MC_BAD;
    }

	int ret = MC_OK;

    if (fseek(f, 0, SEEK_END)) {
        MC_PERR("Failed to seek in file \"%s\"", fn);
        ret = MC_BAD;
        goto close_file;
    }
    long flen = ftell(f);
    if (fseek(f, 0, SEEK_SET)) {
        MC_PERR("Failed to seek in file \"%s\"", fn);
        ret = MC_BAD;
        goto close_file;
    }

    *buffPtr = malloc(sizeof(char) * (flen + 1));
    (*buffPtr)[flen] = '\0';
    if (*buffPtr == NULL) {
        MC_PERR("Out of memory while reading file \"%s\"", fn);
        ret = MC_BAD;
        goto close_file;
    }

	if (fread(*buffPtr, sizeof(char), flen, f) != flen) {
		MC_PERR("Failed to properly read from file \"%s\"", fn);
		ret = MC_BAD;
		goto close_file;
	}

close_file:
	if (fclose(f) == EOF) {
		MC_PERR("Failed to close file \"%s\"", fn);
		return MC_BAD;
	}

	return ret;
}