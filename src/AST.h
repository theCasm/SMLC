#ifndef SML_AST_H
#define SML_AST_H

#include <unistd.h>

void printTree();

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
    WHILE
};

struct ASTNode {
    enum NodeType type;
    struct ASTNode *left;
    struct ASTNode *right;
    size_t start;
    size_t death;
};

struct AST {
    struct ASTNode *head;
};

#endif