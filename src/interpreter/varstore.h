#include <ctype.h>
#include "../ast.h"

typedef struct Varstore {
	char **keys;
	Node **nodes;
	size_t len;
	size_t cap;
} Varstore;
Varstore *varstore_make(void);
void varstore_free(Varstore *vs);
void varstore_add_item(Varstore *vs, const char *key, Node *val);
void varstore_remove_item(Varstore *vs, const char *key);
