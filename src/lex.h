#ifndef SML_LEX_H
#define SML_LEX_H

enum TokenType {
	NUMBER,
	LPAR,
	RPAR,
	PLUS,
	MINUS,
	TIMES,
	DIVIDE,
	MODULO,
	AND,
	OR,
	EQUALS,
	NOT_EQUALS,
	NOT,
	LESS_THAN,
	LESS_THAN_EQUALS,
	GREATER_THAN,
	GREATER_THAN_EQUALS,
	LEFT_SHIFT,
	RIGHT_SHIFT,
	BITWISE_AND,
	BITWISE_OR,
	BITWISE_XOR,
	BITWISE_NOT,
	TOKEN_EOF,
	LINE_END
};

static const char *TokenStrings[] = {
	"Number",
	"(",
	")",
	"+",
	"-",
	"*",
	"/",
	"%",
	"and",
	"or",
	"==",
	"!=",
	"!",
	"<",
	"<=",
	">",
	">=",
	"<<",
	">>",
	"&",
	"|",
	"^",
	"~",
	"EOF",
	"\\n"
};

struct Token {
	enum TokenType type;
	char *spelling;
};

int isInfix(enum TokenType);
struct Token *peek(void);
void acceptIt(void);
void accept(enum TokenType);

#endif
