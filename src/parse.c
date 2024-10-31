#include "lex.h"
#include "parse.h"
#include "AST.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define MAX_P 10


/*
 * For now, identifiers are ignored and treated like unimportant terminals. In the future,
 * we will need to treat them similarily to numbers, in that they are a terminal that needs to
 * be parsed further. This is why some of the 'accept(IDENTIFIERS)' are structured in the way that they
 * are.
*/

extern char *fullInput;

static struct ASTLinkedNode *foldExpr(struct ASTLinkedNode *left, enum TokenType type, struct ASTLinkedNode *right);
static struct ASTLinkedNode *parseProgram();
static struct ASTLinkedNode *parseGlobalDecl();
static struct ASTLinkedNode *parseCommand();
struct ASTLinkedNode *parseSingleCommand();
static struct ASTLinkedNode *parseFunctionDecl();
static struct ASTLinkedNode *parseParamList();
static struct ASTLinkedNode *parseArgList();
static struct ASTLinkedNode *parseIfExpr();
static struct ASTLinkedNode *parseWhileLoop();
static struct ASTLinkedNode *parseConstDecl();
static struct ASTLinkedNode *parseVarDecl();
static struct ASTLinkedNode *parseIdentifierCommand();
static struct ASTLinkedNode *parseIndirectAssignment();
static struct ASTLinkedNode *parsePriority(int);
static struct ASTLinkedNode *parseExpr();
static struct ASTLinkedNode *parsePrimaryExpr();
static int isPriority(enum TokenType type, int priority);
static struct ASTLinkedNode *handleUnexpectedToken(struct Token *tok);

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
 * Takes two expressions and an operator and combines them into one operation appropriately.
*/
static struct ASTLinkedNode *foldExpr(struct ASTLinkedNode *left, enum TokenType type, struct ASTLinkedNode *right)
{
	struct ASTLinkedNode *ans = newLinkedAstNode(EXPR);
	ans->val.children = left;
	left->next = right;
	// hopefully right->next is NULL!
	ans->val.operationType = type;
	ans->val.isConstant = left->val.isConstant && right->val.isConstant;
	return ans;
}

/*
 * Does the thing.
 *
 * exists to hide all the messy wiring actually involved in parsing.
*/
struct AST *parse()
{
	struct ASTLinkedNode *head = parseProgram();
	struct AST *ans = malloc(sizeof(*ans));
	ans->root = head;
	return ans;
}

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

/*
 * globalDecl ::= fnDecl | varDecl | constDecl
*/
static struct ASTLinkedNode *parseGlobalDecl()
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
		return ans;
	default:
		// TODO: way better error. this makes no sense when u see it
		return handleUnexpectedToken(next);
	}
}

/*
 * Command ::= EOL* (singleCommand EOL*)*
*/
static struct ASTLinkedNode *parseCommand()
{
	struct ASTLinkedNode *child = NULL, *ans = newLinkedAstNode(COMMAND);
	struct Token *next = peek();
	while (next && next->type == LINE_END) {
		acceptIt();
		next = peek();
	}
	while (next->type == CONST || next->type == VAR || next->type == IF || next->type == WHILE || next->type == IDENTIFIER || next->type == TIMES) {
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

/*
 * singleCommand ::= const-decl | var-decl | if-expr
 *                 | while-loop | ('{' Command '}')
 *                 | identifier-command | indirect-assignment
 * 
*/
struct ASTLinkedNode *parseSingleCommand()
{
	struct ASTLinkedNode *ans = newLinkedAstNode(SINGLE_COMMAND);
	struct Token *next = peek();
	switch (next->type) {
	case CONST:
		ans->val.children = parseConstDecl();
		ans->val.isConstant = 1;
		//fprintf(stderr, "%p\n", ans->val.children);
		return ans;
	case VAR:
		ans->val.children = parseVarDecl();
		ans->val.isConstant = ans->val.children->val.isConstant;
		return ans;
	case IF:
		ans->val.children = parseIfExpr();
		ans->val.isConstant = ans->val.children->val.isConstant;
		return ans;
	case WHILE:
		ans->val.children = parseWhileLoop();
		ans->val.isConstant = ans->val.children->val.isConstant;
		return ans;
	case LCPAR:
		acceptIt();
		ans->val.children = parseCommand();
		accept(RCPAR);
		return ans;
	case IDENTIFIER:
		ans->val.children = parseIdentifierCommand();
		return ans;
		
	case TIMES:
		ans->val.children = parseIndirectAssignment();
		return ans;
	default:
		break;
	}
	puts("singleCommand unexpected");
	return handleUnexpectedToken(next);
}

/*
 * fn-decl ::= FUNC (VOID | NON_VOID) Identifier ParamList single-command
*/
static struct ASTLinkedNode *parseFunctionDecl()
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
	// TODO: care about the identifier. probably make parser function to turn it into something useful.
	accept(IDENTIFIER);
	ans->val.children = parseParamList();
	ans->val.children->next = parseSingleCommand();
	return ans;
}

/*
 * ArgList ::= '(' (Expr (',' Expr)*)? ')'
*/
static struct ASTLinkedNode *parseArgList()
{
	struct ASTLinkedNode *child = NULL, *ans = newLinkedAstNode(ARG_LIST);
	// im sorry for unused var clang - we will need it later though!
	accept(LPAR);
	struct Token *next = peek();
	if (next->type != RPAR) {
		if (child == NULL) {
			child = parseExpr();
			ans->val.children = child;
		} else {
			child->next = parseExpr();
			child = child->next;
		}
		next = peek();
	}
	while (next->type != RPAR) {
		accept(COMMA);
		child->next = parseExpr();
		child = child->next;
		next = peek();
	}
	accept(RPAR);
	return ans;
}

/*
 * ParamList ::= '(' (Identifier (',' Identifier)*)? ')'
*/
static struct ASTLinkedNode *parseParamList()
{
	struct ASTLinkedNode *child, *ans = newLinkedAstNode(PARAM_LIST);
	// im sorry for unused var clang - we will need it later though!
	accept(LPAR);
	struct Token *next = peek();
	if (next->type == IDENTIFIER) {
		acceptIt();
		next = peek();
	}
	while (next->type != RPAR) {
		accept(COMMA);
		accept(IDENTIFIER);
		next = peek();
	}
	accept(RPAR);
	return ans;
}

/*
 * IfExpr ::= IF Expr single-command (ELSE single-command)?
*/
static struct ASTLinkedNode *parseIfExpr()
{
	struct ASTLinkedNode *ans = newLinkedAstNode(IF_EXPR);
	accept(IF);
	ans->val.children = parseExpr();
	ans->val.children->next = parseSingleCommand();
	if (peek()->type == ELSE) {
		acceptIt();
		ans->val.children->next->next = parseSingleCommand();
	}
	return ans;
}

/*
 * WhileLoop ::= WHILE Expr single-command
*/
static struct ASTLinkedNode *parseWhileLoop()
{
	struct ASTLinkedNode *ans = newLinkedAstNode(WHILE_LOOP);
	accept(WHILE);
	ans->val.children = parseExpr();
	ans->val.children->next = parseSingleCommand();
	return ans;
}

/*
 * const-decl ::= CONST Identifier '=' constExpr EOL
*/
static struct ASTLinkedNode *parseConstDecl()
{
	accept(CONST);
	accept(IDENTIFIER);
	accept(ASSIGN);
	struct ASTLinkedNode *expr = parseExpr();
	accept(LINE_END);
	if (!expr->val.isConstant) {
		// no! bad! bad programmer!
		// TODO: actually include declaration that was bad.
		fputs("constant variable declared with non-constant expression :(\n", stderr);
		exit(1);
	}
	struct ASTLinkedNode *ans = newLinkedAstNode(CONST_DECL);
	ans->val.children = expr;
	ans->val.isConstant = 1;
	return ans;
}

/*
 * var-decl ::= VAR Identifier ('=' Expr)? EOL
*/
static struct ASTLinkedNode *parseVarDecl()
{
	struct ASTLinkedNode *ans = newLinkedAstNode(VAR_DECL);
	ans->val.isConstant = 0;
	accept(VAR);
	accept(IDENTIFIER);
	struct Token *next = peek();
	if (next->type != LINE_END) {
		accept(ASSIGN);
		struct ASTLinkedNode *expr = parseExpr();
		ans->val.children = expr;
		ans->val.isConstant = ans->val.children->val.isConstant;
	}
	accept(LINE_END);
	return ans;
}

/*
 * directAssignment ::= Identifier '=' Expr EOL
 * or
 * functionCall ::= Identifier ArgList EOL
 * together, these are not LL(1), so we need to break it up
*/
static struct ASTLinkedNode *parseIdentifierCommand()
{
	struct ASTLinkedNode *ans = newLinkedAstNode(DIRECT_ASSIGN);
	accept(IDENTIFIER); // TODO: something with this
	struct Token *next = peek();
	if (next->type == LPAR) {
		ans->val.type = FUNC_CALL;
		ans->val.children = parseArgList();
		accept(LINE_END);
		return ans;
	}
	accept(ASSIGN);
	ans->val.children = parseExpr();
	accept(LINE_END);
	return ans;
}

/*
 * indirectAssignment ::= '*'primaryExpr '=' Expr EOL
*/
static struct ASTLinkedNode *parseIndirectAssignment()
{
	struct ASTLinkedNode *ans = newLinkedAstNode(INDIRECT_ASSIGN);
	accept(TIMES);
	ans->val.children = parsePrimaryExpr();
	accept(ASSIGN);
	ans->val.children->next = parseExpr();
	accept(LINE_END);
	return ans;
}

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
static struct ASTLinkedNode *parsePriority(int priority)
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
static struct ASTLinkedNode *parsePrimaryExpr()
{
	struct Token *next = peek();
	struct ASTLinkedNode *ans;
	char *spelling;
	switch (next->type) {
	case NUMBER:
		spelling = calloc(next->end - next->start + 1, sizeof(char));
		strncpy(spelling, fullInput + next->start, next->end - next->start);
		spelling[next->end - next->start] = '\0';
		int base = (spelling[1] == 'x') ? 16 : ((spelling[0] == '0') ? 8 : 10);
		ans = newLinkedAstNode(NUMBER_LITERAL);
		ans->val.val = strtol(spelling, NULL, base);
		ans->val.isConstant = 1;
		free(spelling);
		acceptIt();
		return ans;
	case IDENTIFIER:
		// some kind of variable or smthn
		// TODO:
		// at this point, we should check if the identifier is a function - for now, we will assume based on context
		acceptIt();
		next = peek();
		if (next->type == LPAR) {
			ans = newLinkedAstNode(FUNC_CALL);
			ans->val.children = parseArgList();
			// this is a memory leak, btw. Im not fixing it so that way I cant ignore the TODO
			// we will need to use the result in some way. perhaps make a function call node with these as its children.
		}
		// VERY VERY TODO
		ans = newLinkedAstNode(NUMBER_LITERAL);
		ans->val.val = 0;
		ans->val.isConstant = 0;
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
		ans->val.isConstant = ans->val.children->val.isConstant;
		return ans;
	case BITWISE_NOT:
	case NOT:
		ans = newLinkedAstNode(EXPR);
		ans->val.operationType = next->type;
		acceptIt();
		ans->val.children = parsePrimaryExpr();
		ans->val.isConstant = ans->val.children->val.isConstant;
		return ans;
	case TIMES:
		ans = newLinkedAstNode(EXPR);
		ans->val.operationType = DEREF;
		acceptIt();
		ans->val.children = parsePrimaryExpr();
		ans->val.isConstant = 0;
		return ans;
	default:
		puts("primary unexpected");
		handleUnexpectedToken(next);
		exit(1);
	}
}

/*
 * small priority is done *first*! think "first priority". big priority *last*!
 * we work up this table from bottom to top when evaluating expressions.
*/
int isPriority(enum TokenType type, int priority)
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

static struct ASTLinkedNode *handleUnexpectedToken(struct Token *tok)
{
	// TODO: lexer function to turn start, end into string.
	char *unexpectedTok = malloc(tok->end - tok->start + 1);
	strncpy(unexpectedTok, fullInput + tok->start, tok->end - tok->start);
	unexpectedTok[tok->end - tok->start] = '\0';
	fprintf(stderr, "Unexpected: `%s`\n", unexpectedTok);
	free(unexpectedTok);

	exit(1);
}