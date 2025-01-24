#include "AST.h"
#include "nodeParse.h"
#include "../IO/IO.h"
#include <stdio.h>

/*
 * singleCommand ::= const-decl | var-decl | if-expr
 *                 | while-loop | ('{' Command '}')
 *                 | identifier-command | indirect-assignment
 * 				   | return Expr? EOL
 * 
*/
struct ASTLinkedNode *parseSingleCommand()
{
	struct ASTLinkedNode *ans = newLinkedAstNode(SINGLE_COMMAND);
	struct Token *next = peek();
	switch (next->type) {
	case CONST:
		ans->val.children = parseConstDecl();
		//fprintf(stderr, "%p\n", ans->val.children);
		return ans;
	case VAR:
		ans->val.children = parseVarDecl();
		return ans;
	case IF:
		ans->val.children = parseIfExpr();
		return ans;
	case WHILE:
		ans->val.children = parseWhileLoop();
		return ans;
	case LCPAR:
		acceptIt();
		ans->val.children = parseCommand();
		accept(RCPAR);
		return ans;
	case IDENTIFIER:
		ans->val.children = parseIdentifierCommand();
		return ans;
	case TIMES:
		ans->val.children = parseIndirectAssignment();
		return ans;
	case RETURN:
		// we are treating this different than every other command - return directives, simply put, *are* different.
		acceptIt();
		ans->val.type = RETURN_DIRECTIVE;
		next = peek();
		if (next->type != LINE_END) ans->val.children = parseExpr();
		accept(LINE_END);
		return ans;
	default:
		break;
	}
	puts("singleCommand unexpected");
	return handleUnexpectedToken(next);
}