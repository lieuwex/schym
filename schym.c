#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "src/ast.h"
#include "src/stringify.h"
#include "src/interpreter/interpreter.h"
#include "src/intern.h"
#include "src/util.h"

void printusage(const char *progname) {
	fprintf(stderr, "USAGE:\t%s [-f] [ -e script | file ]\n\n", progname);

	fprintf(stderr, "FLAGS:\n");
	fprintf(stderr, "\t-f\tformat the given file\n");
}

int main(int argc, char **argv) {
	char *src = NULL;
	bool format = false;

#define FLAG(s, l) (!skip && (streq(argv[i], s) || streq(argv[i], l)))
	bool skip = false;
	for (int i = 1; i < argc; i++) {
		if (streq(argv[i], "--") && !skip) {
			skip = true;
			continue;
		}

		if (FLAG("-e", "--eval")) {
			i++;
			src = astrcpy(argv[i]);
		} else if (FLAG("-f", "--format")) {
			format = true;
		} else if (FLAG("-h", "--help")) {
			printusage(argv[0]);
			return 0;
		} else {
			src = readfile(argv[i]);
			if (src == NULL) {
				fprintf(stderr, "couldn't read file: '%s'\n", argv[i]);
				return 1;
			}
		}
	}
#undef FLAG

	if (src == NULL) {
		printusage(argv[0]);
		return 1;
	}

	ProgramParseResult program = parseprogram(src);
	if (program.err) {
		fprintf(stderr, "Program error: %s at line %d col %d\n", program.err, program.errloc.line, program.errloc.col);
		return 1;
	} else if (program.len == 0) {
		fprintf(stderr, "Program is empty\n");
		return 1;
	}

	free(src);

	if (format) {
		for (size_t i = 0; i < program.len; i++) {
			printf("%s\n", stringify(program.nodes[i], 0));
		}
		return 0;
	}

	Scope *scope = scope_make(NULL, true);
	InternEnvironment *env = ie_make(); // TODO: free
	for (size_t i = 0; i < program.len; i++) {
		if (program.nodes[i]->type != AST_EXPR) {
			continue;
		}

		InternedNode interned = intern(program.nodes[i], env);
		RunResult res = in_run(scope, interned);
		node_free(interned.node);
		//in_destroy(is);
		if (res.err != NULL) {
			fprintf(stderr, "Error while executing code: %s\n", res.err);
			return 1;
		}
	}
	ie_free(env, false);

	return 0;
}
