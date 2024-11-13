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
 * The role of the contextual analyzer, for our purposes, is to verify:
 *  - constant expressions have static values
 *  - identifier references actually exist / are in scope
 *  - correct number of arguments are used in function calls
 * 
 * And to assign:
 *  - ref pointers to their respective definition
 *  - frame index
 *  - isConstant
 * 
 * The typeless nature of SML means we don't really have that much to check.
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "contextualAnalysis.h"
#include "AST.h"
#include "lex.h"

struct definition {
	size_t startIndex;
	size_t endIndex; 
	struct ASTLinkedNode *def;
};

static struct definition *defStack = NULL;
static size_t defIndex = 0;
static size_t defCap = 0;
static int frameIndex = 0;

static void initDefStack();
static void pushDef(size_t start, size_t end, struct ASTLinkedNode *def);
static void popDef();
static void *searchForDef(size_t identifierStart, size_t identifierEnd);
static void pass1(struct AST *tree);
static void pass2(struct ASTLinkedNode *curr);

/*
 * EFFECTS: invokes analysis functions in order to do complete analysis of given AST. 
 */
struct AST *analyze(struct AST *ast)
{
	// TODO: 2 pass context analysis - pass 1 for global decls, pass 2 for everything else
	// - will allow globals (notably functions) to be defined lower than their first use
	initDefStack();
	pass1(ast);
	pass2(ast->root);
	return ast;
}

/*
 * REQUIRES: initDefStack been called.
 * EFFECTS: finds global function definitions and adds them to stack so they can be found later
*/
static void pass1(struct AST *tree)
{
	// TODO: ensure function names are unique
	struct ASTLinkedNode *child, *globaldec, *root = tree->root;
	struct ASTLinkedNode *ident;
	for (globaldec = root->val.children; globaldec != NULL; globaldec = globaldec->next) {
		child = globaldec->val.children;
		if (child->val.type == FN_DECL) {
			ident = child->val.children;
			pushDef(ident->val.startIndex, ident->val.endIndex, child);
		}
	}
}

static int clobbersReturn = 0;

/*
 * REQUIRES: initDefStack called, pass1 called
 * EFFECTS: does main part of context analysis by verifying:
 *  - constant variable values are statically known
 *  - variable references have a valid, in scope definition
 *  - function calls refer to defined function and use proper argument count
 * 
 * Finally, this function links references to their definition in the AST, and decorates definitions with:
 *  - stack frame depth
 *  - stack frame index
 *  - frameVars of fn
 *  - paramCount of fn
 *  - clobbersReturn
 *  - isParam of var
*/
static void pass2(struct ASTLinkedNode *curr)
{
	if (curr == NULL) return;

	struct ASTLinkedNode *child, *temp;
	struct ASTLinkedNode *ident;
	struct ASTLinkedNode *params;
	struct ASTLinkedNode *singleCommand;
	int oldIndex;
	size_t startDefIndex;
	char *name = NULL;
	switch (curr->val.type) {
	case FN_DECL:
		// TODO: set pointer to string of identifier
		ident = curr->val.children;
		ident->val.definition = 0;
		params = ident->next;
		singleCommand = params->next;
		oldIndex = frameIndex;
		for (frameIndex = 0, child = params->val.children; child != NULL; child = child->next, frameIndex++) {
			name = malloc(child->val.endIndex - child->val.startIndex + 1);
			getInputSubstr(name, child->val.startIndex, child->val.endIndex);
			pushDef(child->val.startIndex, child->val.endIndex, child);
			child->val.frameIndex = frameIndex;
			child->val.isParam = 1;
		}
		curr->val.paramCount = frameIndex;
		clobbersReturn = 0;
		curr->val.clobbersReturn = 0;
		frameIndex = 0;
		pass2(singleCommand);
		if (clobbersReturn) {
			curr->val.clobbersReturn = 1;
		}
		curr->val.frameVars = frameIndex;
		frameIndex = oldIndex;
		for (child = params->val.children; child != NULL; child = child->next) {
			popDef();
		}
		// TODO: ensure ALL functions return. Otherwise it WILL compile and it WILL be weird.
		// TODO: maybe insert a return statement in void functions at the end
		break;
	case CONST_DECL:
		// TODO: set pointer to string of identifier
		// TODO: ensure const names are unique
		ident = curr->val.children;
		pushDef(ident->val.startIndex, ident->val.endIndex, curr);
		pass2(ident->next);
		if (!ident->next->val.isConstant) {
			name = calloc(curr->val.children->val.endIndex - curr->val.children->val.startIndex + 1, sizeof(*name));
			getInputSubstr(name, curr->val.children->val.startIndex, curr->val.children->val.endIndex);
			fprintf(stderr, "Constant values must be statically known, but `%s` is defined to non-statically known expression.\n", name);
			exit(1);
		}
		break;
	case VAR_DECL:
		// TODO: set pointer to string of identifier
		// TODO: ensure var names are unique
		ident = curr->val.children;
		ident->val.definition = NULL;
		pushDef(ident->val.startIndex, ident->val.endIndex, curr);
		curr->val.frameIndex = frameIndex++;
		curr->val.isParam = 0;
		if (ident->next) pass2(ident->next);
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
		clobbersReturn = 1;
		ident = curr->val.children;
		ident->val.definition = searchForDef(ident->val.startIndex, ident->val.endIndex);
		if (!ident->val.definition) {
			name = calloc(ident->val.endIndex - ident->val.startIndex + 1, sizeof(*name));
			getInputSubstr(name, ident->val.startIndex, ident->val.endIndex);
			fprintf(stderr, "Could not find definition of `%s`.\n", name);
			exit(1);
		}
		struct ASTLinkedNode *args = ident->next;
		temp = ident->val.definition->val.children->next;
		for (child = args->val.children; child != NULL; child = child->next, temp = temp->next) {
			if (temp == NULL) {
				fputs("Too many args\n", stderr);
				break;
			}
			pass2(child);
		}
		if (temp != NULL) {
			fputs("Too few args\n", stderr);
		}
		break;
	case EXPR:
		curr->val.isConstant = 1;
		for (child = curr->val.children; child != NULL; child = child->next) {
			pass2(child);
			if (!child->val.isConstant) {
				curr->val.isConstant = 0;
			}
		}
		break;
	case COMMAND:
		startDefIndex = defIndex;
		for (child = curr->val.children; child != NULL; child = child->next) {
			pass2(child);
			// TODO: ensure that return is last part of command, warning otherwise
		}
		while(defIndex > startDefIndex) {
			popDef();
		}
		break;
	case RETURN_DIRECTIVE: 
		//TODO: ensure is in function when this is found
		//TODO: if not last statement in function, throw warning and trim tree to remove everything after.
		//  -> We probably won't do that here - maybe global flag when in function and check for return in command cases?
	default:
		for (child = curr->val.children; child != NULL; child = child->next) {
			pass2(child);
		}
		return;
	}
}

/*
 * REQUIRES: initDefStack has not yet been called
 * EFFECTS: initializes definition stack
*/
static void initDefStack()
{
	defCap = 4;
	defIndex = 0;
	defStack = calloc(4, sizeof(*defStack));
}

/*
 * REQUIRES: initDefStack has been called
 * EFFECTS: pushes definition with given characteristics to stack
*/
static void pushDef(size_t start, size_t end, struct ASTLinkedNode *def)
{
	if (defIndex >= defCap) {
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

/*
 * REQUIRES: initStackDef previously called, definition exists on stack.
 * EFFECTS: Removes last added definition from stack
*/
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

/*
 * REQUIRES: initDefStack been called
 * EFFECTS: produces pointer to identifier definition AST node if it is on stack, or null otherwise.
*/
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
