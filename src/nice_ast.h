#pragma once

#include "ast.h"
#include "stringify.h"

Node *createNode(ASTtype type, bool quoted);

bool expr_append(Node *expr, Node *node);
bool expr_remove(Node *node, size_t at);
