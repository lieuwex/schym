#include <stdlib.h>
#include "intern.h"
#include "util.h"

typedef struct Map {
	size_t nkeys;
	char **keys;
	char **values;
} Map;
Map *createMap(void) {
	Map *map = malloc(1, sizeof(Map));
	map->nkeys = 0;
	map->keys = malloc(0, sizeof(char*));
	map->values = malloc(0, sizeof(char*));
	return map;
}
void map_free(Map *map, bool freeValues) {
	for (size_t i = 0; i < map->nkeys; i++) {
		free(map->keys[i]);
		if (freeValues) {
			free(map->values[i]);
		}
	}
	free(map->keys);
	free(map->values);
	free(map);
}
char *map_addItem(Map *map, const char *key, const char *value) {
	map->nkeys++;
	map->keys = realloc(map->keys, map->nkeys, sizeof(char*));
	map->values = realloc(map->values, map->nkeys, sizeof(char*));

	map->keys[map->nkeys-1] = astrcpy(key);
	map->values[map->nkeys-1] = astrcpy(value);

	return map->values[map->nkeys-1];
}
char *map_getItem(const Map *map, const char *key) {
	for (size_t i = 0; i < map->nkeys; i++) {
		if (streq(map->keys[i], key)) {
			return map->values[i];
		}
	}

	return NULL;
}

struct InternEnvironment {
	Map *map;
};
InternEnvironment *ie_make(void) {
	InternEnvironment *env = malloc(1, sizeof(InternEnvironment));
	env->map = createMap();
	return env;
}
void ie_free(InternEnvironment *env, bool freeValues) {
	if (env == NULL) {
		return;
	}

	map_free(env->map, freeValues);
	free(env);
}

InternedNode intern(const Node *node, InternEnvironment *env) {
	return skipIntern(node);

	Node *copy = node_copy(node);

	if (
		copy->type == AST_QUOTED &&
		copy->quoted.node->type == AST_VAR
	) {
		const char *name = copy->quoted.node->var.name;
		char *val = map_getItem(env->map, name);
		if (val == NULL) {
			val = map_addItem(env->map, name, name);
		}
		copy->quoted.node->var.name = val;
	} else if (copy->type == AST_QUOTED) {
		copy->quoted.node = intern(node->quoted.node, env).node;
	} else if (copy->type == AST_EXPR) {
		for (size_t i = 0; i < copy->expr.len; i++) {
			copy->expr.nodes[i] = intern(node->expr.nodes[i], env).node;
		}
	}

	InternedNode res;
	res.node = copy;
	return res;
}

InternedNode skipIntern(const Node *node) {
	InternedNode res;
	res.node = node_copy(node);
	return res;
}
