#include "AST.h"
#include "nodeParse.h"
#include "../IO/IO.h"

/*
 * globalDecl ::= fnDecl | varDecl | constDecl
*/
struct ASTLinkedNode *parseGlobalDecl()
{
	struct Token *next = peek();
	struct ASTLinkedNode *ans = newLinkedAstNode(GLOBAL_DECL);
	switch (next->type) {
	case FUNC:
		ans->val.children = parseFunctionDecl();
		return ans;
	case CONST:
		ans->val.children = parseConstDecl();
		return ans;
	case VAR:
		ans->val.children = parseVarDecl();
		ans->val.children->val.isStatic = 1;
		return ans;
	default:
		// TODO: way better error. this makes no sense when u see it
		return handleUnexpectedToken(next);
	}
}