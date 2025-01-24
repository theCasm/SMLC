#include "AST.h"
#include "nodeParse.h"
#include "../IO/IO.h"

/*
 * fn-decl ::= FUNC (VOID | NON_VOID) Identifier ParamList single-command
*/
struct ASTLinkedNode *parseFunctionDecl()
{
	struct ASTLinkedNode *ans = newLinkedAstNode(FN_DECL);
	accept(FUNC);
	struct Token *next = peek();
	// TODO: specify error better
	if (next->type != VOID && next->type != NON_VOID) {
		return handleUnexpectedToken(next);
	}
	ans->val.isVoid = next->type == VOID;
	acceptIt();
	next = peek();
	ans->val.startIndex = next->start;
	ans->val.endIndex = next->end;
	ans->val.children = parseIdentifier();
	ans->val.children->next = parseParamList();
	ans->val.children->next->next = parseSingleCommand();
	return ans;
}