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
 * 
 * 
 * The parser is responsbile for everything syntax. This includes:
 *  - building the AST
 *  - Assigning isStatic when necessary
*/
#include "lex.h"
#include "parse.h"
#include "AST/nodeParse.h"
#include "AST/AST.h"
#include <stdlib.h>

/*
 * Does the thing.
 *
 * exists to hide all the messy wiring actually involved in parsing.
*/
struct AST *parse()
{
	struct ASTLinkedNode *head = parseProgram();
	struct AST *ans = malloc(sizeof(*ans));
	ans->root = head;
	return ans;
}