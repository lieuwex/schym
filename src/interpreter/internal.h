#pragma once

#include "./interpreter.h"
#include "./varmap.h"
#include "../ast.h"

typedef struct RunResult {
	// If err != NULL, node will be NULL.
	// Not-NULL values need to be freed.
	Node *node;
	char *err;
} RunResult;

typedef struct InterEnv {
	VarMap *variables;
} InterEnv;

RunResult rr_null(void);
RunResult rr_errf(const char*, ...);
RunResult rr_node(Node *);

RunResult run(InterEnv*, const Node*);

double getNumVal(InterEnv *, const Node*);
