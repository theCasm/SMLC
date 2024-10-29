#include "parse.h"
#include "lex.h"
#include "AST.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	printf("> ");
	struct Token *next;
	struct AST *ast;
	while ((next = peek())->type != TOKEN_EOF) {
		fflush(stdout);
		//printf("%d\n", parse());
		ast = parse();
		printTree(ast);
		freeTree(ast);
		putchar('\n');
		printf("> ");
	}
}
