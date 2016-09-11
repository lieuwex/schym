#include <ctype.h>
#include <string.h>
#include <assert.h>
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
