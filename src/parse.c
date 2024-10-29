#include "lex.h"
#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define MAX_P 10

extern char *fullInput;

int fold(int left, enum TokenType type, int right);
int parsePriority(int);
int parsePrimaryExpr();
int isPriority(enum TokenType type, int priority);
void handleUnexpectedToken(struct Token *tok);

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
}

int parse()
{
	int ans = parsePriority(MAX_P);
	accept(LINE_END);
	return ans;
}

int parsePriority(int priority)
{
	if (priority <= 0) {
		return parsePrimaryExpr();
	}
	int left = parsePriority(priority - 1);
	int right;
	struct Token *next = peek();

	while (isPriority(next->type, priority)) {
		acceptIt();
		right = parsePriority(priority - 1);
		left = fold(left, next->type, right);
		freeToken(next);
		next = peek();
	}
	return left;
}

int parsePrimaryExpr()
{
	struct Token *next = peek();
	int ans;
	switch (next->type) {
	case NUMBER:
		acceptIt();
		char *spelling = calloc(next->end - next->start + 1, sizeof(char));
		strncpy(spelling, fullInput + next->start, next->end - next->start);
		spelling[next->end - next->start] = '\0';
		int base = (spelling[1] == 'x') ? 16 : ((spelling[0] == '0') ? 8 : 10);
		ans = strtol(spelling, NULL, base);
		freeToken(next);
		free(spelling);
		return ans;
	case LPAR:
		acceptIt();
		ans = parsePriority(MAX_P);
		accept(RPAR);
		freeToken(next);
		return ans;
	case MINUS:
		acceptIt();
		freeToken(next);
		return -parsePrimaryExpr();
	case BITWISE_NOT:
		acceptIt();
		freeToken(next);
		return ~parsePrimaryExpr();
	default:
		handleUnexpectedToken(next);
		freeToken(next);
		exit(1);
	}
}

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

void handleUnexpectedToken(struct Token *tok)
{
	char *unexpectedTok = malloc(tok->end - tok->start + 1);
	strncpy(unexpectedTok, fullInput + tok->start, tok->end - tok->start);
	unexpectedTok[tok->end - tok->start] = '\0';
	fprintf(stderr, "Unexpected: `%s`\n", unexpectedTok);
	free(unexpectedTok);
}