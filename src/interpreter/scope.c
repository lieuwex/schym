#include <assert.h>
#include "scope.h"
#include "../util.h"

Scope *scope_make(Scope *parent, bool addPrelude) {
	Scope *res = malloc(1, sizeof(Scope));
	assert(res);

	res->variables = varmap_make();
	assert(res->variables);

	res->builtins = NULL;
	res->parent = parent;

	if (parent == NULL) {
		res->builtins = builtins_make(addPrelude);
		assert(res->builtins);
	}

	return res;
}

Scope *scope_copy(Scope *scope) {
	Scope *res = malloc(1, sizeof(Scope));
	assert(res);

	res->builtins = builtins_copy(scope->builtins);
	res->variables = varmap_copy(scope->variables);
	res->parent = scope->parent;

	return res;
}

void scope_free(Scope *scope) {
	if (scope == NULL) {
		return;
	}

	builtins_free(scope->builtins);
	varmap_free(scope->variables);
	free(scope);
}

Scope *scope_get_root(Scope *scope) {
	while (scope->parent != NULL) {
		scope = scope->parent;
	}
	return scope;
}
