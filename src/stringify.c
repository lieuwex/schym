#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "stringify.h"
#include "util.h"

char *typetostr(const Node *node) {
	switch (node->type) {
	case AST_QUOTED: {
		char *res;
		asprintf(&res, "quoted %s", typetostr(node->quoted.node));
		return res;
	}
	case AST_EXPR: return "expression";
	case AST_VAR: return "variable";
	case AST_STR: return "string";
	case AST_NUM: return "number";
	case AST_COMMENT: return "comment";
	case AST_FUN: return "function";

	default: return "UNKNOWN";
	}
}

// #define INDENT "\t"
#define INDENT "  "

char *stringify(const Node *node, int lvl) {
	assert(node);
	char *res = malloc(1, sizeof(char));
	assert(res);
	res[0] = '\0';

	switch (node->type) {
	case AST_QUOTED:
		strappend(&res, "'");
		char *str = stringify(node->quoted.node, lvl);
		strappend(&res, str);
		free(str);
		break;

	case AST_EXPR:
		strappend(&res, "(");
		bool didindent = false;
		bool issmall = node->expr.len < 4;
		for (size_t i = 0; i < node->expr.len; i++) {
			if (i != 0) {
				didindent = true;
				if (issmall) {
					strappend(&res, " ");
				} else {
					strappend(&res, "\n");
					for (int x = 0; x < lvl + 1; x++) {
						strappend(&res, INDENT);
					}
				}
			}
			char *str = stringify(node->expr.nodes[i], lvl + didindent);
			strappend(&res, str);
			free(str);
		}
		strappend(&res, ")");
		break;

	case AST_VAR:
		strappend(&res, node->var.name);
		break;

	case AST_STR:
		strappend(&res, "\"");
		strappend(&res, node->str.str);
		strappend(&res, "\"");
		break;

	case AST_NUM: {
		char *buf;
		asprintf(&buf, "%g", node->num.val);
		assert(buf);
		strappend(&res, buf);
		free(buf);
		break;
	}

	case AST_COMMENT: {
		strappend(&res, "; ");
		strappend(&res, node->comment.content);
		break;
	}

	case AST_FUN: {
		if (node->function.isBuiltin) {
			strappend(&res, "[ builtin function ]");
		} else {
			const Expression *args = &node->function.args;
			Node *body = node->function.body;

			strappend(&res, "(fun ");

			Node *node = malloc(1, sizeof(Node));
			node->type = AST_EXPR;
			node->expr = *args;
			strappend(&res, stringify(node, lvl + 1));
			strappend(&res, " ");
			free(node);

			strappend(&res, stringify(body, lvl + 1));
			strappend(&res, ")");
		}
		break;
	}
	}

	return res;
}

char *toString(const Node *node) {
	if (node == NULL) {
		return astrcpy("nil");
	}

	switch (node->type) {
	case AST_STR:
		return astrcpy(node->str.str);

	default:
		return stringify(node, 0);
	}
}
