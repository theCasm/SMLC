#ifndef SML_AST_H
#define SML_AST_H

#include <unistd.h>
#include "lex.h"

enum NodeType {
    PROGRAM,
    GLOBAL_DECL,
    FN_DECL,
    CONST_DECL,
    VAR_DECL,
    IDENTIFIER,
    CONST_EXPR,
    EXPR,
    COMMAND,
    SINGLE_COMMAND,
    IF,
    WHILE,
    NUMBER_LITERAL
};

static const char *NODE_TYPE_STRINGS[] = {
    "program",
    "global declaration",
    "function declaration",
    "constant declaration",
    "variable declaration",
    "identifier",
    "constant expression",
    "expression",
    "command",
    "single command",
    "if statement",
    "while loop",
    "number literal"
};

struct ASTNode {
    enum NodeType type;
    int isConstant;
    struct ASTNode *left;
    struct ASTNode *right;
    union {
        struct {
            size_t start;
            size_t death;
        };
        enum TokenType operationType;
        int val;
    };
};

struct AST {
    struct ASTNode *root;
};

void printTree(struct AST *);
struct ASTNode *newAstNode(enum NodeType type);
void freeTree(struct AST *);

#endif