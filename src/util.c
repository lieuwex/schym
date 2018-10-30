#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "util.h"

bool streq(const char *a, const char *b) {
	return strcmp(a, b) == 0;
}

void strappend(char **str, const char *app) {
	size_t l1 = strlen(*str), l2 = strlen(app);
	*str = realloc(*str, l1 + l2 + 1, sizeof(char));
	assert(*str);
	memcpy(*str + l1, app, l2 + 1);
}

char *astrcpy(const char *src) {
	if (src == NULL) {
		return NULL;
	}

	size_t len = strlen(src);
	char *buf = malloc(len + 1, sizeof(char));
	assert(buf);
	memcpy(buf, src, len + 1);
	return buf;
}

char *emptystr(void) {
	char *res = malloc(1, sizeof(char));
	assert(res);
	res[0] = '\0';
	return res;
}

char *readfile(const char *fname) {
	FILE *f = fopen(fname, "rb");
	if (f == NULL) {
		return NULL;
	}
	if (fseek(f, 0, SEEK_END) == -1) {
		fclose(f);
		return NULL;
	}

	long flen = ftell(f);
	if (flen == -1) {
		fclose(f);
		return NULL;
	}
	rewind(f);

	char *buf = malloc(flen + 1, sizeof(char));
	if (buf == NULL) {
		fclose(f);
		return NULL;
	}

	fread(buf, 1, flen, f);
	if (ferror(f)) {
		fclose(f);
		free(buf);
		return NULL;
	}

	if (memchr(buf, '\0', flen) != NULL) {
		fprintf(stderr, "Invalid null char in file '%s'\n", fname);
		exit(1);
	}

	buf[flen] = '\0';
	fclose(f);
	return buf;
}
