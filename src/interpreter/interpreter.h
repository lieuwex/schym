#pragma once

#include "../ast.h"

typedef struct InterState InterState;

InterState* in_make(void);
void in_destroy(InterState *is);

typedef struct RunResult{
	// If err != NULL, node will be NULL.
	// Not-NULL values need to be freed.
	Node *node;
	char *err;
} RunResult;

//returns the Node returned, or NULL if nothing
RunResult in_run(InterState *is,const Node *node);
