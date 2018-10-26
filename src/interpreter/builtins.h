#pragma once

#include "../ast.h"

typedef struct Builtin {
	const char *name;
	Function function;
	bool enabled;
} Builtin;

typedef RunResult (*BuiltinFn)(Scope*, const char*, size_t, const Node**);
typedef struct BuiltinList BuiltinList;

BuiltinList *builtins_make(bool);
BuiltinList *builtins_copy(const BuiltinList *builtins);
void builtins_free(BuiltinList *builtins);

Builtin *getBuiltin(BuiltinList *builtins, const char *name);
void addBuiltin(BuiltinList *builtins, const char *name, BuiltinFn fn);
void enableBuiltin(BuiltinList *builtins, const char *name, bool enable);
