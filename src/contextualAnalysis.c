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
 * The role of the contextual analyzer, for our purposes, is:
 *  - confirm constant declarations are actually static
 *  - ensure identifier references actually exist / are in scope
 *  - verify that correct number of arguments are used in function calls
 *  - link up identifier references to their respective definition
 * 
 * The typeless nature of SML means we don't really have that much to check.
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "contextualAnalysis.h"
#include "AST.h"
#include "lex.h"

static struct definition {
	size_t startIndex;
	size_t endIndex; 
	struct ASTLinkedNode *def;
};

static struct definition *defStack = NULL;
static size_t defIndex = 0;
static size_t defCap = 0;

static void pushDef(size_t start, size_t end, struct ASTLinkedNode *def)
{
	if (defCap == 0) {
		defCap = 4;
		defIndex = 0;
		defStack = calloc(4, sizeof(*defStack));
	} else if (defIndex >= defCap) {
		defCap *= 2;
		if (!(defStack = realloc(defStack, sizeof(*defStack) * defCap))) {
			exit(1);
		}
	}
	defStack[defIndex].startIndex = start;
	defStack[defIndex].endIndex = end;
	defStack[defIndex].def = def;
	++defIndex;
}

static void popDef()
{
	defIndex--;
	if (defIndex * 2 < defCap) {
		defCap /= 2;
		if (!(defStack = realloc(defStack, sizeof(*defStack) * defCap))) {
			exit(1);
		}
	}
}

static void *searchForDef(size_t identifierStart, size_t identifierEnd)
{
	if (defIndex == 0) return NULL;
	for (size_t i = defIndex; i > 0; i--) {
		if (compareInputSubstr(identifierStart, identifierEnd, defStack[i - 1].startIndex, defStack[i - 1].endIndex)) {
			return defStack[i - 1].def;
		}
	}
	return NULL;
}

static void analyzeNext(struct ASTLinkedNode *curr)
{
	if (curr == NULL) return;

	struct ASTLinkedNode *child, *temp;
	struct ASTLinkedNode *ident;
	struct ASTLinkedNode *params;
	struct ASTLinkedNode *singleCommand;
	size_t startDefIndex;
	char *name = NULL;
	switch (curr->val.type) {
	case FN_DECL:
		ident = curr->val.children;
		params = ident->next;
		singleCommand = params->next;
		pushDef(ident->val.startIndex, ident->val.endIndex, curr);
		for (child = params->val.children; child != NULL; child = child->next) {
			name = malloc(child->val.endIndex - child->val.startIndex + 1);
			getInputSubstr(name, child->val.startIndex, child->val.endIndex);
			pushDef(child->val.startIndex, child->val.endIndex, params);
		}
		analyzeNext(singleCommand);
		for (child = params->val.children; child != NULL; child = child->next) {
			popDef();
		}
		// TODO: ensure non-void fn returns
		break;
	case CONST_DECL:
		ident = curr->val.children;
		pushDef(ident->val.startIndex, ident->val.endIndex, curr);
		analyzeNext(ident->next);
		if (!ident->next->val.isConstant) {
			name = calloc(curr->val.children->val.endIndex - curr->val.children->val.startIndex + 1, sizeof(*name));
			getInputSubstr(name, curr->val.children->val.startIndex, curr->val.children->val.endIndex);
			fprintf(stderr, "Constant values must be statically known, but `%s` is defined to non-statically known expression.\n", name);
			exit(1);
		}
		// TODO: compile-time evaluate this expression, and set curr->val appropriately
		break;
	case VAR_DECL:
		ident = curr->val.children;
		pushDef(ident->val.startIndex, ident->val.endIndex, curr);
		if (ident->next) analyzeNext(ident->next);
		break;
	case IDENT_REF:
		curr->val.definition = searchForDef(curr->val.startIndex, curr->val.endIndex);
		if (!curr->val.definition) {
			name = calloc(curr->val.endIndex - curr->val.startIndex + 1, sizeof(*name));
			getInputSubstr(name, curr->val.startIndex, curr->val.endIndex);
			fprintf(stderr, "Could not find definition of `%s`.\n", name);
			exit(1);
		}
		break;
	case FUNC_CALL:
		ident = curr->val.children;
		curr->val.definition = searchForDef(ident->val.startIndex, ident->val.endIndex);
		if (!curr->val.definition) {
			name = calloc(ident->val.endIndex - ident->val.startIndex + 1, sizeof(*name));
			getInputSubstr(name, ident->val.startIndex, ident->val.endIndex);
			fprintf(stderr, "Could not find definition of `%s`.\n", name);
			exit(1);
		}
		struct ASTLinkedNode *args = ident->next;
		temp = curr->val.definition->val.children->next;
		for (child = args->val.children; child != NULL; child = child->next, temp = temp->next) {
			if (temp == NULL) {
				fputs("Too many args\n", stderr);
				break;
			}
			analyzeNext(child);
		}
		if (temp != NULL) {
			fputs("Too few args\n", stderr);
		}
		break;
	case CONST_EXPR:
		for (child = curr->val.children; child != NULL; child = child->next) {
			analyzeNext(child);
			if (!child->val.isConstant) {
				fputs("Constant expr is not constant", stderr);
				exit(1);
			}
		}
		curr->val.isConstant = 1;
		break;
	case EXPR:
		curr->val.isConstant = 1;
		for (child = curr->val.children; child != NULL; child = child->next) {
			analyzeNext(child);
			if (!child->val.isConstant) {
				curr->val.isConstant = 0;
			}
		}
		break;
	case COMMAND:
		startDefIndex = defIndex;
		for (child = curr->val.children; child != NULL; child = child->next) {
			analyzeNext(child);
			// TODO: ensure that return is last part of command, warning otherwise
		}
		while(defIndex > startDefIndex) {
			popDef();
		}
		break;
	case RETURN_DIRECTIVE: //TODO: ensure is in function when this is found
	default:
		for (child = curr->val.children; child != NULL; child = child->next) {
			analyzeNext(child);
		}
		return;
	}
}

struct AST *analyze(struct AST *ast)
{
	// TODO: 2 pass context analysis - pass 1 for global decls, pass 2 for everything else
	// - will allow globals (notably functions) to be defined lower than their first use
	analyzeNext(ast->root);
	return ast;
}