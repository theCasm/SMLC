#ifndef SML_AST_H
#define SML_AST_H

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

#endif