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

//#define STACK_WORDS (128)
#define STACK_WORDS 16
#define DEFAULT_STACK_TOP (0x3000)

static void codegenProgram(struct ASTLinkedNode *program);
static void codegenFuncDecl(struct ASTLinkedNode *decl);
static void codegenSingleCommand(struct ASTLinkedNode *command);
static void codegenFuncCall(struct ASTLinkedNode *call, int regDest);
static void codegenIdentRef(struct ASTLinkedNode *varref, int regDest);
static void codegenExpr(struct ASTLinkedNode *expr, int regDest);
static void codegenOperation(enum TokenType type, int regArg, int regDest);
static void codegenPrefixOperation(enum TokenType type, int reg);

static char startAsm[] = ".pos 0x1000\n"
    "_start:\n"
    "ld $_stackBottom, r5\n"
    "deca r5\n"
    "gpc $6, r6\n"
    "j main\n"
    "halt\n";

static char saveAllGPRegs[] = "deca r5\t\t# save all regs\n"
    "st r0, (r5)\n"
    "ld $-20, r0\n"
    "add r0, r5\n"
    "st r1, 16(r5)\n"
    "st r2, 12(r5)\n"
    "st r3, 8(r5)\n"
    "st r4, 4(r5)\n"
    "st r7, (r5)\n\n";

static char restoreAllGPRegs[] = "ld (r5), r7\t\t# restore all regs\n"
    "ld 4(r5), r4\n"
    "ld 8(r5), r3\n"
    "ld 12(r5), r2\n"
    "ld 16(r5), r1\n"
    "ld $20, r0\n"
    "add r0, r5\n"
    "ld (r5), r0\n"
    "inca r5\n\n";

void generateCode(struct AST *tree)
{
    codegenProgram(tree->root);
}

static int isInFn = 0;
static int uniqueNum = 0;

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
        if (child->val.children->val.type == FN_DECL) {
            codegenFuncDecl(child->val.children);
        } else {
            //printf("%s", NODE_TYPE_STRINGS[child->val.type]);
        }
    }
    // TODO: keep track of bytes so far so there is no hope of a data/stack section being on top of something else
    fprintf(stdout, ".pos 0x%X\n", DEFAULT_DATA_TOP);
    for (child = program->val.children; child != NULL; child = child->next) {
        if (child->val.children->val.type == VAR_DECL) {
            char *name = calloc(child->val.children->val.endIndex - child->val.children->val.startIndex + 1, sizeof(char));
            getInputSubstr(name, child->val.children->val.startIndex, child->val.children->val.endIndex);
            fprintf(stdout, "%s: .long 0\n", name);
            free(name);
        }
    }

    fprintf(stdout, ".pos 0x%X\n_stackTop:\n", DEFAULT_STACK_TOP);
    for (size_t i = 0; i < STACK_WORDS; i++) {
        fputs(".long 0\n", stdout);
    }
    fputs("_stackBottom:\n", stdout);
}

static void codegenFuncDecl(struct ASTLinkedNode *decl)
{
    char *name = calloc(decl->val.children->val.endIndex - decl->val.children->val.startIndex + 1, sizeof(char));
    getInputSubstr(name, decl->val.children->val.startIndex, decl->val.children->val.endIndex);
    fprintf(stdout, "%s:\n", name);
    free(name);
    fputs(saveAllGPRegs, stdout);
    fprintf(stdout, "ld $-%d, r0\t\t# allocate local vars\nadd r0, r5\n\n", 4*decl->val.frameVars);
    isInFn = 1;
    codegenSingleCommand(decl->val.children->next->next);
    fprintf(stdout, "\nld $%d, r0\t\t# de-alloc local vars\nadd r0, r5\n\n", 4*decl->val.frameVars);
    fputs(restoreAllGPRegs, stdout);
    fputs("j (r6)\n", stdout);
}

/*
 * Generates assembly for single command. Assumes access to all registers.
 */
static void codegenSingleCommand(struct ASTLinkedNode *command)
{
    struct ASTLinkedNode *temp, *child = command->val.children;
    switch(child->val.type) {

    case CONST_DECL:
        return;
    case VAR_DECL:
        if (!child->val.children->next) {
            return;
        }
        codegenExpr(child->val.children->next, 0);
        fprintf(stdout, "st r0, %d(r5)\n", child->val.frameIndex*4);
        return;
    case IF_EXPR:
        return;
    case WHILE_LOOP:
        return;
    case COMMAND:
        for (temp = child->val.children; temp != NULL; temp = temp->next) {
            codegenSingleCommand(temp);
            if (!isInFn) break;
        }
        return;
    case DIRECT_ASSIGN:
        codegenExpr(child->val.children->next, 0);
        fprintf(stdout, "st r0, %d(r5)\n", child->val.frameIndex*4);
        return;
    case INDIRECT_ASSIGN:
        codegenExpr(child->val.children, 0);
        codegenExpr(child->val.children->next, 1);
        fputs("st r1, (r0)\n", stdout);
        break;
    case FUNC_CALL:
        codegenFuncCall(child, 0);
        return;
    case RETURN_DIRECTIVE:
        isInFn = 0;
        return;
    default:
        fprintf(stderr, "codegenSingleCommand does not recognize `%s` - dang coupling :c\nIgnoring\n",
            NODE_TYPE_STRINGS[child->val.type]);
        return;
    }
}

static void codegenFuncCall(struct ASTLinkedNode *call, int regDest)
{
    struct ASTLinkedNode *temp;
    fputs("deca r5\nst r0, (r5)\n", stdout);
    fprintf(stdout, "ld $-%d, r0\nadd r0, r5\n", 4*call->val.paramCount);
    for (temp = call->val.children->next->val.children; temp != NULL; temp = temp->next) {
        // TODO:
    }
    char *name = calloc(call->val.children->val.endIndex - call->val.children->val.startIndex + 1, sizeof(char));
    getInputSubstr(name, call->val.children->val.startIndex, call->val.children->val.endIndex);
    fprintf(stdout, "j %s", name);
    free(name);
    fprintf(stdout, "mov r0, r%d", regDest);
    fputs("ld (r5), r0\ninca r5\n", stdout);
    fprintf(stdout, "ld $%d, r0\nadd r0, r5\n", 4*call->val.paramCount);
}

static void codegenIdentRef(struct ASTLinkedNode *varref, int regDest)
{
    if (varref->val.definition->val.isConstant) {
        fprintf(stdout, "ld %d, r%d\n", varref->val.definition->val.val, regDest);
        return;
    }
    struct ASTLinkedNode *identifier = varref->val.definition->val.children;
    if (varref->val.definition->val.isStatic) {
        char *name = calloc(identifier->val.endIndex - identifier->val.startIndex + 1, sizeof(char));
        getInputSubstr(name, identifier->val.startIndex, identifier->val.endIndex);
        fprintf(stdout, "ld $%s, r%d\nld (r%d), r%d\n", name, regDest, regDest, regDest);
        free(name);
        return;
    }
    fprintf(stdout, "ld %d(r5), r%d\n", 4*varref->val.definition->val.frameIndex, regDest);
    return;
}

/*
 * calculates operation on reg
*/
static void codegenPrefixOperation(enum TokenType type, int reg)
{
    switch (type) {
    case MINUS:
        fprintf(stdout, "not r%d\ninc r%d\n", reg, reg);
        return;
    case BITWISE_NOT:
        fprintf(stdout, "not r%d\n", reg);
        return;
    case NOT:
        // TODO
        return;
    case DEREF:
        fprintf(stdout, "ld (r%d), r%d\n", reg, reg);
        return;
    default:
        fprintf(stderr, "CODEGEN: idk how to fold in prefix %s\n", TokenStrings[type]);
		return;
    }
}

/*
 * Generates asm to compute expression and store result in regDest
 * Preserves all regs < regDest, assumes regs >= regDest can be modified at will
 * r5, r6 are treated specially and are never clobbered.
*/
static void codegenExpr(struct ASTLinkedNode *expr, int regDest)
{
    if (expr->val.type == NUMBER_LITERAL) {
        fprintf(stdout, "ld $%d, r%d", expr->val.val, regDest);
        return;
    } else if (expr->val.type == FUNC_CALL) {
        codegenFuncCall(expr, regDest);
        return;
    } else if (expr->val.type == IDENT_REF) {
        codegenIdentRef(expr, regDest);
        return;
    }

    int left = regDest;
    int right = regDest + 1;
    if (isInfix(expr->val.operationType)) {
        if (regDest >= 4) {
            // TODO
            return;
        }
        codegenExpr(expr->val.children, left);
        codegenExpr(expr->val.children->next, right);
        codegenOperation(expr->val.operationType, left, right);
        return;
    }
    codegenExpr(expr->val.children, left);
    codegenPrefixOperation(expr->val.operationType, left);
}

/*
 * Computes operation and stores in left.
 * CLOBBERS *BOTH* left and right.
*/
static void codegenOperation(enum TokenType type, int left, int right)
{
    switch (type) {
	case PLUS:
        fprintf(stdout, "add r%d, r%d\n", right, left);
		return;
	case MINUS:
		fprintf(stdout, "not r%d\ninc r%d\nadd r%d, r%d\n", right, right, right, left);
		return;
	case TIMES:
        // We use r6 for this - having a guaranteed extra reg is helpful.
        fputs("deca r5\nst r6, (r5)\n", stdout);
        fprintf(stdout, "L%d:\n", uniqueNum);
        fprintf(stdout, "ld $1, r6\nand r%d, r6\nbe r6, L%dC\ninc r%d\n", right, uniqueNum, left);
        fprintf(stdout, "L%dC:\nshr $1, r%d\n shl $1, r%d\n", uniqueNum, right, left);
        fprintf(stdout, "L%dE:\n", uniqueNum++);
        fputs("ld (r5), r6\ninca r5\n", stdout);
		return;
	case DIVIDE:
        // TODO: not too important right now.
		fprintf(stdout, "add r%d, r%d\n", right, left);
		return;
	case MODULO:
        // TODO: not too important right now.
		fprintf(stdout, "add r%d, r%d\n", right, left);
		return;
	case LEFT_SHIFT:
        // TODO: we assume right is at most 32. Otherwise, this is equivilant to only looking at rightmost 5 bit.
        // TODO:
		fprintf(stdout, "add r%d, r%d\n", right, left);
		return;
	case RIGHT_SHIFT:
        // TODO:
		fprintf(stdout, "add r%d, r%d\n", right, left);
		return;
	case LESS_THAN:
        // TODO:
		fprintf(stdout, "add r%d, r%d\n", right, left);
		return;
	case LESS_THAN_EQUALS:
        // TODO;
		fprintf(stdout, "add r%d, r%d\n", right, left);
		return;
	case GREATER_THAN:
        // TODO
		fprintf(stdout, "add r%d, r%d\n", right, left);
		return;
	case GREATER_THAN_EQUALS:
        // TODO
		fprintf(stdout, "add r%d, r%d\n", right, left);
		return;
	case EQUALS:
        // TODO
		fprintf(stdout, "add r%d, r%d\n", right, left);
		return;
	case NOT_EQUALS:
        // TODO
		fprintf(stdout, "add r%d, r%d\n", right, left);
		return;
	case OR:
        // TODO
		fprintf(stdout, "add r%d, r%d\n", right, left);
		return;
	case AND:
        // TODO
		fprintf(stdout, "add r%d, r%d\n", right, left);
		return;
	case BITWISE_AND:
        // TODO
		fprintf(stdout, "add r%d, r%d\n", right, left);
		return;
	case BITWISE_OR:
        // TODO
		fprintf(stdout, "add r%d, r%d\n", right, left);
		return;
	case BITWISE_XOR:
        // TODO
		fprintf(stdout, "add r%d, r%d\n", right, left);
		return;
	default:
		fprintf(stderr, "CODEGEN: idk how to fold in %s\n", TokenStrings[type]);
		return;
	}
}