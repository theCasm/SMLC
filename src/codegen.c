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
 * 
 * This file does the actual translation part of compiling. As part of this, it:
 *  - creates the _start entry point function which calls main
 *  - determines the stack top and bottom addresses
 * 
 * So that the code made by the user has a nice environment to run in.
*/

#include <stdio.h>
#include <stdlib.h>

#include "codegen.h"
#include "AST.h"

#define DEFAULT_DATA_TOP (0x2000)

#define STACK_WORDS (128)
#define DEFAULT_STACK_TOP (0x3000)

static void codegenProgram(struct ASTLinkedNode *program);
static void codegenFuncDecl(struct ASTLinkedNode *decl);
static void codegenSingleCommand(struct ASTLinkedNode *command, int frameOffset);

static char startAsm[] = ".pos 0x1000\n"
    "_start:\n"
    "ld $_stackBottom, r5\n"
    "deca r5\n"
    "gpc $6, r6\n"
    "j main\n"
    "halt";

void generateCode(struct AST *tree)
{
    codegenProgram(tree->root);
}

/*
 * EFFECTS: outputs assembler for the program organized as such:
 *   .pos 0x1000
 *   _start
 *   fn defs
 * 
 *   .pos <data top>
 *   global vars
 * 
 *   .pos <stack top>
 *   stack top
*/
static void codegenProgram(struct ASTLinkedNode *program)
{
    fputs(startAsm, stdout);
    struct ASTLinkedNode * child;
    for (child = program->val.children; child != NULL; child = child->next) {
        if (child->val.type == FUNC) {
            codegenFuncDecl(child);
        }
    }
    // TODO: keep track of bytes so far so there is no hope of a data/stack section being on top of something else
    fprintf(stdout, ".pos 0x%X\n", DEFAULT_DATA_TOP);
    for (child = program->val.children; child != NULL; child = child->next) {
        if (child->val.type == VAR_DECL) {
            char *name = calloc(child->val.endIndex - child->val.startIndex + 1, sizeof(char));
            getInputSubstr(name, child->val.startIndex, child->val.endIndex);
            fprintf(stdout, "%s: .long 0\n", name);
            free(name);
        }
    }

    fprintf(stdout, ".pos 0x%X\n_stackTop:\n", DEFAULT_STACK_TOP);
    for (size_t i = 0; i < STACK_WORDS; i++) {
        fputs(".long 0", stdout);
    }
    fputs("_stackBottom:\n", stdout);
}

static void codegenFuncDecl(struct ASTLinkedNode *decl)
{
    // TODO
}