#pragma once

#include "varstore.h"

typedef struct Interpreter {
	Varstore *vars;
} Interpreter;
Interpreter *interpreter_make(void);
