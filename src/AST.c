/*
 * Copyright 2024 Aidan Undheim
 *
 * This file is part of SMLC.
 * 
 * SMLC is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 * 
 * SMLC is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * SMLC. If not, see <https://www.gnu.org/licenses/>. 
*/
#include "AST.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

static void printTreeHelper(struct ASTLinkedNode *curr, int tabs);
static void freeTreeHelper(struct ASTLinkedNode *n);

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
    ans->startIndex = 0;
    ans->endIndex = 0;
    ans->birth = -1;
    ans->death = -1;
    return ans;
}

struct ASTLinkedNode *newLinkedAstNode(enum NodeType type)
{
    struct ASTLinkedNode *ans = malloc(sizeof(*ans));
    ans->val.type = type;
    ans->val.children = NULL;
    ans->val.isConstant = 0;
    ans->val.startIndex = 0;
    ans->val.endIndex = 0;
    ans->val.birth = -1;
    ans->val.death = -1;
    ans->next = NULL;
    return ans;
}

static void freeTreeHelper(struct ASTLinkedNode *n)
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

