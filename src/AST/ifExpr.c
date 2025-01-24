#include "AST.h"
#include "nodeParse.h"

/*
 * IfExpr ::= IF Expr single-command (ELSE single-command)?
*/
struct ASTLinkedNode *parseIfExpr()
{
	struct ASTLinkedNode *ans = newLinkedAstNode(IF_EXPR);
	accept(IF);
	ans->val.children = parseExpr();
	ans->val.children->next = parseSingleCommand();
	if (peek()->type == ELSE) {
		acceptIt();
		ans->val.children->next->next = parseSingleCommand();
	}
	return ans;
}