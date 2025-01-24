#include "AST.h"
#include "nodeParse.h"

/*
 * const-decl ::= CONST Identifier '=' constExpr EOL
*/
struct ASTLinkedNode *parseConstDecl()
{
	struct ASTLinkedNode *ans = newLinkedAstNode(CONST_DECL);

	accept(CONST);
	ans->val.children = parseIdentifier();
	accept(ASSIGN);
	struct ASTLinkedNode *expr = parseExpr();
	accept(LINE_END);

	ans->val.children->next = expr;
	return ans;
}