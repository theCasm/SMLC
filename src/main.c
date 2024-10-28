#include "parse.h"
#include "lex.h"
#include <stdio.h>

int main(void)
{
	printf("> ");
	while (peek()->type != TOKEN_EOF) {
		fflush(stdout);
		printf("%d\n", parse());
		printf("> ");
	}
}
