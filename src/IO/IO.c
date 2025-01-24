#include "IO.h"
#include "../lex.h"
#include "../AST/AST.h"
#include <stdio.h>
#include <stdlib.h>

struct ASTLinkedNode *handleUnexpectedToken(struct Token *tok)
{
	// TODO: lexer function to turn start, end into string.
	char *unexpectedTok = malloc(tok->end - tok->start + 1);
	getInputSubstr(unexpectedTok, tok->start, tok->end);
	fprintf(stderr, "Unexpected: `%s`\n", unexpectedTok);
	free(unexpectedTok);

	exit(1);
}