#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ast.h"
#include "util.h"
#include "stringify.h"

// TODO: handle errors way better

static Node *make_node(ASTtype type) {
	Node *res = malloc(1, sizeof(Node));
	res->type = type;
	return res;
}

static ParseResult make_parse_res(void) {
	ParseResult res;
	res.node = NULL;
	res.err = NULL;
	return res;
}

static char *expr_add_item(Expression *expr, Node *node) {
	if (expr == NULL) {
		return "expr == NULL";
	} else if (node == NULL) {
		return "node == NULL";
	}

	expr->len++;
	expr->nodes = realloc(expr->nodes, expr->len, sizeof(Node*));
	if (expr->nodes == NULL) {
		return "out of memory";
	}

	expr->nodes[expr->len - 1] = node;

	return NULL;
}

static void skipspaces(const char **codep) {
	const char *code = *codep;
	while (isspace(*code)) {
		code++;
	}
	*codep = code;
}

static char *copystringfromend(const char *src, size_t len) {
	char *res = malloc(len + 1, sizeof(char));
	if (res == NULL) {
		return NULL;
	}
	memcpy(res, src - len, len);
	res[len] = '\0';
	return res;
}

char *typetostr(const ASTtype type) {
	switch (type) {
	case AST_EXPR: return "expression";
	case AST_VAR: return "variable";
	case AST_STR: return "string";
	case AST_NUM: return "number";
	case AST_COMMENT: return "comment";
	}
}

ParseResult _parse(const char **codep);

static ParseResult parsecomment(const char **codep) {
	const char *code = *codep;
	ParseResult res = make_parse_res();

	if (*code != ';') {
		return res;
	}
	code++;

	size_t len = 0;
	while (*code && *code != '\n') {
		code++;
		len++;
	}

	char *content = copystringfromend(code, len);

	res.node = make_node(AST_COMMENT);
	res.node->comment.content = content;

	*codep = code;
	return res;
}

static ParseResult parsenumber(const char **codep) {
	const char *code = *codep;
	ParseResult res = make_parse_res();

	if (!isdigit(*code) && *code != '.') {
		return res;
	}

	size_t len = 0;
	while (isalphanum(*code) || *code == '.') {
		code++;
		len++;
	}

	char *buf = copystringfromend(code, len);
	char *endptr;
	double val = strtod(buf, &endptr);
	if (*endptr != '\0') {
		res.err = "invalid number";
		return res;
	}
	free(buf);

	res.node = make_node(AST_NUM);
	res.node->num.val = val;

	*codep = code;
	return res;
}

static ParseResult parsestring(const char **codep) {
	const char *code = *codep;
	ParseResult res = make_parse_res();

	if (*code != '"') {
		return res;
	}
	code++;

	size_t len = 0;
	while (*code != '"') {
		if (*code == '\0' || *code == '\n') {
			res.err = "string unended";
			return res;
		}
		code++;
		len++;
	}

	char *content = copystringfromend(code, len);

	res.node = make_node(AST_STR);
	res.node->str.size = len;
	res.node->str.str = content;

	*codep = code + 1; // continue after the string closing char
	return res;
}

static ParseResult parseexpression(const char **codep) {
	const char *code = *codep;
	ParseResult res = make_parse_res();

	bool isquoted = false;

	if (code[0] == '\'') {
		if (code[1] != '(') {
			// TODO: everything should be able to be quoted, we can hold the
			// quoted state in a state machine we give in each parse function,
			// this could also then contain the current index.
			res.err = "invalid quoted expression";
			return res;
		}
		isquoted = true;
		code += 2;
	} else if (code[0] == '(' || code[0] == '[') {
		code++;
	} else {
		return res;
	}

	char closetag = code[-1] == '(' ? ')' : ']';

	Node *node = make_node(AST_EXPR);
	node->expr.len = 0;
	node->expr.nodes = malloc(1, sizeof(Node*));
	node->expr.isquoted = isquoted;

	while (*code != closetag) {
		if (*code == '\0') {
			char *err;
			asprintf(&err, "missing '%c'", closetag);
			res.err = err;
			break;
		}

		ParseResult item = _parse(&code);
		if (item.err != NULL) {
			res.err = item.err;
			break;
		} else if (item.node == NULL) {
			char *err;
			asprintf(&err, "unexpected '%c'", *code);
			res.err = err;
			break;
		}

		char *err = expr_add_item(&node->expr, item.node);
		if (err != NULL) {
			res.err = err;
			break;
		}

		skipspaces(&code);
	}
	code++;

	if (res.err != NULL) {
		node_free(node);
		*codep = code;
		return res;
	}

	res.node = node;
	*codep = code;
	return res;
}

static ParseResult parsevariable(const char **codep) {
	const char *code = *codep;
	ParseResult res = make_parse_res();

	if (!isalpha(*code)) {
		return res;
	}

	size_t len = 0;
	while (*code && isalphanum(*code)) {
		len++;
		code++;
	}

	char *name = copystringfromend(code, len);

	res.node = make_node(AST_VAR);
	res.node->var.name = name;

	*codep = code;
	return res;
}

// Loop over every paser and return the result of the one that passes
ParseResult _parse(const char **codep) {
	ParseResult res = make_parse_res();
	if (!*codep) return res;
	skipspaces(codep);


	res = parsecomment(codep); if (res.node || res.err) return res;
	res = parsenumber(codep); if (res.node || res.err) return res;
	res = parsestring(codep); if (res.node || res.err) return res;
	res = parseexpression(codep); if (res.node || res.err) return res;

	// should be last
	res = parsevariable(codep); if (res.node || res.err) return res;

	return res; // nothing found
}

ParseResult parse(const char *code) {
	return _parse(&code);
}

ProgramParseResult parseprogram(const char *code) {
	ProgramParseResult res;
	res.cap = 4;
	res.len = 0;
	res.err = NULL;
	res.nodes = malloc(res.cap, sizeof(Node*));

	while (*code != '\0') {
		ParseResult item = _parse(&code);
		if (item.err != NULL) {
			res.err = item.err;
			break;
		} else if (item.node == NULL) {
			char *err;
			if (*code == ')' || *code == ']') {
				asprintf(&err, "extraneous '%c'", *code);
			} else {
				asprintf(&err, "unexpected '%c'", *code);
			}
			res.err = err;
			break;
		} else if (item.node->type != AST_EXPR) {
			char *err;
			asprintf(&err, "expected an expression, got a %s instead", typetostr(item.node->type));
			res.err = err;
			break;
		}

		res.len++;
		if (res.len > res.cap) {
			res.cap *= 2;
			res.nodes = realloc(res.nodes, res.cap, sizeof(Node*));
		}
		res.nodes[res.len - 1] = item.node;

		skipspaces(&code);
	}

	if (res.err != NULL) {
		for (size_t i = 0; i < res.len; i++) {
			node_free(res.nodes[i]);
		}
		free(res.nodes);
		res.nodes = NULL;
	}

	return res;
}

void node_free(Node *node) {
	if (node == NULL) {
		return;
	}

	switch (node->type) {
	case AST_EXPR: {
		Expression *expr = &node->expr;
		for (size_t i = 0; i < expr->len; i++) {
			node_free(expr->nodes[i]);
		}
		break;
	}
	case AST_VAR: {
		free(node->var.name);
		break;
	}
	case AST_STR: {
		free(node->str.str);
		break;
	}
	case AST_COMMENT: {
		free(node->comment.content);
		break;
	}
	case AST_NUM:
		break;
	}

	free(node);
}

Node *node_copy(const Node *src) {
	if (src == NULL) {
		return NULL;
	}

	Node *res = make_node(src->type);

	switch (src->type) {
	case AST_EXPR:
		res->expr.len = src->expr.len;
		res->expr.isquoted = src->expr.isquoted;
		for (size_t i = 0; i < src->expr.len; i++) {
			res->expr.nodes[i] = node_copy(src->expr.nodes[i]);
		}
		break;

	case AST_VAR:
		res->var.name = astrcpy(src->var.name);
		break;

	case AST_STR:
		res->str.size = src->str.size;
		res->str.str = astrcpy(src->str.str);
		break;

	case AST_NUM:
		res->num.val = src->num.val;
		break;

	case AST_COMMENT:
		res->comment.content = astrcpy(src->comment.content);
		break;
	}

	return res;
}
