#include "../ast.h"

typedef struct Builtin {
	const char *name;
	Function function;
	bool enabled;
} Builtin;

Builtin *getBuiltin(const char *name);
