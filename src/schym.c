#include <stdlib.h>

#include "ast.h"
#include "stringify.h"

static void getpos(const int index, const char *code, int *col, int *line) {
	unsigned int currcol = 1;
	unsigned int currline = 1;
	int currindex = 0;
	while (*code) {
		if (currindex == index) {
			*col = currcol;
			*line = currline;
			return;
		} else if (*code == '\n') {
			currline++;
			currcol = 1;
		} else {
			currcol++;
		}
		code++;
		currindex++;
	}

	// index not found
	*col = -1;
	*line = -1;
}

int main(int argc, char **argv) {
	/*
	ParseResult cmt = parse("; kaas\ntest");
	printf("%s\n", cmt.node->comment.content);

	ParseResult num = parse("420.69");
	printf("%f\n", num.node->num.val);

	num = parse("420.f69");
	printf("%d | '%s'\n", num.node == NULL, num.err);

	num = parse("f420.69");
	printf("%d | '%s'\n", num.node == NULL, num.err);

	ParseResult str = parse("\"kaas is lekker\"");
	printf("%s\n", str.node->str.str);

	str = parse("\"kaas is lekker");
	printf("uneneded string: err: %s\n", str.err);

	ParseResult list = parse("'( '( 5 \"swag\" ) )");
	printf("res: '( %f \"%s\" )\n", list.node->expr.nodes[0]->expr.nodes[0]->num.val, list.node->expr.nodes[0]->expr.nodes[1]->str.str);

	ParseResult expr = parse("( print 1 )");
	printf("%s(%f)\n", expr.node->expr.nodes[0]->var.name, expr.node->expr.nodes[1]->num.val);
	*/

	if (argc < 2) {
		fprintf(stderr, "usage:\t%s <program code>\n", argv[0]);
		return 1;
	}
	const char *src = argv[1];

	ProgramParseResult program = parseprogram(src);
	// printf("%zu\n", program.len); // 2
	// printf("%g\n", program.nodes[0]->expr.nodes[1]->num.val); // 3
	// printf("%g\n", program.nodes[1]->expr.nodes[1]->num.val); // 5
	// printf("%s\n", program.err); // (null)
	if(program.err){
		fprintf(stderr,"Program error: %s\n",program.err);
		return 1;
	} else if (program.len == 0) {
		fprintf(stderr, "program is empty\n");
		return 1;
	}
	char *str=stringify(program.nodes[0],0);
	printf("src:\n%s\n\n", src);
	printf("stringify:\n%s\n", str);
	free(str);

	int col;
	int row;
	getpos(5, "a\nb\nf3", &col, &row);
	printf("(%d, %d)\n", col, row);

	return 0;
}
