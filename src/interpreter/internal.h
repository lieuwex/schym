#include "../ast.h"
#include "./interpreter.h"
#include "./varmap.h"

typedef struct InterEnv {
	VarMap *variables;
} InterEnv;

RunResult rr_null(void);
RunResult rr_errf(const char*, ...);
RunResult rr_node(Node *);

RunResult run(InterEnv*, const Node*);

double getNumVal(InterEnv *, const Node*);
