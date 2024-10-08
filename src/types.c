#include "types.h"

#include <stdio.h>

#include "asm.h"
#include "ast.h"
#include "defs.h"
#include "misc.h"

bool inttype(enum ASTPRIM type) {
    return (type & 0xf) == 0 && type >= P_CHAR && type <= P_INT;
}

bool ptrtype(enum ASTPRIM type) { return (type & 0xf) != 0; }

int type_size(enum ASTPRIM type, SymTableEntry cType) {
    if (type == P_STRUCT || type == P_UNION) return cType->size;
    return PrimSize(type);
}

ASTnode modify_type(ASTnode tree, enum ASTPRIM rtype, SymTableEntry rctype,
                    enum ASTOP op) {
    enum ASTPRIM ltype;
    int lsize, rsize;

    ltype = tree->type;

    if (op == A_LOGOR || op == A_LOGAND) {
        if (!inttype(ltype) && !ptrtype(ltype)) return NULL;
        if (!inttype(rtype) && !ptrtype(rtype)) return NULL;
        return tree;
    }

    if (ltype == P_STRUCT || ltype == P_UNION) {
        fatal("InternalError: Struct/Union type in modify_type");
    }
    if (rtype == P_STRUCT || rtype == P_UNION) {
        fatal("InternalError: Struct/Union type in modify_type");
    }

    // TODO: Change this and see if anything breaks
    if ((inttype(ltype) && inttype(rtype))) {
        if (ltype == rtype) return tree;
        // TODO: support structs later on

        lsize = type_size(ltype, NULL);
        rsize = type_size(rtype, NULL);
        if (lsize > rsize) {
            return NULL;
        } else if (lsize < rsize) {
            return ASTnode_NewUnary(A_WIDEN, ltype, tree, NULL, NULL, 0);
        }
    }

    if (ptrtype(ltype) && ptrtype(rtype)) {
        debug("Comparing two pointers");
        if (op >= A_EQ && op <= A_GE) {
            return tree;
        }

        debug("op %d ltype %d, rtype %d, pointer_to(P_VOID) %d", op, ltype,
              rtype, pointer_to(P_VOID));

        if (op == A_NONE && (ltype == rtype || ltype == pointer_to(P_VOID))) {
            return tree;
        }
    }

    if (op == A_ADD || op == A_SUBTRACT) {
        // I don't know why but the types are swapped
        // Pointer -> left, Value -> right

        if (inttype(ltype) && ptrtype(rtype)) {
            // can be compared

            rsize = PrimSize(value_at(rtype));
            if (rsize > 1) {
                return ASTnode_NewUnary(A_SCALE, rtype, tree, rctype, NULL,
                                        rsize);
            } else {
                return tree;
            }
        }
    }

    // Types are incompatible
    return NULL;
}

enum ASTPRIM pointer_to(enum ASTPRIM type) {
    if ((type & 0xf) == 0xf) {
        fatala("InternalError: Unknown type %d for pointer", type);
    }
    return type + 1;
}

enum ASTPRIM value_at(enum ASTPRIM type) {
    if ((type & 0xf) == 0) {
        fatala("InternalError: Unknown type %d for value_at", type);
    }
    return type - 1;
}