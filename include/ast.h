#ifndef AST_H
#define AST_H

#include <stdio.h>
#include <stdlib.h>

#include "defs.h"
#include "scan.h"
#include "sym.h"

// used for debugging
#define NO_LABEL -1

struct astnode {
    enum ASTOP op;
    enum ASTPRIM type;
    struct astnode *left;
    struct astnode *mid;
    struct astnode *right;
    // Boolean but will be used as a bitfield later on
    int rvalue;
    union {
        int intvalue; // for INTLIT
        int id; // Var lookup
        int size; // for A_SCALE: size to scale
    } v;
};

typedef struct astnode *ASTnode;


ASTnode ASTnode_New(enum ASTOP op, enum ASTPRIM type, ASTnode left, ASTnode mid,
                    ASTnode right, int intvalue);
void ASTnode_Free(ASTnode this);
ASTnode ASTnode_NewLeaf(enum ASTOP op, enum ASTPRIM type, int intvalue);
ASTnode ASTnode_NewUnary(enum ASTOP op, enum ASTPRIM type, ASTnode left,
                         int intvalue);
void ASTnode_Dump(ASTnode n, SymTable st, int label, int level);
#endif