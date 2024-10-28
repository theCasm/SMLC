#include "lex.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

static struct Token *next = NULL;
struct Token *searchForNext(void);
struct Token *lexRestNumber(char);
struct Token *lexRestPrefix(char);

void freeToken(struct Token *token)
{
	if (token->type == NUMBER) {
		free(token->spelling);
	}
	free(token);
}

int isInfix(enum TokenType type)
{
	return PLUS <= type && type <= BITWISE_XOR;
}

struct Token *peek()
{
	if (next == NULL) {
		next = searchForNext();
	}
	return next;
}

void acceptIt()
{
	next = NULL;
}

void accept(enum TokenType type)
{
	struct Token *next = peek();
	if (next->type != type) {
		fprintf(stderr, "Expected `%s` but got `%s`\n", TokenStrings[type], next->spelling);
	}
	freeToken(next);
	acceptIt();
}

int char1, char2;

struct Token *searchForNext()
{
	int nextChar;
	struct Token *ans = malloc(sizeof(struct Token));
	while ((nextChar = getc(stdin)) == ' ' || nextChar == '\t');
	if (nextChar == EOF) {
		ans->type = TOKEN_EOF;
		ans->spelling = "EOF";
		return ans;
	}
	if (nextChar == '\n') {
		ans->type = LINE_END;
		ans->spelling = "\\n";
		return ans;
	}
	switch (nextChar) {
	case '.':
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		free(ans);
		return lexRestNumber(nextChar);
	case '(':
		ans->type = LPAR;
		ans->spelling = "(";
		return ans;
	case ')':
		ans->type = RPAR;
		ans->spelling = ")";
		return ans;
	case '-':
		ans->type = MINUS;
		ans->spelling = "-";
		return ans;
	case '~':
		ans->type = BITWISE_NOT;
		ans->spelling = "~";
		return ans;
	case '+':
		ans->type = PLUS;
		ans->spelling = "+";
		return ans;
	case '*':
		ans->type = TIMES;
		ans->spelling = "*";
		return ans;
	case '/':
		ans->type = DIVIDE;
		ans->spelling = "/";
		return ans;
	case '%':
		ans->type = MODULO;
		ans->spelling = "%";
		return ans;
	case '<':
		if ((nextChar = getc(stdin)) == '<') {
			ans->type = LEFT_SHIFT;
			ans->spelling = "<<";
		} else if (nextChar == '=') {
			ans->type = LESS_THAN_EQUALS;
			ans->spelling = "<=";
		} else {
			ans->type = LESS_THAN;
			ans->spelling = "<";
			ungetc(nextChar, stdin);
		}
		return ans;
	case '>':
		if ((nextChar = getc(stdin)) == '>') {
			ans->type = RIGHT_SHIFT;
			ans->spelling = ">>";
		} else if (nextChar == '=') {
			ans->type = GREATER_THAN_EQUALS;
			ans->spelling = ">=";
		} else {
			ans->type = GREATER_THAN;
			ans->spelling = ">";
			ungetc(nextChar, stdin);
		}
		return ans;
	case '=':
		if ((nextChar = getc(stdin)) == '=') {
			ans->type = EQUALS;
			ans->spelling = "==";
			return ans;
		} else {
			puts("uh oh");
			// FOR LATER:
			// ans->type = ASSIGN;
			// ans->spelling = "=";
			ungetc(nextChar, stdin);
			exit(1); // TEMPORARY! VERY BAD!
		}
	case '!':
		if ((nextChar = getc(stdin)) == '=') {
			ans->type = NOT_EQUALS;
			ans->spelling = "!=";
		} else {
			ans->type = NOT;
			ans->spelling = "!";
			ungetc(nextChar, stdin);
		}
		return ans;
	case '&':
		ans->type = BITWISE_AND;
		ans->spelling = "&";
		return ans;
	case '^':
		ans->type = BITWISE_XOR;
		ans->spelling = "^";
		return ans;
	case '|':
		ans->type = BITWISE_OR;
		ans->spelling = "|";
		return ans;
	case 'a':
	case 'A':
		char1 = getc(stdin);
		char2 = getc(stdin);
		if (tolower(char1) == 'n' && tolower(char2) == 'd') {
			ans->type = AND;
			ans->spelling = calloc(3, sizeof(char));
			ans->spelling[0] = (char)nextChar;
			ans->spelling[1] = (char)char1;
			ans->spelling[2] = (char)char2;
			return ans;
		}
		ungetc(char2, stdin);
		ungetc(char1, stdin);
		//continue on
	case 'o':
	case 'O':
		char1 = getc(stdin);
		if (tolower(char1) == 'r') {
			ans->type = OR;
			ans->spelling = calloc(2, sizeof(char));
			ans->spelling[0] = nextChar;
			ans->spelling[1] = char1;
			return ans;
		}
		ungetc(char1, stdin);
		//continue on
	default:
		return lexRestPrefix(nextChar);
	}
}

struct Token *lexRestNumber(char first)
{
	struct Token *ans = malloc(sizeof(struct Token));
	int nextChar = '\0';
	ans->type = NUMBER;
	ans->spelling = calloc(64, sizeof(char));
	ans->spelling[0] = first;
	int i = 1;
	while (i < 63 && (isdigit((nextChar = getc(stdin))))) {
		ans->spelling[i++] = nextChar;
	}
	if (nextChar != '.') {
		ungetc(nextChar, stdin);
		return ans;
	}
	ans->spelling[i++] = nextChar;
	while (i < 63 && (isdigit((nextChar = getc(stdin))))) {
		ans->spelling[i++] = nextChar;
	}
	if (nextChar != '\0') {
		ungetc(nextChar, stdin);
	}
	ans->spelling[i] = '\0';
	return ans;
}

struct Token *lexRestPrefix(char first)
{
	// for now
	fprintf(stderr, "Unrecognized token: %c\n", first);
	exit(1);
}
