#include "parse.h"
#include "lex.h"
#include "AST.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	printf("> ");
	struct Token *next;
	struct AST *expr;
	while ((next = peek())->type != TOKEN_EOF) {
		fflush(stdout);
		//printf("%d\n", parseExpr());
		expr = parse();
		printTree(expr);
		freeTree(expr);
		putchar('\n');
		printf("> ");
	}
}
