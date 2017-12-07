#include "../ast.h"

typedef struct Builtin {
	const char *name;
	Function function;
	bool enabled;
} Builtin;

typedef RunResult (*BuiltinFn)(InterEnv*, const char*, size_t, const Node**);
typedef struct BuiltinList BuiltinList;

Builtin *getBuiltin(const char *name);
void addBuiltin(const char *name, BuiltinFn fn);
void enableBuiltin(const char *name, bool enable);
