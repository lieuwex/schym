#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "interpreter.h"
#include "../stringify.h"
#include "../util.h"

#define HASHSIZE (127)

#define NOT_IMPLEMENTED false

static int namehash(const char *name){
	int h=123457;
	for(int i=0;name[i];i++)h+=(i+1)*name[i];
	return h%HASHSIZE;
}

typedef struct Var{
	char *name;
	Node *value;
} Var;

typedef struct Varllist Varllist;
struct Varllist{
	Var *v;
	Varllist *next;
};

struct InterState{
	Varllist *gvars[HASHSIZE]; //global variables
	InterState *next; //will be NULL, except if this is an entry in a scope stack
};

void var_destroy(Var *v){
	free(v->name);
	node_free(v->value);
}

void llist_destroy(Varllist *ll){
	while(ll){
		var_destroy(ll->v);
		ll=ll->next;
	}
}

InterState* in_make(void){
	InterState *in=malloc(1,sizeof(InterState));
	assert(in);
	memset(in->gvars,0,HASHSIZE*sizeof(Varllist));
	return in;
}

static void in_destroy_(InterState *is,bool recursive){
	for(int i=0;i<HASHSIZE;i++){
		if(is->gvars[i])llist_destroy(is->gvars[i]);
	}
	if(recursive&&is->next)in_destroy_(is->next,true);
	free(is);
}

void in_destroy(InterState *is){
	in_destroy_(is,false);
}


static Var* find_var(InterState *scope,const char *name){
	int h=namehash(name);
	while(scope){
		Varllist *ll=scope->gvars[h];
		while(ll){
			if(streq(ll->v->name,name))return ll->v;
			ll=ll->next;
		}
		scope=scope->next;
	}
	return NULL;
}

static RunResult rr_null(void){
	RunResult rr={.node=NULL,.err=NULL};
	return rr;
}

static RunResult rr_node(Node *node){
	RunResult rr={.node=node,.err=NULL};
	return rr;
}

static RunResult rr_errf(const char *format,...){
	va_list ap;
	va_start(ap,format);
	char *buf;
	vasprintf(&buf,format,ap);
	va_end(ap);
	assert(buf);
	RunResult rr={.node=NULL,.err=buf};
	return rr;
}

static void rr_free(RunResult rr){
	if(rr.node)node_free(rr.node);
	if(rr.err)free(rr.err);
}


static RunResult builtin_print(InterState *scope,const Node **args,size_t nargs){
	(void)scope;
	for(size_t i=0;i<nargs;i++){
		if(i!=0)putchar(' ');
		char *str=stringify(args[i],0);
		printf("%s",str);
		free(str);
	}
	putchar('\n');
	return rr_null();
}

static double floatmod(double a,double b){
	if(b==0)return nan("");
	int sa=a<0?-1:1;
	a=fabs(a); b=fabs(b);
	return sa*(a-b*floor(a/b));
}

static RunResult builtin_arith(InterState *scope,char func,const Node **args,size_t nargs){
	if(nargs!=2)return rr_errf("Arithmetic operator '%c' expected 2 args, got %lu",func,nargs);
	const Node *a1=args[0],*a2=args[1];
	RunResult r1=in_run(scope,a1);
	if(r1.err)return r1;
	RunResult r2=in_run(scope,a2);
	if(r2.err){
		rr_free(r1);
		return r2;
	}
	Node *x1=r1.node,*x2=r2.node;
	if(x1->type!=AST_NUM||x2->type!=AST_NUM){
		return rr_errf("Arithmetic operator '%c' expected numbers, got %s and %s",
					typetostr(x1->type),typetostr(x2->type));
	}
	double v1=x1->num.val,v2=x2->num.val;
	node_free(x1); node_free(x2);
	Node *res=malloc(1,sizeof(Node));
	assert(res);
	res->type=AST_NUM;
	switch(func){
		case '+': res->num.val=v1+v2; break;
		case '-': res->num.val=v1-v2; break;
		case '*': res->num.val=v1*v2; break;
		case '/': res->num.val=v1/v2; break;
		case '%': res->num.val=floatmod(v1,v2); break;
		default:
			assert(false);
	}
	return rr_node(res);
}


static RunResult in_funccall(InterState *scope,const char *name,const Node **args,size_t nargs){
	if(streq(name,"print"))return builtin_print(scope,args,nargs);
	else if(strlen(name)==0&&strchr("+-*/%",name[0])!=NULL)return builtin_arith(scope,'+',args,nargs);
	else return rr_errf("Unknown function '%s'",name);
}

RunResult in_run(InterState *scope,const Node *node){
	assert(node);
	assert(scope);
	switch(node->type){
		case AST_EXPR:
			if(node->expr.isquoted)return rr_node(node_copy(node));
			if(node->expr.len==0)return rr_errf("Non-quoted expression can't be empty");
			if(node->expr.nodes[0]->type!=AST_VAR){
				return rr_errf("Cannot call non-variable (type %s)",
						typetostr(node->expr.nodes[0]->type));
			}
			return in_funccall(scope,node->expr.nodes[0]->var.name,(const Node**)(node->expr.nodes+1),node->expr.len-1);

		case AST_VAR:{
			Var *v=find_var(scope,node->var.name);
			if(!v)return rr_errf("Variable not found: %s",node->var.name);
			return rr_node(node_copy(v->value));
		}

		case AST_STR:
		case AST_NUM:
			return rr_node(node_copy(node));

		case AST_COMMENT:
			fprintf(stderr,"WARNING: comment passed to in_run()\n");
			return rr_null();

		default:
			assert(false);
	}
}
