#ifndef SML_IO_H
#define SML_IO_H

#include "../AST/AST.h"
#include "../lex.h"

// TODO: no return value
struct ASTLinkedNode *handleUnexpectedToken(struct Token *tok);

#endif