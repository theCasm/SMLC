#include "lex.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

static struct Token *next = NULL;
struct Token *searchForNext(void);
struct Token *lexRestNumber(struct Token *);
struct Token *lexRestPrefix(char);

size_t inputIndex = 0;
char *fullInput = NULL;
size_t fullInputSize = 0;

int getNextChar() {
	if (fullInput == NULL) {
		fullInput = calloc(512, sizeof(char));
		fullInputSize = 512;
	} else if (inputIndex + 1 >= fullInputSize) {
		fullInput = realloc(fullInput, fullInputSize * 2);
		fullInputSize *= 2;
	}
	int ans = getc(stdin);
	fullInput[inputIndex++] = (char)ans;
	return ans;
}

void undoNextChar(char c) {
	inputIndex--;
	ungetc(c, stdin);
}

void freeToken(struct Token *token)
{
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
		char *unexpectedTok = malloc(next->end - next->start + 1);
		strncpy(unexpectedTok, fullInput + next->start, next->end - next->start);
		unexpectedTok[next->end - next->start] = '\0';
		fprintf(stderr, "Expected `%s` but got `%s`\n", TokenStrings[type], unexpectedTok);
		free(unexpectedTok);
	}
	freeToken(next);
	acceptIt();
}

int char1, char2;

struct Token *searchForNext()
{
	int nextChar;
	struct Token *ans = malloc(sizeof(struct Token));
	while ((nextChar = getNextChar()) == ' ' || nextChar == '\t');
	ans->start = inputIndex - 1;
	if (nextChar == EOF) {
		ans->type = TOKEN_EOF;
		ans->end = inputIndex;
		return ans;
	}
	if (nextChar == '\n') {
		ans->type = LINE_END;
		ans->end = inputIndex;
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
		return lexRestNumber(ans);
	case '(':
		ans->type = LPAR;
		break;
		return ans;
	case ')':
		ans->type = RPAR;
		break;
	case '-':
		ans->type = MINUS;
		break;
	case '~':
		ans->type = BITWISE_NOT;
		break;
	case '+':
		ans->type = PLUS;
		break;
	case '*':
		ans->type = TIMES;
		break;
	case '/':
		ans->type = DIVIDE;
		break;
	case '%':
		ans->type = MODULO;
		break;
	case '<':
		if ((nextChar = getNextChar()) == '<') {
			ans->type = LEFT_SHIFT;
		} else if (nextChar == '=') {
			ans->type = LESS_THAN_EQUALS;
		} else {
			ans->type = LESS_THAN;
			undoNextChar(nextChar);
		}
		break;
	case '>':
		if ((nextChar = getNextChar()) == '>') {
			ans->type = RIGHT_SHIFT;
		} else if (nextChar == '=') {
			ans->type = GREATER_THAN_EQUALS;
		} else {
			ans->type = GREATER_THAN;
			undoNextChar(nextChar);
		}
		break;
	case '=':
		if ((nextChar = getNextChar()) == '=') {
			ans->type = EQUALS;
			break;
		} else {
			puts("uh oh");
			// FOR LATER:
			// ans->type = ASSIGN;
			// ans->spelling = "=";
			undoNextChar(nextChar);
			exit(1); // TEMPORARY! VERY BAD!
		}
	case '!':
		if ((nextChar = getNextChar()) == '=') {
			ans->type = NOT_EQUALS;
		} else {
			ans->type = NOT;
			undoNextChar(nextChar);
		}
		break;
	case '&':
		ans->type = BITWISE_AND;
		break;
	case '^':
		ans->type = BITWISE_XOR;
		break;
	case '|':
		ans->type = BITWISE_OR;
		break;
	case 'a':
	case 'A':
		char1 = getNextChar();
		char2 = getNextChar();
		if (tolower(char1) == 'n' && tolower(char2) == 'd') {
			ans->type = AND;
			break;
		}
		undoNextChar(char2);
		undoNextChar(char1);
		//continue on
	case 'o':
	case 'O':
		char1 = getNextChar();
		if (tolower(char1) == 'r') {
			ans->type = OR;
			break;
		}
		undoNextChar(char1);
		//continue on
	default:
		return lexRestPrefix(nextChar);
	}
	ans->end = inputIndex;
	return ans;
}

struct Token *lexRestNumber(struct Token *ans)
{
	int nextChar = '\0';
	ans->type = NUMBER;
	int i = 0;
	while ((isdigit((nextChar = getNextChar())) || (i == 0 && nextChar == 'x'))) i++;
	if (nextChar != '.') {
		undoNextChar(nextChar);
		ans->end = inputIndex;
		return ans;
	}
	while ((isdigit((nextChar = getNextChar())))) {}
	if (nextChar != '\0') {
		undoNextChar(nextChar);
	}
	ans->end = inputIndex;
	return ans;
}

struct Token *lexRestPrefix(char first)
{
	// for now
	fprintf(stderr, "Unrecognized token: %c\n", first);
	exit(1);
}
