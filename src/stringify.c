#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "stringify.h"
#include "util.h"

// #define INDENT "\t"
#define INDENT "  "

static void appendstr(char **str,const char *app){
	int l1=strlen(*str),l2=strlen(app);
	*str=realloc(*str,l1+l2+1,sizeof(char));
	assert(*str);
	memcpy(*str+l1,app,l2+1);
}

char *stringify(const Node *node,int lvl) {
	assert(node);
	char *res=malloc(1,sizeof(char));
	assert(res);
	res[0]='\0';

	switch(node->type){
		case AST_EXPR:
			if(node->expr.isquoted)appendstr(&res,"'");
			appendstr(&res,"(");
			bool didindent=false;
			bool issmall = node->expr.len < 3;
			for(size_t i=0;i<node->expr.len;i++){
				if (i!=0){
					didindent=true;
					if (node->expr.isquoted || issmall) {
						appendstr(&res," ");
					} else {
						appendstr(&res,"\n");
						for(int x=0;x<lvl+1;x++) {
							appendstr(&res,INDENT);
						}
					}
				}
				char *str=stringify(node->expr.nodes[i],lvl+didindent);
				appendstr(&res, str);
				free(str);
			}
			appendstr(&res,")");
			break;

		case AST_VAR:
			appendstr(&res,node->var.name);
			break;

		case AST_STR:
			appendstr(&res,"\"");
			appendstr(&res,node->str.str);
			appendstr(&res,"\"");
			break;

		case AST_NUM:{
			char *buf;
			asprintf(&buf,"%g",node->num.val);
			assert(buf);
			appendstr(&res,buf);
			free(buf);
			break;
		}

		case AST_COMMENT: {
			appendstr(&res, "; ");
			appendstr(&res, node->comment.content);
			break;
		}
	}

	return res;
}
