#include "AST.h"
#include "nodeParse.h"

/*
 * ParamList ::= '(' (Identifier (',' Identifier)*)? ')'
*/
struct ASTLinkedNode *parseParamList()
{
	struct ASTLinkedNode *child = NULL, *ans = newLinkedAstNode(PARAM_LIST);
	// im sorry for unused var clang - we will need it later though!
	accept(LPAR);
	struct Token *next = peek();
	if (next->type == IDENTIFIER) {
		child = parseIdentifier();
		child->val.type = VAR_DECL;
		ans->val.children = child;
		next = peek();
	}
	while (next->type != RPAR) {
		accept(COMMA);
		if (child == NULL) {
			child = parseIdentifier();
			child->val.type = VAR_DECL;
			ans->val.children = child;
		} else {
			child->next = parseIdentifier();
			child->next->val.type = VAR_DECL;
			child = child->next;
		}
		next = peek();
	}
	accept(RPAR);
	return ans;
}