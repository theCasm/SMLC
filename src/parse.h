#ifndef SML_PARSE_H
#define SML_PARSE_H

#include "AST.h"

struct AST *parse(void);
struct ASTLinkedNode *parseSingleCommand(); // for testing only - TODO remove eventually

#endif
