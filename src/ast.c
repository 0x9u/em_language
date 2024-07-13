#include "ast.h"

ASTnode ASTnode_New(enum ASTOP op, enum ASTPRIM type, ASTnode left, ASTnode mid,
                    ASTnode right, int intvalue) {
    ASTnode n = calloc(1, sizeof(struct astnode));
    if (n == NULL) {
        fprintf(stderr, "Error: Unable to initialise ASTnode\n");
        exit(-1);
    }

    n->op = op;
    n->type = type;
    n->left = left;
    n->mid = mid;
    n->right = right;
    n->v.intvalue = intvalue;

    return n;
}

ASTnode ASTnode_NewLeaf(enum ASTOP op, enum ASTPRIM type, int intvalue) {
    return ASTnode_New(op, type, NULL, NULL, NULL, intvalue);
}

ASTnode ASTnode_NewUnary(enum ASTOP op, enum ASTPRIM type, ASTnode left,
                         int intvalue) {
    return ASTnode_New(op, type, left, NULL, NULL, intvalue);
}

void ASTnode_Free(ASTnode this) {
    if (this->left) ASTnode_Free(this->left);
    if (this->right) ASTnode_Free(this->right);
    free(this);
}

static int makeLabel(void) {
    static int id = 1;
    return id++;
}

void ASTnode_Dump(ASTnode n, SymTable st, int label, int level) {
    int Lfalse, Lstart, Lend;
    switch (n->op) {
        case A_IF:
            Lfalse = makeLabel();
            for (int i = 0; i < level; i++) printf(" ");
            printf("A_IF");
            if (n->right) {
                Lend = makeLabel();
                printf("end L%d", Lend);
            }
            printf("\n");
            ASTnode_Dump(n->left, st, Lfalse, level + 2);
            ASTnode_Dump(n->mid, st, NO_LABEL, level + 2);
            if (n->right) {
                ASTnode_Dump(n->right, st, NO_LABEL, level + 2);
            }
            return;
        case A_WHILE:
            Lstart = makeLabel();
            for (int i = 0; i < level; i++) printf("  ");
            printf("A_WHILE start L%d\n", Lstart);
            Lend = makeLabel();
            ASTnode_Dump(n->left, st, Lend, level + 2);
            ASTnode_Dump(n->right, st, NO_LABEL, level + 2);
            return;
        default:
            break;
    }

    if (n->op == A_GLUE) level = -2;
    if (n->left) ASTnode_Dump(n->left, st, NO_LABEL, level + 2);
    if (n->right) ASTnode_Dump(n->right, st, NO_LABEL, level + 2);

    for (int i = 0; i < level; i++) printf(" ");
    switch (n->op) {
        case A_GLUE:
            printf("\n\n");
            return;
        case A_FUNCTION:
            printf("A_FUNCTION %s\n", st->Gsym[n->v.id].name);
            return;
        case A_ADD:
            printf("A_ADD\n");
            return;
        case A_SUBTRACT:
            printf("A_SUBTRACT\n");
            return;
        case A_MULTIPLY:
            printf("A_MULTIPLY\n");
            return;
        case A_DIVIDE:
            printf("A_DIVIDE\n");
            return;
        case A_MODULO:
            printf("A_MODULO\n");
            return;
        case A_EQ:
            printf("A_EQ\n");
            return;
        case A_NE:
            printf("A_NE\n");
            return;
        case A_LT:
            printf("A_LT\n");
            return;
        case A_GT:
            printf("A_GT\n");
            return;
        case A_LE:
            printf("A_LE\n");
            return;
        case A_GE:
            printf("A_GE\n");
            return;
        case A_INTLIT:
            printf("A_INTLIT %d\n", n->v.intvalue);
            return;
        case A_IDENT:
            if (n->rvalue)
                printf("A_IDENT rval %s type %d\n", st->Gsym[n->v.id].name,
                       n->type);
            else
                printf("A_IDENT %s type %d\n", st->Gsym[n->v.id].name,
                       n->type);
            return;
        case A_ASSIGN:
            printf("A_ASSIGN type %d\n", n->type);
            return;
        case A_PRINT:
            printf("A_PRINT\n");
            return;
        case A_INPUT:
            printf("A_INPUT\n");
            return;
        case A_LABEL:
            printf("A_LABEL %s\n", st->Gsym[n->v.id].name);
            return;
        case A_GOTO:
            printf("A_GOTO %s\n", st->Gsym[n->v.id].name);
            return;
        case A_RETURN:
            printf("A_RETURN type %d\n", n->type);
            return;
        case A_WIDEN:
            printf("A_WIDEN type %d\n", n->type);
            return;
        case A_FUNCCALL:
            printf("A_FUNCCALL %s\n", st->Gsym[n->v.id].name);
            return;
        case A_ADDR:
            printf("A_ADDR %s\n", st->Gsym[n->v.id].name);
            return;
        case A_DEREF:
            if (n->rvalue)
                printf("A_DEREF rval\n");
            else
                printf("A_DEREF\n");
            return;
        case A_SCALE:
            printf("A_SCALE %d\n", n->v.size);
            return;
        case A_STRLIT:
            printf("A_STRLIT %s\n", st->Gsym[n->v.id].name);
            return;
        default:
            fprintf(stderr, "Error: Unknown AST node %d\n", n->op);
            exit(-1);
    }
}