#include "varmap.h"
#include "../util.h"
#include "../stringify.h"
#include <assert.h>
#include <string.h>

typedef struct VarMap {
	size_t nkeys;
	char **keys;
	Node **values;
} VarMap;

VarMap *varmap_make(void) {
	VarMap *map = malloc(1, sizeof(VarMap));
	map->nkeys = 0;
	map->keys = malloc(0, sizeof(char*));
	map->values = malloc(0, sizeof(Node*));
	return map;
}

Node *varmap_getItem(VarMap *map, const char *key) {
	if (streq(key, "nil")) {
		return NULL;
	}

	for (size_t i = 0; i < map->nkeys; i++) {
		if (streq(map->keys[i], key)) {
			return map->values[i]; // REVIEW: copy?
		}
	}

	return NULL;
}

void varmap_setItem(VarMap *map, const char *key, const Node *node) {
	varmap_removeItem(map, key);

	map->nkeys++;
	map->keys = realloc(map->keys, map->nkeys, sizeof(char*));
	map->values = realloc(map->values, map->nkeys, sizeof(Node*));

	map->keys[map->nkeys-1] = astrcpy(key);
	map->values[map->nkeys-1] = node_copy(node);
}

void varmap_removeItem(VarMap *map, const char *key) {
	// copied from tomjson

	size_t i;
	for (i = 0; i < map->nkeys; i++) {
		if (streq(map->keys[i], key)) break;
	}
	if (i == map->nkeys) {
		// key doesn't exist
		return;
	}
	free(map->keys[i]);
	memmove(
		map->keys + i,
		map->keys + i + 1,
		(map->nkeys - i - 1) * sizeof(char*)
	);
	node_free(map->values[i]);
	memmove(
		map->values + i,
		map->values + i + 1,
		(map->nkeys - i - 1) * sizeof(Node*)
	);
	map->nkeys--;
}

VarMap *varmap_copy(const VarMap *map) {
	VarMap *res = varmap_make();
	for (size_t i = 0; i < map->nkeys; i++) {
		varmap_setItem(res, map->keys[i], map->values[i]);
	}
	return res;
}

void varmap_print(const VarMap *map) {
	for (size_t i = 0; i < map->nkeys; i++) {
		const char *key = map->keys[i];
		const Node *node = map->values[i];
		printf("%s = %s\n", key, stringify(node, 0));
	}
}

void varmap_free(VarMap *map) {
	for (size_t i = 0; i < map->nkeys; i++) {
		free(map->keys[i]);
		free(map->values[i]);
	}
	free(map->keys);
	free(map->values);
	free(map);
}
