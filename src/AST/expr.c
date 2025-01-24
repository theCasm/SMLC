#include "AST.h"
#include "nodeParse.h"
#include "../IO/IO.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_P 10

static struct ASTLinkedNode *foldExpr(struct ASTLinkedNode *left, enum TokenType type, struct ASTLinkedNode *right);
static int isPriority(enum TokenType type, int priority);
extern char *fullInput;

static void codegenMinus(int left, int right);
static void codegenDivide(int left, int right);
static void codegenModulus(int left, int right);
static void codegenLeftShift(int left, int right);
static void codegenRightShift(int left, int right);
static void codegenNotEquals(int left, int right);
static void codegenOr(int left, int right);
static void codegenAnd(int left, int right);
static void codegenDynamicMultiplication(int left, int right);

/*
 * We will need this for precomputation later!
 * 
int fold(int left, enum TokenType type, int right)
{
	switch (type) {
	case PLUS:
		return left + right;
	case MINUS:
		return left - right;
	case TIMES:
		return left * right;
	case DIVIDE:
		return left / right;
	case MODULO:
		return left % right;
	case LEFT_SHIFT:
		return left << right;
	case RIGHT_SHIFT:
		return left >> right;
	case LESS_THAN:
		return left < right;
	case LESS_THAN_EQUALS:
		return left <= right;
	case GREATER_THAN:
		return left > right;
	case GREATER_THAN_EQUALS:
		return left >= right;
	case EQUALS:
		return left == right;
	case NOT_EQUALS:
		return left != right;
	case OR:
		return left || right;
	case AND:
		return left && right;
	case BITWISE_AND:
		return left & right;
	case BITWISE_OR:
		return left | right;
	case BITWISE_XOR:
		return left ^ right;
	default:
		fprintf(stderr, "idk how to fold in %s\n", TokenStrings[type]);
		return left;
	}
}*/

/*
 * parsePriority does the actual expr parsing - small priority means "priority one" - done first.
 * Big priority done last.
*/
struct ASTLinkedNode *parseExpr()
{
	return parsePriority(MAX_P);
}

/*
 * Expr(p) ::= Expr(p - 1) (Operator(p) Expr(p - 1))*
*/
struct ASTLinkedNode *parsePriority(int priority)
{
	if (priority <= 0) {
		return parsePrimaryExpr();
	}
	struct ASTLinkedNode *right, *left = parsePriority(priority - 1);
	struct Token *next = peek();

	while (isPriority(next->type, priority)) {
		enum TokenType operationType = next->type;
		acceptIt();
		right = parsePriority(priority - 1);
		left = foldExpr(left, operationType, right);
		next = peek();
	}
	return left;
}


/*
 * primaryExpr ::= Number | Identifier | '(' Expr ')' | '-'primaryExpr
 				| '~'primaryExpr | '!'primaryExpr | '*'primaryExpr
*/
struct ASTLinkedNode *parsePrimaryExpr()
{
	struct Token *next = peek();
	struct ASTLinkedNode *ans, *temp;
	char *spelling;
	switch (next->type) {
	case NUMBER:
        // TODO: use lexers thing for this
		spelling = calloc(next->end - next->start + 1, sizeof(char));
		strncpy(spelling, fullInput + next->start, next->end - next->start);
		spelling[next->end - next->start] = '\0';
        int base = 10;
        if (next->end - next->start > 1) base = (spelling[1] == 'x') ? 16 : ((spelling[0] == '0') ? 8 : 10);
		ans = newLinkedAstNode(NUMBER_LITERAL);
		ans->val.val = strtol(spelling, NULL, base);
		ans->val.isConstant = 1;
		free(spelling);
		acceptIt();
		return ans;
	case IDENTIFIER:
		ans = parseIdentifier();
		next = peek();
		if (next->type == LPAR) {
			temp = newLinkedAstNode(FUNC_CALL);
			temp->val.children = ans;
			temp->val.children->next = parseArgList();
			return temp;
		}
		return ans;
	case LPAR:
		acceptIt();
		ans = parseExpr();
		accept(RPAR);
		return ans;
	case MINUS:
		acceptIt();
		ans = newLinkedAstNode(EXPR);
		ans->val.operationType = NEGATE;
		ans->val.children = parsePrimaryExpr();
		return ans;
	case BITWISE_NOT:
	case NOT:
		ans = newLinkedAstNode(EXPR);
		ans->val.operationType = next->type;
		acceptIt();
		ans->val.children = parsePrimaryExpr();
		return ans;
	case TIMES:
		ans = newLinkedAstNode(EXPR);
		ans->val.operationType = DEREF;
		acceptIt();
		ans->val.children = parsePrimaryExpr();
		return ans;
	default:
		puts("primary unexpected");
		handleUnexpectedToken(next);
		exit(1);
	}
}

/*
 * small priority is done *first*! think "first priority".
 * we work up this table from bottom to top when evaluating expressions.
*/
static int isPriority(enum TokenType type, int priority)
{
	if (!isInfix(type)) {
		return 0;
	}
	switch (priority) {
	case 10:
		return type == OR;
	case 9:
		return type == AND;
	case 8:
		return type == BITWISE_OR;
	case 7:
		return type == BITWISE_XOR;
	case 6:
		return type == BITWISE_AND;
	case 5:
		return type == EQUALS || type == NOT_EQUALS;
	case 4:
		return type == LESS_THAN || type == LESS_THAN_EQUALS || type == GREATER_THAN || type == GREATER_THAN_EQUALS;
	case 3:
		return type == LEFT_SHIFT || type == RIGHT_SHIFT;
	case 2:
		return type == PLUS || type == MINUS;
	case 1:
		return type == TIMES || type == DIVIDE || type == MODULO;
	default:
		return 0;
	}
}

/*
 * Takes two expressions and an operator and combines them into one operation appropriately.
*/
static struct ASTLinkedNode *foldExpr(struct ASTLinkedNode *left, enum TokenType type, struct ASTLinkedNode *right)
{
	struct ASTLinkedNode *ans = newLinkedAstNode(EXPR);
	ans->val.children = left;
	left->next = right;
	// hopefully right->next is NULL!
	ans->val.operationType = type;
	return ans;
}