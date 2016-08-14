#include "interpreter.h"
#include "../util.h"

Interpreter *Interpreter_make(void) {
	Interpreter *res = malloc(1, sizeof(Interpreter));
	res->vars = varstore_make();
	return res;
}
