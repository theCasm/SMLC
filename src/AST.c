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
#include "lex.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

static void printTreeHelper(struct ASTLinkedNode *curr, int tabs);
static void freeTreeHelper(struct ASTLinkedNode *n);

void printTree(struct AST *tree)
{
    printTreeHelper(tree->root, 0);
}

static void printNode(struct ASTLinkedNode *node)
{
    if (node->val.type == VAR_DECL) {
        printf("%p ", node);
        if (node->val.isStatic) printf("static ");
        if (node->val.isConstant) printf("const ");
        printf("depth=%d ", node->val.frameDepth);
    } else if (node->val.type == IDENT_REF) {
        printf("def=%p ", node->val.definition);
    } else if (node->val.type == EXPR) {
        printf("type=`%s` ", TokenStrings[node->val.operationType]);
    }
    printf("%s", NODE_TYPE_STRINGS[node->val.type]);
}

// TODO: print context decorations, if they exist.
static void printTreeHelper(struct ASTLinkedNode *curr, int tabs)
{
    if (curr == NULL) {
        return;
    }
    for (int i = 0; i < tabs; i++) printf("\t");
    printNode(curr);
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
    ans->isStatic = 0;
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
    ans->val.isStatic = 0;
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

