#include "AST.h"
#include "nodeParse.h"

/*
 * WhileLoop ::= WHILE Expr single-command
*/
struct ASTLinkedNode *parseWhileLoop()
{
	struct ASTLinkedNode *ans = newLinkedAstNode(WHILE_LOOP);
	accept(WHILE);
	ans->val.children = parseExpr();
	ans->val.children->next = parseSingleCommand();
	return ans;
}