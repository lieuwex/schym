#pragma once

#include "ast.h"

// REVIEW: make this structure recursive, so that even when I grab some deep
// node I will know its interned?
typedef struct InternedNode {
	Node *node;
} InternedNode;

typedef struct InternEnvironment InternEnvironment;
InternEnvironment *ie_make(void);
void ie_free(InternEnvironment*, bool);

InternedNode intern(const Node *node, InternEnvironment*);
