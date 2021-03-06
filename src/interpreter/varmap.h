#pragma once

#include "../ast.h"

typedef struct VarMap VarMap;

VarMap *varmap_make(void);
Node *varmap_getItem(VarMap*, const char*);
void varmap_setItem(VarMap*, const char*, const Node*);
void varmap_removeItem(VarMap*, const char*);
VarMap *varmap_copy(const VarMap*);
void varmap_print(const VarMap*);
void varmap_free(VarMap*);
