#include "AST.h"
#include "nodeParse.h"

/*
 * var-decl ::= VAR Identifier ('=' Expr)? EOL
*/
struct ASTLinkedNode *parseVarDecl()
{
	struct ASTLinkedNode *ans = newLinkedAstNode(VAR_DECL);
	accept(VAR);
	ans->val.children = parseIdentifier();
	struct Token *next = peek();
	if (next->type != LINE_END) {
		accept(ASSIGN);
		struct ASTLinkedNode *expr = parseExpr();
		ans->val.children->next = expr;
	}
	accept(LINE_END);
	return ans;
}