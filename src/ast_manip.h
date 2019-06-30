#pragma once

#include "ast.h"
#include "stringify.h"

Node *createNode(ASTtype type, bool quoted);

bool expr_append(Node *expr, const Node *node);
bool expr_remove(Node *expr, size_t at);

Node *array_to_list(const Node**, size_t);
