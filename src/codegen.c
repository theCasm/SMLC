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
static void codegenInfixOperation(struct ASTLinkedNode *expr, int regDest);
static void codegenPrefixOperation(struct ASTLinkedNode *expr, int reg);
static void codegenMinus(int left, int right);
static void codegenDivide(int left, int right);
static void codegenModulus(int left, int right);
static void codegenLeftShift(int left, int right);
static void codegenRightShift(int left, int right);
static void codegenNotEquals(int left, int right);
static void codegenOr(int left, int right);
static void codegenAnd(int left, int right);
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
    }

    if (isInfix(expr->val.operationType)) {
        codegenInfixOperation(expr, regDest);
        return;
    }
    codegenPrefixOperation(expr, regDest);
}

/*
 * generates code to compute prefix operation
*/
static void codegenPrefixOperation(struct ASTLinkedNode *expr, int destReg)
{
    codegenExpr(expr->val.children, destReg);
    switch (expr->val.operationType) {
    case MINUS:
        fprintf(stdout, "not r%d\ninc r%d\n", destReg, destReg);
        return;
    case BITWISE_NOT:
        fprintf(stdout, "not r%d\n", destReg);
        return;
    case NOT:
        fprintf(stdout, "beq r%d, C%dS\nld $0, r%d\nbr C%dE\nC%dS: ld $1, r%d\nC%dE:\n",
            destReg, uniqueNum, destReg, uniqueNum, uniqueNum, destReg, uniqueNum);
        uniqueNum++;
        return;
    case DEREF:
        fprintf(stdout, "ld (r%d), r%d\n", destReg, destReg);
        return;
    default:
        fprintf(stderr, "CODEGEN: idk how to fold in prefix %s\n", TokenStrings[expr->val.type]);
		return;
    }
}

/*
 * Computes operation and stores in left.
 * CLOBBERS *BOTH* left and right.
*/
static void codegenInfixOperation(struct ASTLinkedNode *expr, int destReg)
{
    // TODO: handle operation w/ one of left, right an integer literal in different function

	// These we do now because of how horrible the dynamic versions are
	if (expr->val.children->next->val.isConstant) {
		if (expr->val.operationType == LEFT_SHIFT) {
			codegenExpr(expr->val.children, destReg);
			fprintf(stdout, "shl $%d, r%d\n", expr->val.children->next->val.val, destReg);
			return;
		} else if (expr->val.operationType == RIGHT_SHIFT) {
			codegenExpr(expr->val.children, destReg);
			fprintf(stdout, "shr $%d, r%d\n", expr->val.children->next->val.val, destReg);
			return;
		}
	}

	codegenExpr(expr->val.children, destReg);
    int right = destReg + 1;
    if (destReg >= 4) {
        fprintf(stdout, "deca r5\nst r%d (r5)\n", destReg);
        entireFrameOffset += 4;
        codegenExpr(expr->val.children->next, destReg);
        fprintf(stdout, "mov r%d, r7\n", destReg);
        fprintf(stdout, "ld (r5), r%d\ninca r5\n", destReg);
        entireFrameOffset -= 4;
        right = 7;
    } else {
		codegenExpr(expr->val.children->next, right);
	}
    switch (expr->val.operationType) {
	case PLUS:	
        fprintf(stdout, "add r%d, r%d\n", right, destReg);
		return;
	case MINUS:
		codegenMinus(destReg, right);
		return;
	case TIMES:
        codegenDynamicMultiplication(destReg, right);
		return;
	case DIVIDE:
        codegenDivide(destReg, right);
		return;
	case MODULO:
        codegenModulus(destReg, right);
		return;
	case LEFT_SHIFT:
		codegenLeftShift(destReg, right);
		return;
	case RIGHT_SHIFT:
		codegenRightShift(destReg, right);
		return;
	case LESS_THAN:
        codegenMinus(destReg, right);
        fprintf(stdout, "bgt r%d, C%dS\nld $0, r%d\nbr C%dE\nC%dS: ld $1, r%d\nC%dE:\n",
            right, uniqueNum, destReg, uniqueNum, uniqueNum, destReg, uniqueNum);
        uniqueNum++;
        return;
	case LESS_THAN_EQUALS:
        codegenMinus(destReg, right);
        fprintf(stdout, "bgt r%d, C%dS\nbe r%d, C%dS\nld $0, r%d\nbr C%dE\nC%dS: ld $1, r%d\nC%dE:\n",
            right, uniqueNum, right, uniqueNum, destReg, uniqueNum, uniqueNum, destReg, uniqueNum);
        uniqueNum++;
        return;
	case GREATER_THAN:
        codegenMinus(destReg, right);
        fprintf(stdout, "bgt r%d, C%dS\nld $0, r%d\nbr C%dE\nC%dS: ld $1, r%d\nC%dE:\n",
            destReg, uniqueNum, destReg, uniqueNum, uniqueNum, destReg, uniqueNum);
        uniqueNum++;
        return;
	case GREATER_THAN_EQUALS:
        codegenMinus(destReg, right);
        fprintf(stdout, "bgt r%d, C%dS\nbe r%d, C%dS\nld $0, r%d\nbr C%dE\nC%dS: ld $1, r%d\nC%dE:\n",
            destReg, uniqueNum, destReg, uniqueNum, destReg, uniqueNum, uniqueNum, destReg, uniqueNum);
        uniqueNum++;
        return;
	case EQUALS:
        codegenMinus(destReg, right);
		fprintf(stdout, "beq r%d, C%dS\nld $0, r%d\nbr C%dE\nC%dS: ld $1, r%d\nC%dE:\n",
            destReg, uniqueNum, destReg, uniqueNum, uniqueNum, destReg, uniqueNum);
        uniqueNum++;
		return;
	case NOT_EQUALS:
        codegenNotEquals(destReg, right);
		return;
	case OR:
        codegenOr(destReg, right);
		return;
	case AND:
        codegenAnd(destReg, right);
		return;
	case BITWISE_AND:
		fprintf(stdout, "and r%d, r%d\n", right, destReg);
		return;
	case BITWISE_OR:
        fprintf(stdout, "not r%d\nnot r%d\nand r%d, r%d\nnot r%d\n",
			destReg, right, right, destReg, destReg);
		return;
	case BITWISE_XOR:
		// a + b = a (+) b + carry = a (+) b + (a ^ b) << 1
		// ==> a (+) b = a + b - (a ^ b) << 1
		fputs("deca r5\nst r6 (r5)\n", stdout);
		fprintf(stdout, 
			"mov r%d, r6\n"
			"and r%d, r6\n"
			"shl $1, r6\n"
			"not r6\n"
			"inc r6\n"
			"add r%d, r%d\n"
			"add r6, r%d\n",
			right, destReg, right, destReg, destReg);
		fputs("ld (r5) r6\ninca r5\n", stdout);
		return;
	default:
		fprintf(stderr, "CODEGEN: idk how to fold in %s\n", TokenStrings[expr->val.type]);
		return;
	}
}

static void codegenMinus(int left, int right)
{
    fprintf(stdout, 
        "not r%d\n"
        "inc r%d\n"
        "add r%d, r%d\n",
        right, right, right, left);
}

static void codegenDivide(int left, int right)
{
	/*
	 * We need 3 helper regs - division is hard enough when you can decide the hardware!
	 * This is a modified non-restorative division algorithm given we only have 32 bit regs
	 * and want integer division.
	 */

	int count, negativeDivisor, result;
	count = 6;
	if (right != 7) {
		negativeDivisor = 7;
		if (left != 4 && right != 4) {
			result = 4;
		} else if (left != 3 && right != 3) {
			result = 3;
		} else {
			result = 2;
		}
	} else if (left != 4) {
		negativeDivisor = 4;
		if (left != 3) {
			result = 3;
		} else {
			result = 2;
		}
	} else {
		negativeDivisor = 3;
		result = 2;
	}
	fprintf(stdout,
		"deca r5\n"
		"st r%d, (r5)\n"
		"deca r5\n"
		"st r%d, (r5)\n"
		"deca r5\n"
		"st r%d, (r5)\n",
		count, negativeDivisor, result);
	// first, we shift the divisor so everything lines up
	fprintf(stdout,
		"ld $1, r%d\n"
		"ld $0, r%d\n"
		"L%d1S:\n"
		"mov r%d, r%d\n"
		"not r%d\n"
		"inc r%d\n"
		"add r%d, r%d\n"
		"bgt r%d, L%d1E\n"
		"inc r%d\n"
		"shl $1, r%d\n"
		"br L%d1S\n"
		"L%d1E:\n",
		count, result, uniqueNum, left, negativeDivisor, negativeDivisor, negativeDivisor, right, negativeDivisor,
		negativeDivisor, uniqueNum, count, right, uniqueNum, uniqueNum);
	// now, we do the division
	fprintf(stdout,
		"mov r%d, r%d\n"
		"not r%d\n"
		"inc r%d\n"
		"L%d2S:\n"
		"beq r%d, L%d2E\n"
		"dec r%d\n"
		"shl $1, r%d\n"
		"bgt r%d, L%d2C\n"
		"beq r%d, L%d2C\n"
		"add r%d, r%d\n"
		"dec r%d\n"
		"br L%d2CE\n"
		"L%d2C:\n"
		"add r%d, r%d\n"
		"inc r%d\n"
		"L%d2CE:\n"
		"shl $1, r%d\n"
		"br L%d2S\n"
		"L%d2E:\n",
		right, negativeDivisor, negativeDivisor, negativeDivisor, uniqueNum, count, uniqueNum, count, result,
        left, uniqueNum, left, uniqueNum, right, left, result, uniqueNum, uniqueNum, negativeDivisor, left,
        result, uniqueNum, left, uniqueNum, uniqueNum);
    // finally, we adjust the result. non-res can choose a negative remainder for integer division - we fix that here
    fprintf(stdout,
        "bgt r%d, C%d\n"
        "beq r%d, C%d\n"
        "dec r%d\n"
        "C%d:\n"
        "mov r%d, r%d\n",
        left, uniqueNum, left, uniqueNum, result, uniqueNum, result, left);
	fprintf(stdout,
		"ld (r5), r%d\n"
		"inca r5\n"
		"ld (r5), r%d\n"
		"inca r5\n"
		"ld (r5), r%d\n"
		"inca r5\n",
		result, negativeDivisor, count);
    uniqueNum++;
}


static void codegenModulus(int left, int right)
{
    /*
     * Modified from codegenDiv 
    */
   int count, negativeDivisor;
	count = 6;
	if (right != 7) {
		negativeDivisor = 7;
	} else if (left != 4) {
		negativeDivisor = 4;
	} else {
		negativeDivisor = 3;
	}
	fprintf(stdout,
		"deca r5\n"
		"st r%d, (r5)\n"
		"deca r5\n"
		"st r%d, (r5)\n",
		count, negativeDivisor);
	// first, we shift the divisor so everything lines up
	fprintf(stdout,
		"ld $1, r%d\n"
		"L%d1S:\n"
		"mov r%d, r%d\n"
		"not r%d\n"
		"inc r%d\n"
		"add r%d, r%d\n"
		"bgt r%d, L%d1E\n"
		"inc r%d\n"
		"shl $1, r%d\n"
		"br L%d1S\n"
		"L%d1E:\n",
		count, uniqueNum, left, negativeDivisor, negativeDivisor, negativeDivisor, right, negativeDivisor,
		negativeDivisor, uniqueNum, count, right, uniqueNum, uniqueNum);
	// now, we do the division
	fprintf(stdout,
		"mov r%d, r%d\n"
		"not r%d\n"
		"inc r%d\n"
		"L%d2S:\n"
		"beq r%d, L%d2E\n"
		"dec r%d\n"
		"bgt r%d, L%d2C\n"
		"beq r%d, L%d2C\n"
		"add r%d, r%d\n"
		"br L%d2CE\n"
		"L%d2C:\n"
		"add r%d, r%d\n"
		"L%d2CE:\n"
		"shr $1, r%d\n"
        "shr $1, r%d\n"
		"br L%d2S\n"
		"L%d2E:\n",
		right, negativeDivisor, negativeDivisor, negativeDivisor, uniqueNum, count, uniqueNum, count,
        left, uniqueNum, left, uniqueNum, right, left, uniqueNum, uniqueNum, negativeDivisor, left,
        uniqueNum, right, negativeDivisor, uniqueNum, uniqueNum);
    // finally, we adjust the result. non-res can choose a negative remainder for integer division - we fix that here
    fprintf(stdout,
        "bgt r%d, C%d\n"
        "beq r%d, C%d\n"
        "add r1, r0\n"
        "C%d:\n",
        left, uniqueNum, left, uniqueNum, uniqueNum);
	fprintf(stdout,
		"ld (r5), r%d\n"
		"inca r5\n"
		"ld (r5), r%d\n"
		"inca r5\n",
		negativeDivisor, count);
    uniqueNum++;
}

/*
 * I am of the opinion that allowing a register input for shl would
 * be worth it. This is hell.
*/
static void codegenLeftShift(int left, int right)
{
	fputs("deca r5\nst r6 (r5)\n", stdout);
	fprintf(stdout,
		"ld $-31, r6\n"
		"add r%d, r6\n"
		"bgt bigshl%d\n"
		"br smallshl%d\n"
		"bigshl%d:\n"
		"ld $0, r%d\n"
		"br LSH%d32\n"
		"smallshl%d:\n"
		"ld $1, r6\n"
		"and r%d, r6\n"
		"shr $1, r%d\n"
		"beq r6, LSH%d2\n"
		"shl $1, r%d\n"
		"LSH%d2:\n"
		"ld $1, r6\n"
		"and r%d, r6\n"
		"shr $1, r%d\n"
		"beq r6, LSH%d4\n"
		"shl $2, r%d\n"
		"LSH%d4:\n"
		"ld $1, r6\n"
		"and r%d, r6\n"
		"shr $1, r%d\n"
		"beq r6, LSH%d8\n"
		"shl $4, r%d\n"
		"LSH%d8:\n"
		"ld $1, r6\n"
		"and r%d, r6\n"
		"shr $1, r%d\n"
		"beq r6, LSH%d16\n"
		"shl $8, r%d\n"
		"LSH%d16:\n"
		"ld $1, r6\n"
		"and r%d, r6\n"
		"shr $1, r%d\n"
		"beq r6, LSH%d16\n"
		"shl $16, r%d\n"
		"LSH%d32:\n",
		right, uniqueNum, uniqueNum, uniqueNum, left, uniqueNum, uniqueNum, right, right,
		uniqueNum, left, uniqueNum, right, right, uniqueNum, left, uniqueNum,
		right, right, uniqueNum, left, uniqueNum, right, right, uniqueNum, left,
		uniqueNum, right, right, uniqueNum, left, uniqueNum);
	uniqueNum++;
	fputs("ld (r5) r6\ninca r5\n", stdout);
}

static void codegenRightShift(int left, int right)
{
	fputs("deca r5\nst r6 (r5)\n", stdout);
	fprintf(stdout,
		"ld $-31, r6\n"
		"add r%d, r6\n"
		"bgt bigshr%d\n"
		"br smallshr%d\n"
		"bigshr%d:\n"
		"ld $0, r%d\n"
		"br LSH%d32\n"
		"smallshr%d:\n"
		"ld $1, r6\n"
		"and r%d, r6\n"
		"shr $1, r%d\n"
		"beq r6, RSH%d2\n"
		"shr $1, r%d\n"
		"RSH%d2:\n"
		"ld $1, r6\n"
		"and r%d, r6\n"
		"shr $1, r%d\n"
		"beq r6, RSH%d4\n"
		"shr $2, r%d\n"
		"RSH%d4:\n"
		"ld $1, r6\n"
		"and r%d, r6\n"
		"shr $1, r%d\n"
		"beq r6, RSH%d8\n"
		"shr $4, r%d\n"
		"RSH%d8:\n"
		"ld $1, r6\n"
		"and r%d, r6\n"
		"shr $1, r%d\n"
		"beq r6, RSH%d16\n"
		"shr $8, r%d\n"
		"RSH%d16:\n"
		"ld $1, r6\n"
		"and r%d, r6\n"
		"shr $1, r%d\n"
		"beq r6, RSH%d16\n"
		"shr $16, r%d\n"
		"RSH%d32:\n",
		right, uniqueNum, uniqueNum, uniqueNum, left, uniqueNum, uniqueNum, right, right,
		uniqueNum, left, uniqueNum, right, right, uniqueNum, left, uniqueNum,
		right, right, uniqueNum, left, uniqueNum, right, right, uniqueNum, left,
		uniqueNum, right, right, uniqueNum, left, uniqueNum);
	uniqueNum++;
	fputs("ld (r5) r6\ninca r5\n", stdout);
}

static void codegenNotEquals(int left, int right)
{
    codegenMinus(left, right);
    fprintf(stdout,
        "beq r%d, C%dS\n"
        "ld $1, r%d\n"
        "br C%dE\n"
        "C%dS: ld $0, r%d\n"
        "C%dE:\n",
        left, uniqueNum, left, uniqueNum, uniqueNum, left, uniqueNum);
    uniqueNum++;
}

static void codegenOr(int left, int right)
{
    fprintf(stdout,
        "beq r%d, C%dS\n"
        "beq r%d, C%dS\n"
        "ld $0, r%d\n"
        "br C%dE\n"
        "C%dS:ld $1, r%d\n"
        "C%dE:\n",
        left, uniqueNum, right, uniqueNum, left, uniqueNum, uniqueNum, left, uniqueNum);
    uniqueNum++;
}

static void codegenAnd(int left, int right)
{
    fprintf(stdout,
        "beq r%d, C%dS\n"
        "beq r%d, C%dS\n"
        "ld $1, r%d\n"
        "br C%dE\n"
        "C%dS:\n"
        "ld $0, r%d\n"
        "C%dE:\n",
        left, uniqueNum, right, uniqueNum, left, uniqueNum, uniqueNum, left, uniqueNum);
    uniqueNum++;
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