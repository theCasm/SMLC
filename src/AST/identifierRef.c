#include "AST.h"
#include "nodeParse.h"

/*
 * directAssignment ::= Identifier '=' Expr EOL
 * or
 * functionCall ::= Identifier ArgList EOL
 * together, these are not LL(1), so we need to break it up
*/
struct ASTLinkedNode *parseIdentifierCommand()
{
	struct ASTLinkedNode *child, *ans = newLinkedAstNode(DIRECT_ASSIGN);
	child = parseIdentifier();
	ans->val.children = child;

	struct Token *next = peek();
	if (next->type == LPAR) {
		ans->val.type = FUNC_CALL;
		child->next = parseArgList();
		accept(LINE_END);
		return ans;
	}
	accept(ASSIGN);
	child->next = parseExpr();
	accept(LINE_END);
	return ans;
}

struct ASTLinkedNode *parseIdentifier()
{
	struct Token *next = peek();
	struct ASTLinkedNode *ans = newLinkedAstNode(IDENT_REF);
	ans->val.startIndex = next->start;
	ans->val.endIndex = next->end;
	accept(IDENTIFIER);
	return ans;
}