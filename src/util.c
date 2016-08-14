#include <ctype.h>
#include <string.h>
#include <assert.h>
#include "util.h"

bool isalphanum(const char c) {
	return isalpha(c) || isdigit(c);
}

bool streq(const char *a, const char *b) {
	return strcmp(a, b) == 0;
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
