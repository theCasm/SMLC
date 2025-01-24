#include "AST.h"
#include "nodeParse.h"

/*
 * ArgList ::= '(' (Expr (',' Expr)*)? ')'
*/
struct ASTLinkedNode *parseArgList()
{
	struct ASTLinkedNode *child = NULL, *ans = newLinkedAstNode(ARG_LIST);
	accept(LPAR);
	struct Token *next = peek();
	if (next->type != RPAR) {
		if (child == NULL) {
			child = parseExpr();
			ans->val.children = child;
		} else {
			child->next = parseExpr();
			child = child->next;
		}
		next = peek();
	}
	while (next->type != RPAR) {
		accept(COMMA);
		child->next = parseExpr();
		child = child->next;
		next = peek();
	}
	accept(RPAR);
	return ans;
}