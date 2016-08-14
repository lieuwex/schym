#pragma once

#include <stdbool.h>
#include <stdlib.h>

#define malloc(count, size) malloc(count * size)
#define realloc(ptr, count, size) realloc(ptr, count * size)

bool isalphanum(const char);

bool streq(const char*, const char*);

char *astrcpy(const char *src);
