#include <assert.h>
#include <string.h>
#include "varstore.h"
#include "../util.h"

static void varstore_ensure_capacity(Varstore *vs, size_t capacity) {
	while (vs->cap < capacity) {
		vs->cap *= 2;
	}

	vs->keys = realloc(vs->keys, vs->cap, sizeof(char*));
	assert(vs->keys);

	vs->nodes = realloc(vs->nodes, vs->cap, sizeof(Node*));
	assert(vs->nodes);
}

Varstore *varstore_make(void) {
	Varstore *res = malloc(1, sizeof(Varstore));

	res->len = 0;
	res->cap = 4;
	res->keys = malloc(res->cap, sizeof(char*));
	res->nodes = malloc(res->cap, sizeof(Node*));

	return res;
}

void varstore_add_item(Varstore *vs, const char *key, Node *val) {
	char *k = astrcpy(key);
	Node *v = node_copy(val);

	for (size_t i = 0; i < vs->len; i++) {
		if (streq(vs->keys[i], key)) {
			free(vs->keys[i]);
			node_free(vs->nodes[i]);
			vs->keys[i] = k;
			vs->nodes[i] = v;
			return;
		}
	}

	vs->len++;
	varstore_ensure_capacity(vs, vs->len);
	vs->keys[vs->len - 1] = k;
	vs->nodes[vs->len - 1] = v;
}

void varstore_remove_item(Varstore *vs, const char *key) {
	size_t index;
	for (index = 0; index < vs->len; index++) {
		if (streq(vs->keys[index], key)) {
			break;
		}
	}
	assert(index != vs->len); // key doesn't exist
	memmove(
		vs->keys + index,
		vs->keys + index + 1,
		(vs->len - index - 1) * sizeof(char*)
	);
	memmove(
		vs->nodes + index,
		vs->nodes + index + 1,
		(vs->len - index - 1) * sizeof(Node*)
	);
	vs->len--;
}

void varstore_free(Varstore *vs) {
	for (size_t i = 0; i < vs->len; i++) {
		free(vs->keys[i]);
		node_free(vs->nodes[i]);
	}
	free(vs->keys);
	free(vs->nodes);
	free(vs);
}
