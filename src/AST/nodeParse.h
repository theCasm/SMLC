#ifndef SML_NODE_PARSE_H
#define SML_NODE_PARSE_H

struct ASTLinkedNode *parseProgram();
struct ASTLinkedNode *parseGlobalDecl();
struct ASTLinkedNode *parseCommand();
struct ASTLinkedNode *parseSingleCommand();
struct ASTLinkedNode *parseFunctionDecl();
struct ASTLinkedNode *parseParamList();
struct ASTLinkedNode *parseArgList();
struct ASTLinkedNode *parseIfExpr();
struct ASTLinkedNode *parseWhileLoop();
struct ASTLinkedNode *parseConstDecl();
struct ASTLinkedNode *parseVarDecl();
struct ASTLinkedNode *parseIdentifierCommand();
struct ASTLinkedNode *parseIndirectAssignment();
struct ASTLinkedNode *parsePriority(int priority);
struct ASTLinkedNode *parseExpr();
struct ASTLinkedNode *parsePrimaryExpr();
struct ASTLinkedNode *parseIdentifier();

#endif