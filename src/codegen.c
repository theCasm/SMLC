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
static void codegenSingleCommand(struct ASTLinkedNode *command);
static void codegenFuncCall(struct ASTLinkedNode *call, int regDest);
static void codegenIdentRef(struct ASTLinkedNode *varref, int regDest);
static void codegenWhileLoop(struct ASTLinkedNode *loop);
static void codegenIf(struct ASTLinkedNode *ifExpr);
static void codegenDirectAssign(struct ASTLinkedNode *assignment);
static void codegenExpr(struct ASTLinkedNode *expr, int regDest);
static void codegenOperation(enum TokenType type, int regArg, int regDest);
static void codegenPrefixOperation(enum TokenType type, int reg);
static void codegenDynamicMultiplication(int left, int right);

static char startAsm[] = ".pos 0x1000\n"
    "_start:\n"
    "ld $_stackBottom, r5\n"
    "deca r5\n"
    "gpc $6, r6\n"
    "j main\n"
    "halt\n\n";

static char saveAllGPRegs[] = "deca r5\t\t# save all regs\n"
    "st r0, (r5)\n"
    "ld $-20, r0\n"
    "add r0, r5\n"
    "st r1, 16(r5)\n"
    "st r2, 12(r5)\n"
    "st r3, 8(r5)\n"
    "st r4, 4(r5)\n"
    "st r7, (r5)\n\n";

static char restoreAllGPRegs[] = "\nld (r5), r7\t\t# restore all regs\n"
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
static int frameArgOffset = 0;
static int entireFrameOffset = 0;

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
            char *name = calloc(child->val.children->val.children->val.endIndex - child->val.children->val.children->val.startIndex + 1, sizeof(char));
            getInputSubstr(name, child->val.children->val.children->val.startIndex, child->val.children->val.children->val.endIndex);
            fprintf(stdout, "%s: .long 0\n", name);
            free(name);
        }
    }

    fprintf(stdout, ".pos 0x%X\n_stackTop:\n", DEFAULT_STACK_TOP);
    for (size_t i = 0; i < STACK_WORDS; i++) {
        fputs(".long 0\n", stdout);
    }
    fputs("_stackBottom: .long 0\n", stdout);
}

/*
 * 
*/
static void codegenFuncDecl(struct ASTLinkedNode *decl)
{
    char *name = calloc(decl->val.children->val.endIndex - decl->val.children->val.startIndex + 1, sizeof(char));
    getInputSubstr(name, decl->val.children->val.startIndex, decl->val.children->val.endIndex);
    fprintf(stdout, "%s:\n", name);
    free(name);
    fputs(saveAllGPRegs, stdout);
    frameArgOffset += 24;
    if (decl->val.clobbersReturn) {
        fputs("deca r5\t\t# save r6\nst r6, (r5)\n", stdout);
        frameArgOffset += 4;
    }
    if (decl->val.frameVars > 0) {
        fprintf(stdout, "ld $-%d, r0\t\t# allocate local vars\nadd r0, r5\n\n", 4*decl->val.frameVars);
        frameArgOffset += 4*decl->val.frameVars;
    }
    isInFn = 1;
    codegenSingleCommand(decl->val.children->next->next);
    if (decl->val.frameVars > 0) {
        fprintf(stdout, "\nld $%d, r0\t\t# de-alloc local vars\nadd r0, r5\n\n", 4*decl->val.frameVars);
        frameArgOffset -= 4*decl->val.frameVars;
    }

    if (decl->val.clobbersReturn) {
        fputs("ld (r5), r6\t\t# restore r6\ninca r5\n", stdout);
        frameArgOffset -= 4;
    }
    fputs(restoreAllGPRegs, stdout);
    frameArgOffset -= 24;
    fputs("j (r6)\t\t# return\n\n", stdout);
}

/*
 * Generates assembly for single command. Assumes access to all registers.
 */
static void codegenSingleCommand(struct ASTLinkedNode *command)
{
    if (command->val.type == RETURN_DIRECTIVE) {
        isInFn = 0;
        if (command->val.children) {
            codegenExpr(command->val.children, 0);
        }
        return;
    }
    struct ASTLinkedNode *temp, *child = command->val.children;
    int offset;
    switch(child->val.type) {

    case CONST_DECL:
        return;
    case VAR_DECL:
        if (!child->val.children->next) {
            return;
        }
        codegenExpr(child->val.children->next, 0);
        offset = child->val.frameIndex*4;
        if (child->val.isParam) offset += frameArgOffset;
        fprintf(stdout, "st r0, %d(r5)\n", offset + entireFrameOffset);
        return;
    case IF_EXPR:
        codegenIf(child);
        return;
    case WHILE_LOOP:
        codegenWhileLoop(child);
        return;
    case COMMAND:
        for (temp = child->val.children; temp != NULL; temp = temp->next) {
            codegenSingleCommand(temp);
            if (!isInFn) break;
        }
        return;
    case DIRECT_ASSIGN:
        codegenDirectAssign(child);
        return;
    case INDIRECT_ASSIGN:
        codegenExpr(child->val.children, 0);
        codegenExpr(child->val.children->next, 1);
        fputs("st r1, (r0)\n", stdout);
        break;
    case FUNC_CALL:
        codegenFuncCall(child, 0);
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
    if (regDest != 0) {
        fputs("deca r5\t\t# Save r0\nst r0, (r5)\n\n", stdout);
        entireFrameOffset += 4;
    }
    if (call->val.children->val.definition->val.paramCount > 0) {
        fprintf(stdout, "ld $-%d, r0\t\t# alloc args\nadd r0, r5\n\n", 4*call->val.children->val.definition->val.paramCount);
        entireFrameOffset += 4*call->val.children->val.definition->val.paramCount;
    }
    int i = 0;
    for (temp = call->val.children->next->val.children; temp != NULL; temp = temp->next) {
        codegenExpr(temp, 0);
        fprintf(stdout, "st r0, %d(r5)\n", i++*4);
    }
    char *name = calloc(call->val.children->val.endIndex - call->val.children->val.startIndex + 1, sizeof(char));
    getInputSubstr(name, call->val.children->val.startIndex, call->val.children->val.endIndex);
    fprintf(stdout, "gpc $6, r6\nj %s\n", name);
    free(name);
    if(regDest != 0) {
        fprintf(stdout, "mov r0, r%d\n", regDest);
        fputs("ld (r5), r0\t\t# restore r0\ninca r5\n\n", stdout);
        entireFrameOffset -= 4;
    }
    if (call->val.children->val.definition->val.paramCount > 0) {
        fprintf(stdout, "ld $%d, r0\t\t# dealloc args\nadd r0, r5\n\n", 4*call->val.children->val.definition->val.paramCount);
        entireFrameOffset -= 4*call->val.children->val.definition->val.paramCount;
    }
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
    int offset = varref->val.definition->val.frameIndex*4;
    if (varref->val.definition->val.isParam) offset += frameArgOffset;
    fprintf(stdout, "ld %d(r5), r%d\n", offset + entireFrameOffset, regDest);
    return;
}

static void codegenWhileLoop(struct ASTLinkedNode *loop)
{
    // We always use j instead of br to avoid issues with labels being too far apart
    int number = uniqueNum++;
    fprintf(stdout, "L%dS:\n", number);
    codegenExpr(loop->val.children, 0);
    fprintf(stdout, "beq r0, L%dEInter\n", number);
    fprintf(stdout, "br L%dEInterEnd\n", number);
    fprintf(stdout, "L%dEInter:\n", number);
    fprintf(stdout, "j L%dE\n", number);
    fprintf(stdout, "L%dEInterEnd:\n", number);
    codegenSingleCommand(loop->val.children->next);
    fprintf(stdout, "j L%dS\n", number);
    fprintf(stdout, "L%dE:\n", number);
}

static void codegenIf(struct ASTLinkedNode *ifExpr)
{
    // We always use j instead of br to avoid issues with labels being too far apart
    int number = uniqueNum++;
    codegenExpr(ifExpr->val.children, 0);
    fprintf(stdout, "beq r0, ELSE%dSInter\n", number);
    fprintf(stdout, "br ELSE%dSInterEnd\n", number);
    fprintf(stdout, "ELSE%dSInter:\nj ELSE%dS\nELSE%dSInterEnd:\n", number, number, number);
    codegenSingleCommand(ifExpr->val.children->next);
    if (ifExpr->val.children->next->next) {
        fprintf(stdout, "j ELSE%dE\n", number);
    }
    fprintf(stdout, "ELSE%dS:\n", number);
    if (ifExpr->val.children->next->next) {
        codegenSingleCommand(ifExpr->val.children->next->next);
        fprintf(stdout, "ELSE%dE:\n", number);
    }
}

static void codegenDirectAssign(struct ASTLinkedNode *assignment)
{
    codegenExpr(assignment->val.children->next, 0);
    if (assignment->val.children->val.definition->val.isStatic) {
        struct ASTLinkedNode *ident = assignment->val.children->val.definition->val.children;
        char *name = calloc(ident->val.endIndex - ident->val.startIndex + 1, sizeof(char));
        getInputSubstr(name, ident->val.startIndex, ident->val.endIndex);
        fprintf(stdout, "ld $%s, r1\nst r0, (r1)\n", name);
        free(name);
        return;
    }
    int offset = assignment->val.children->val.definition->val.frameIndex*4;
    if (assignment->val.children->val.definition->val.isParam) offset += frameArgOffset;
    fprintf(stdout, "st r0, %d(r5)\n", offset + entireFrameOffset);
}

/*
 * Generates asm to compute expression and store result in regDest
 * Preserves all regs < regDest, assumes regs >= regDest can be modified at will
 * r5, r6 are treated specially and are never clobbered.
 * REQUIRES: regDest is not 7. We need at least 2 regs to work with.
*/
static void codegenExpr(struct ASTLinkedNode *expr, int regDest)
{
    if (expr->val.type == NUMBER_LITERAL) {
        fprintf(stdout, "ld $%d, r%d\n", expr->val.val, regDest);
        return;
    } else if (expr->val.type == FUNC_CALL) {
        codegenFuncCall(expr, regDest);
        return;
    } else if (expr->val.type == IDENT_REF) {
        codegenIdentRef(expr, regDest);
        return;
    } else if (isInfix(expr->val.operationType) && (expr->val.children->val.isConstant || expr->val.children->next->val.isConstant)) {
        // FOR TESTING PURPOSES! WILL BE REMOVED!
        if (expr->val.operationType == RIGHT_SHIFT && expr->val.children->next->val.isConstant) {
            codegenExpr(expr->val.children, regDest);
            fprintf(stdout, "shr $%d, r%d\n", expr->val.children->next->val.val, regDest);
            return;
        } else if (expr->val.operationType == LEFT_SHIFT && expr->val.children->next->val.isConstant) {
            codegenExpr(expr->val.children, regDest);
            fprintf(stdout, "shl $%d, r%d\n", expr->val.children->next->val.val, regDest);
            return;
        }
    }

    int left = regDest;
    int right = regDest + 1;
    if (isInfix(expr->val.operationType)) {
        if (regDest >= 4) {
            codegenExpr(expr->val.children, left);
            fprintf(stdout, "deca r5\nst r%d (r5)\n", left);
            entireFrameOffset += 4;
            codegenExpr(expr->val.children->next, left);
            fprintf(stdout, "mov r%d, r7\n", left);
            fprintf(stdout, "ld (r5), r%d\ninca r5\n", left);
            entireFrameOffset -= 4;
            codegenOperation(expr->val.operationType, left, 7);
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
        fprintf(stdout, "beq r%d, C%dS\nld $0, r%d\nbr C%dE\nC%dS: ld $1, r%d\nC%dE:\n",
            reg, uniqueNum, reg, uniqueNum, uniqueNum, reg, uniqueNum);
        uniqueNum++;
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
 * Computes operation and stores in left.
 * CLOBBERS *BOTH* left and right.
 * TODO: handle operation w/ one of left, right an integer literal in different function
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
        codegenDynamicMultiplication(left, right);
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
        // we assume right is at most 32. Otherwise, this is equivilant to only looking at rightmost 5 bit.
        // TODO:
		fprintf(stdout, "add r%d, r%d\n", right, left);
		return;
	case LESS_THAN:
        codegenOperation(MINUS, right, left);
        fprintf(stdout, "bgt r%d, C%dS\nld $0, r%d\nbr C%dE\nC%dS: ld $1, r%d\nC%dE:\n",
            right, uniqueNum, left, uniqueNum, uniqueNum, left, uniqueNum);
        uniqueNum++;
        return;
	case LESS_THAN_EQUALS:
        codegenOperation(MINUS, right, left);
        fprintf(stdout, "bgt r%d, C%dS\nbe r%d, C%dS\nld $0, r%d\nbr C%dE\nC%dS: ld $1, r%d\nC%dE:\n",
            right, uniqueNum, right, uniqueNum, left, uniqueNum, uniqueNum, left, uniqueNum);
        uniqueNum++;
        return;
	case GREATER_THAN:
        codegenOperation(MINUS, left, right);
        fprintf(stdout, "bgt r%d, C%dS\nld $0, r%d\nbr C%dE\nC%dS: ld $1, r%d\nC%dE:\n",
            left, uniqueNum, left, uniqueNum, uniqueNum, left, uniqueNum);
        uniqueNum++;
        return;
	case GREATER_THAN_EQUALS:
        codegenOperation(MINUS, left, right);
        fprintf(stdout, "bgt r%d, C%dS\nbe r%d, C%dS\nld $0, r%d\nbr C%dE\nC%dS: ld $1, r%d\nC%dE:\n",
            left, uniqueNum, left, uniqueNum, left, uniqueNum, uniqueNum, left, uniqueNum);
        uniqueNum++;
        return;
	case EQUALS:
        codegenOperation(MINUS, left, right);
		fprintf(stdout, "beq r%d, C%dS\nld $0, r%d\nbr C%dE\nC%dS: ld $1, r%d\nC%dE:\n",
            left, uniqueNum, left, uniqueNum, uniqueNum, left, uniqueNum);
        uniqueNum++;
		return;
	case NOT_EQUALS:
        codegenOperation(MINUS, left, right);
		fprintf(stdout, "beq r%d, C%dS\nld $1, r%d\nbr C%dE\nC%dS: ld $0, r%d\nC%dE:\n",
            left, uniqueNum, left, uniqueNum, uniqueNum, left, uniqueNum);
        uniqueNum++;
		return;
	case OR:
        fprintf(stdout, "beq r%d, C%dS\nbeq r%d, C%dS\nld $0, r%d\nbr C%dE\nC%dS:ld $1, r%d\nC%dE:\n",
            left, uniqueNum, right, uniqueNum, left, uniqueNum, uniqueNum, left, uniqueNum);
        uniqueNum++;
		return;
	case AND:
        fprintf(stdout, "beq r%d, C%dS\nbeq r%d, C%dS\nld $1, r%d\nbr C%dE\nC%dS:\nld $0, r%d\nC%dE:\n",
            left, uniqueNum, right, uniqueNum, left, uniqueNum, uniqueNum, left, uniqueNum);
        uniqueNum++;
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

/*
 * Calculates left * right, stores result in left
*/
static void codegenDynamicMultiplication(int left, int right)
{
    // we need 2 extra regs for this - we use r6, and either r7 or r4 or r1
    int tempReg;
    if (left != 7 && right != 7) {
        tempReg = 7;
    } else if (left != 4 && right != 4) {
        tempReg = 4;
    } else {
        tempReg = 1;
    }


    fprintf(stdout, "deca r5\nst r6, (r5)\ndeca r5\nst r%d, (r5)\n", tempReg);
    fprintf(stdout, "mov r%d, r%d\n", left, tempReg);
    fprintf(stdout, "ld $0, r%d\n", left);
    fprintf(stdout, "L%d:\n", uniqueNum);
    fprintf(stdout, "beq r%d, L%dE\n", right, uniqueNum);
    fprintf(stdout, "ld $1, r6\nand r%d, r6\nbeq r6, L%dC\nadd r%d, r%d\n", right, uniqueNum, tempReg, left);
    fprintf(stdout, "L%dC:\nshr $1, r%d\n shl $1, r%d\n", uniqueNum, right, tempReg);
    fprintf(stdout, "br L%d\n", uniqueNum);
    fprintf(stdout, "L%dE:\n", uniqueNum);
    fprintf(stdout, "ld (r5), r%d\ninca r5\nld (r5), r6\ninca r5\n", tempReg);
    uniqueNum++;
}