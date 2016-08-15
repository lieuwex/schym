#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "stringify.h"
#include "interpreter/interpreter.h"

char* readfile(const char *fname){
	FILE *f=fopen(fname,"rb");
	if(!f)return NULL;
	if(fseek(f,0,SEEK_END)==-1){fclose(f); return NULL;}
	long flen=ftell(f);
	if(flen==-1){fclose(f); return NULL;}
	rewind(f);

	char *buf=malloc(flen+1);
	if(!buf){fclose(f); return NULL;}
	fread(buf,1,flen,f);
	if(ferror(f)){fclose(f); free(buf); return NULL;}
	if(memchr(buf,'\0',flen)!=NULL){
		fprintf(stderr,"Invalid null char in file '%s'\n",fname);
		exit(1);
	}
	buf[flen]='\0';
	fclose(f);
	return buf;
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
	printf("unended string: err: %s\n", str.err);

	ParseResult list = parse("'( '( 5 \"swag\" ) )");
	printf("res: '( %f \"%s\" )\n", list.node->expr.nodes[0]->expr.nodes[0]->num.val, list.node->expr.nodes[0]->expr.nodes[1]->str.str);

	ParseResult expr = parse("( print 1 )");
	printf("%s(%f)\n", expr.node->expr.nodes[0]->var.name, expr.node->expr.nodes[1]->num.val);
	*/

	if (argc < 2) {
		fprintf(stderr, "usage:\t%s <program code>\n", argv[0]);
		return 1;
	}
	char *src = readfile(argv[1]);

	ProgramParseResult program = parseprogram(src);
	// printf("%zu\n", program.len); // 2
	// printf("%g\n", program.nodes[0]->expr.nodes[1]->num.val); // 3
	// printf("%g\n", program.nodes[1]->expr.nodes[1]->num.val); // 5
	// printf("%s\n", program.err); // (null)
	if(program.err){
		fprintf(stderr,"Program error: %s at line %d col %d\n",program.err,program.errloc.line,program.errloc.col);
		return 1;
	} else if (program.len == 0) {
		fprintf(stderr, "program is empty\n");
		return 1;
	}
	char *str=stringify(program.nodes[0],0);
	printf("src:\n%s\n", src);
	printf("stringify:\n%s\n", str);
	free(str);

	free(src);

	InterState *is=in_make();
	in_run(is,program.nodes[0]);
	in_destroy(is);

	return 0;
}
