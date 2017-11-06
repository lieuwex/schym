#pragma once

#include "../ast.h"
#include "../intern.h"

typedef struct InterEnv InterEnv;

InterEnv* in_make(void);
void in_destroy(InterEnv *is);

typedef struct RunResult{
	// If err != NULL, node will be NULL.
	// Not-NULL values need to be freed.
	Node *node;
	char *err;
} RunResult;

RunResult in_run(InterEnv*, const InternedNode);
