/*
 * Copyright 2024 Aidan Undheim
 *
 * This file is part of SMLC.
 * 
 * SMLC is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 * 
 * SMLC is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * SMLC. If not, see <https://www.gnu.org/licenses/>. 
*/
#include "parse.h"
#include "lex.h"
#include "AST.h"
#include "contextualAnalysis.h"
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
		expr = analyze(parse());
		printTree(expr);
		freeTree(expr);
		putchar('\n');
		printf("> ");
	}
}
