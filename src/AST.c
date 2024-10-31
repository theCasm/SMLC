#include "AST.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

static void printTreeHelper(struct ASTLinkedNode *curr, int tabs);

void printTree(struct AST *tree)
{
    printTreeHelper(tree->root, 0);
}

static void printTreeHelper(struct ASTLinkedNode *curr, int tabs)
{
    if (curr == NULL) {
        return;
    }
    for (int i = 0; i < tabs; i++) printf("\t");
    printf("%s", NODE_TYPE_STRINGS[curr->val.type]);
    struct ASTLinkedNode *c = curr->val.children;
    if (c != NULL) {
        printf("->{\n");
        while (c != NULL) {
            printTreeHelper(c, tabs + 1);
            c = c->next;
        }
        for (int i = 0; i < tabs; i++) printf("\t");
        printf("}\n");
    }
    printf("\n");
}

struct ASTNode *newAstNode(enum NodeType type)
{
    struct ASTNode *ans = malloc(sizeof(*ans));
    ans->type = type;
    ans->children = NULL;
    ans->isConstant = 0;
    ans->start = -1;
    ans->death = -1;
    return ans;
}

struct ASTLinkedNode *newLinkedAstNode(enum NodeType type)
{
    struct ASTLinkedNode *ans = malloc(sizeof(*ans));
    ans->val.type = type;
    ans->val.children = NULL;
    ans->val.isConstant = 0;
    ans->val.start = -1;
    ans->val.death = -1;
    ans->next = NULL;
    return ans;
}

struct ASTLinkedNode *linkNode(struct ASTNode *n)
{
    struct ASTLinkedNode *ans = newLinkedAstNode(n->type);
    ans->val.children = n->children;
    ans->val.isConstant = n->isConstant;
    ans->val.start = n->start;
    ans->val.death = n->death;
    free(n); // if we have any dangling pointers, they're made here!
    return ans;
}

void freeTreeHelper(struct ASTLinkedNode *n)
{
    if (n == NULL) {
        return;
    }
    if (n->val.children != NULL) {
        struct ASTLinkedNode *t, *c = n->val.children;
        while (c != NULL) {
            t = c->next;
            freeTreeHelper(c);
            c = t;
        }
    }
    free(n);
}

void freeTree(struct AST *t)
{
    freeTreeHelper(t->root);
    free(t);
}

