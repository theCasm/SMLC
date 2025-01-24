#include "AST.h"
#include "nodeParse.h"

/*
 * program ::= globalDecl (globalDecl | EOL)*
*/
struct ASTLinkedNode *parseProgram()
{
	struct ASTLinkedNode *ans = newLinkedAstNode(PROGRAM);
	struct ASTLinkedNode *first = parseGlobalDecl();
	ans->val.children = first;
	struct Token *next = peek();

	while (next->type == CONST || next->type == VAR || next->type == FUNC || next->type == LINE_END) {
		if (next->type == LINE_END) {
			acceptIt();
			next = peek();
			continue;
		}
		first->next = parseGlobalDecl();
		first = first->next;
		next = peek();
	}
	return ans;
}