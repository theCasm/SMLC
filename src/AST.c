#include "AST.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

static void printTreeHelper(struct ASTNode *curr);

void printTree(struct AST *tree)
{
    printTreeHelper(tree->root);
}

static void printTreeHelper(struct ASTNode *curr)
{
    if (curr == NULL) {
        return;
    }
    if (curr->left != NULL) {
        printf("(");
        printTreeHelper(curr->left);
        printf(")<-");
    }
    printf("%s", NODE_TYPE_STRINGS[curr->type]);
    if (curr->right != NULL) {
        printf("->(");
        printTreeHelper(curr->right);
        printf(")");
    }
}

struct ASTNode *newAstNode(enum NodeType type)
{
    struct ASTNode *ans = malloc(sizeof(*ans));
    ans->type = type;
    ans->left = NULL;
    ans->right = NULL;
    ans->isConstant = 0;
    ans->start = -1;
    ans->death = -1;
    return ans;
}

static void freeTreeHelper(struct ASTNode *n)
{
    if (n == NULL) {
        return;
    }
    freeTreeHelper(n->left);
    freeTreeHelper(n->right);
    free(n);
}

void freeTree(struct AST *t)
{
    freeTreeHelper(t->root);
    free(t);
}

