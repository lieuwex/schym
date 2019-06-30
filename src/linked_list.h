#pragma once

#include "ast.h"

typedef struct LinkedList LinkedList;

LinkedList *ll_make(void);
LinkedList *ll_copy(const LinkedList*);

Node *ll_pop_front(LinkedList*);
Node *ll_pop_back(LinkedList*);

void ll_push_front(LinkedList*, Node*);
void ll_push_back(LinkedList*, Node*);

Node *ll_front(const LinkedList*);
Node *ll_back(const LinkedList*);

void ll_free(LinkedList*);
