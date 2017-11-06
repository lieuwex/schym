#include "../ast.h"
#include "./interpreter.h"

RunResult rr_null(void);
RunResult rr_errf(const char*, ...);
RunResult rr_node(Node *);

RunResult run(InterEnv*, const Node*);
