#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "stringify.h"
#include "interpreter/interpreter.h"
#include "intern.h"
#include "util.h"

char* readfile(const char *fname){
	FILE *f = fopen(fname, "rb");
	if (f == NULL) {
		return NULL;
	}
	if (fseek(f, 0, SEEK_END) == -1) {
		fclose(f);
		return NULL;
	}

	long flen = ftell(f);
	if (flen == -1) {
		fclose(f);
		return NULL;
	}
	rewind(f);

	char *buf = malloc(flen + 1, sizeof(char));
	if (buf == NULL) {
		fclose(f);
		return NULL;
	}

	fread(buf, 1, flen, f);
	if (ferror(f)) {
		fclose(f);
		free(buf);
		return NULL;
	}

	if (memchr(buf,'\0',flen) != NULL) {
		fprintf(stderr, "Invalid null char in file '%s'\n", fname);
		exit(1);
	}

	buf[flen] = '\0';
	fclose(f);
	return buf;
}

void printusage(const char *progname) {
	fprintf(stderr, "usage:\t%s [ -e script | file ]\n", progname);
}

int main(int argc, char **argv) {
	char *src = NULL;

#define FLAG(s, l) (streq(argv[i], s) || streq(argv[i], l))
	for (int i = 1; i < argc; i++) {
		if (FLAG("-e", "--eval")) {
			i++;
			src = astrcpy(argv[i]);
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

	InterState *is = in_make();
	RunResult res = in_run(is, program.nodes[0]);
	in_destroy(is);
	InternEnvironment *env = ie_make();
	InternedNode interned = intern(program.nodes[0], env);
	ie_free(env, false);
	if (res.err != NULL) {
		fprintf(stderr, "Error while executing code: %s\n", res.err);
		return 1;
	}

	return 0;
}
