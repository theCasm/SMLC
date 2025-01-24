#include "AST.h"
#include "nodeParse.h"

/*
 * indirectAssignment ::= '*'primaryExpr '=' Expr EOL
*/
struct ASTLinkedNode *parseIndirectAssignment()
{
	struct ASTLinkedNode *ans = newLinkedAstNode(INDIRECT_ASSIGN);
	accept(TIMES);
	ans->val.children = parsePrimaryExpr();
	accept(ASSIGN);
	ans->val.children->next = parseExpr();
	accept(LINE_END);
	return ans;
}