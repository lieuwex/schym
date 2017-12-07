#pragma once

#include "./interpreter.h"
#include "./varmap.h"
#include "../ast.h"

// TODO: better way for range
#define EXPECT(cond, n) do { \
	if (!(nargs cond n)) { \
		return rr_errf("expected number of args to %s %d but is %d", #cond, n, nargs); \
	} \
} while(0)

typedef struct RunResult {
	// If err != NULL, node will be NULL.
	// Not-NULL values need to be freed.
	Node *node;
	char *err;
} RunResult;

typedef struct InterEnv {
	VarMap *variables;
	InterEnv *parent;
} InterEnv;

RunResult rr_null(void);
RunResult rr_errf(const char*, ...);
RunResult rr_node(Node*);

Node *getVar(const InterEnv*, const char*);

RunResult run(InterEnv*, const Node*);

double getNumVal(InterEnv*, const Node*);
