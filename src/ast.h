#pragma once

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum ASTtype {
	AST_EXPR,
	AST_VAR,
	AST_STR,
	AST_NUM,
	AST_COMMENT,
} ASTtype;

typedef struct Node Node;

typedef struct Expression {
	size_t len;
	Node **nodes;
	bool isquoted;
} Expression;

typedef struct Variable {
	char *name;
} Variable;

typedef struct String {
	size_t size;
	char *str;
} String;

typedef struct Number {
	// this should really use typeclasses for different kind of numbers
	double val;
} Number;

typedef struct Comment {
	char *content;
} Comment;

struct Node {
	int index;
	ASTtype type;
	union {
		Expression expr;
		Variable var;
		String str;
		Number num;
		Comment comment;
	};
};

typedef struct Location {
	int col;
	int line;
	int index;
} Location;

typedef struct ParseResult {
	Node *node;
	const char *err;
	Location errloc;
	const char *_errp;
} ParseResult;

typedef struct ProgramParseResult {
	Node **nodes;
	size_t cap;
	size_t len;
	const char *err;
	Location errloc;
} ProgramParseResult;

ParseResult parse(const char *code);
ProgramParseResult parseprogram(const char *code);
void node_free(Node *node);
Node *node_copy(const Node *node);
