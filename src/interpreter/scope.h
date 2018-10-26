#pragma once

#include "./varmap.h"
#include "./builtins.h"

typedef struct Scope Scope;
struct Scope {
	Scope *parent;
	VarMap *variables;
	BuiltinList *builtins;
};

Scope *scope_make(Scope *parent, bool addPrelude);
Scope *scope_copy(Scope *scope);
void scope_free(Scope *scope);

Scope *scope_get_root(Scope *scope);
