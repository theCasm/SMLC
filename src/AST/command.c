#include "AST.h"
#include "nodeParse.h"

/*
 * Command ::= EOL* (singleCommand EOL*)*
*/
struct ASTLinkedNode *parseCommand()
{
	struct ASTLinkedNode *child = NULL, *ans = newLinkedAstNode(COMMAND);
	struct Token *next = peek();
	while (next && next->type == LINE_END) {
		acceptIt();
		next = peek();
	}
	while (next->type == CONST || next->type == VAR || next->type == IF || next->type == WHILE || next->type == IDENTIFIER || next->type == TIMES
			|| next->type == RETURN) {
		if (child == NULL) {
			child = parseSingleCommand();
			ans->val.children = child;
		} else {
			child->next = parseSingleCommand();
			child = child->next;
		}
		next = peek();
		while (next->type == LINE_END) {
			acceptIt();
			next = peek();
		}
	}
	return ans;
}