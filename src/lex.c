#include "lex.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

static struct Token *next = NULL;
struct Token *searchForNext(void);
struct Token *lexRestNumber(struct Token *);
static struct Token *checkForIdentifier(struct Token *ans);
static struct Token *handleUnrecognized(int, int);
static int checkInputAgainstStr(char *, int);

size_t inputIndex = 0;
char *fullInput = NULL;
size_t fullInputSize = 0;

// TODO: calling getchar not necessary if we already have next char buffered
// though it won't make any performance difference as stdlib is really good with character buffering.
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

void undoNextChar() {
	ungetc(fullInput[--inputIndex], stdin);
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

/*
 * Accepts last token as correct and deletes it.
 * once a token is accepted, it is gone forever - no use after free's, please!
*/
void acceptIt()
{
	freeToken(next);
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
	acceptIt();
}

int char1, char2;

struct Token *searchForNext()
{
	int nextChar;
	struct Token *ans = malloc(sizeof(struct Token));
	nextChar = getNextChar();
	while (nextChar == ' ' || nextChar == '\t') {
		nextChar = getNextChar();
	}
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
	case '{':
		ans->type = LCPAR;
		break;
	case '}':
		ans->type = RCPAR;
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
			undoNextChar();
		}
		break;
	case '>':
		if ((nextChar = getNextChar()) == '>') {
			ans->type = RIGHT_SHIFT;
		} else if (nextChar == '=') {
			ans->type = GREATER_THAN_EQUALS;
		} else {
			ans->type = GREATER_THAN;
			undoNextChar();
		}
		break;
	case '=':
		if ((nextChar = getNextChar()) == '=') {
			ans->type = EQUALS;
			break;
		}
		undoNextChar();
		ans->type = ASSIGN;
		break;
	case '!':
		if ((nextChar = getNextChar()) == '=') {
			ans->type = NOT_EQUALS;
		} else {
			ans->type = NOT;
			undoNextChar();
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
	case ',':
		ans->type = COMMA;
		break;
	default:
		undoNextChar();
		return checkForIdentifier(ans);
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
		undoNextChar();
		ans->end = inputIndex;
		return ans;
	}
	while ((isdigit((nextChar = getNextChar())))) {}
	if (nextChar != '\0') {
		undoNextChar();
	}
	ans->end = inputIndex;
	return ans;
}

static struct Token *checkForIdentifier(struct Token *ans)
{
	int nextChar = getNextChar();
	ans->type = -1;
	switch(nextChar) {
	case 'a':
	case 'A':
		if (checkInputAgainstStr("nd", 1)) {
			ans->type = AND;
		}
		break;
	case 'c':
	case 'C':
		if (checkInputAgainstStr("onst", 1)) {
			ans->type = CONST;
		}
		break;
	case 'f':
	case 'F':
		if (checkInputAgainstStr("unc", 1)) {
			ans->type = FUNC;
		}
		break;
	case 'i':
	case 'I':
		if (checkInputAgainstStr("f", 1)) {
			ans->type = IF;
		}
		break;
	case 'n':
	case 'N':
		if (checkInputAgainstStr("on-void", 1)) {
			ans->type = NON_VOID;
		}
		break;
	case 'o':
	case 'O':
		char1 = getNextChar();
		if (tolower(char1) == 'r') {
			ans->type = OR;
		}
		break;
	case 'v':
	case 'V':
		if (checkInputAgainstStr("ar", 1)) {
			ans->type = VAR;
		} else if (checkInputAgainstStr("oid", 1)) {
			ans->type = VOID;
		}
		break;
	case 'w':
	case 'W':
		if (checkInputAgainstStr("hile", 1)) {
			ans->type = WHILE;
		}
		break;
	}
	if (ans->type != -1) {
		ans->end = inputIndex;
		return ans;
	}
	if (!isalpha(nextChar)) {
		handleUnrecognized(ans->start, inputIndex);
	}
	ans->type = IDENTIFIER;
	do {
		nextChar = getNextChar();
	} while (isalnum(nextChar));
	undoNextChar();
	ans->end = inputIndex;
	return ans;
}

/*
 * Compares next characters case-insensative to s.
 * if peek = 0: will not unget characters, no matter what.
 * if peek = 1: will unget characters if the comparison fails. 
 * if peek = 2: will unget characters no matter what
 * 
 * I have not entirely decided which form to use, so for now all of these options are available.
 * TODO: replace these constants with a static enum
 *
 * REQUIRES: len(s) >= 1
*/
static int checkInputAgainstStr(char *s, int peek)
{
	int next;
	size_t i = 0;

	while (i < strlen(s)) {
		if (tolower((next = getNextChar())) != tolower(s[i++])) {
			while ((peek >= 1) && i-- > 0) {
				undoNextChar();
			}
			return 0;
		}
	}
	while ((peek == 2) && i-- > 0) {
				undoNextChar();
	}
	return 1;
}

static struct Token *handleUnrecognized(int start, int end)
{
	// for now
	char *spelling = calloc(end - start + 1, sizeof(char));
	strncpy(spelling, fullInput + start, end - start);
	spelling[end - start] = '\0';
	fprintf(stderr, "Unrecognized token: %s\n                    ^\n", spelling);
	exit(1);
}
