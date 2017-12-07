#pragma once

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

struct RunResult;
typedef struct RunResult RunResult;
struct InterEnv;
typedef struct InterEnv InterEnv;

typedef enum ASTtype {
	AST_QUOTED,
	AST_EXPR,
	AST_VAR,
	AST_STR,
	AST_NUM,
	AST_COMMENT,

	AST_FUN, // not a real AST node, actually.
} ASTtype;

typedef struct Node Node;

typedef struct Quoted {
	Node *node;
} Quoted;

typedef struct Expression {
	size_t len;
	Node **nodes;
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

typedef struct Function {
	bool isBuiltin;
	union {
	// if isBuiltin:
		RunResult (*fn)(InterEnv*, const char*, size_t, const Node**);
	// else:
		struct {
			Expression args;
			Node *body;
			InterEnv *env;
		};
	};
} Function;

struct Node {
	ASTtype type;
	union {
		Quoted quoted;
		Expression expr;
		Variable var;
		String str;
		Number num;
		Comment comment;
		Function function;
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
