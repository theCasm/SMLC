#ifndef SML_AST_H
#define SML_AST_H

#include <unistd.h>
#include "lex.h"

enum NodeType {
    PROGRAM,
    GLOBAL_DECL,
    FN_DECL,
    PARAM_LIST,
    ARG_LIST,
    CONST_DECL,
    VAR_DECL,
    DIRECT_ASSIGN,
    INDIRECT_ASSIGN,
    IDENT_REF,
    FUNC_CALL,
    CONST_EXPR,
    EXPR,
    COMMAND,
    SINGLE_COMMAND,
    IF_EXPR,
    WHILE_LOOP,
    NUMBER_LITERAL
};

static const char *NODE_TYPE_STRINGS[] = {
    "program",
    "global declaration",
    "function declaration",
    "parameter list",
    "argument list",
    "constant declaration",
    "variable declaration",
    "direct assignment",
    "indirect assignment",
    "identifier reference",
    "function call",
    "constant expression",
    "expression",
    "command block",
    "single command",
    "if statement",
    "while loop",
    "number literal"
};

struct ASTNode {
    enum NodeType type;
    int isConstant;
    struct ASTLinkedNode *children;
    size_t startIndex;
    size_t endIndex;
    union {
        struct {
            size_t birth;
            size_t death;
        };
        enum TokenType operationType;
        int isVoid;
        int val;
    };
};

struct ASTLinkedNode {
    struct ASTLinkedNode *next;
    struct ASTNode val;
};

struct AST {
    struct ASTLinkedNode *root;
};

void printTree(struct AST *);
struct ASTNode *newAstNode(enum NodeType type);
struct ASTLinkedNode *newLinkedAstNode(enum NodeType type);
void freeTree(struct AST *);

#endif