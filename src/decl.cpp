#include <stdlib.h>
#include <string.h>

#include "decl.h"
#include "defs.h"
#include "flags.h"
#include "misc.h"
#include "opt.h"

static SymTableEntry composite_declare(Compiler c, Scanner s, SymTable st,
                                       Token tok, Context ctx,
                                       enum ASTPRIM type);

static void enum_declare(Scanner s, SymTable st, Token tok);

static enum ASTPRIM typedef_declare(Compiler c, Scanner s, SymTable st,
                                    Token tok, Context ctx,
                                    SymTableEntry *cType);

static enum ASTPRIM typedef_type(Scanner s, SymTable st, Token tok,
                                 SymTableEntry *cType);
static int parse_literal(Compiler c, Scanner s, SymTable st, Token tok,
                         Context ctx, enum ASTPRIM type);
SymTableEntry function_declare(Compiler c, Scanner s, SymTable st, Token tok,
                               Context ctx, enum ASTPRIM type,
                               SymTableEntry cType);
static SymTableEntry symbol_declare(Compiler c, Scanner s, SymTable st,
                                    Token tok, Context ctx, enum ASTPRIM type,
                                    SymTableEntry cType, enum STORECLASS _class,
                                    ASTnode *tree);

static int param_declare_list(Compiler c, Scanner s, SymTable st, Token tok,
                              Context ctx, SymTableEntry oldFuncSym,
                              SymTableEntry newFuncSym);

static SymTableEntry array_declare(Compiler c, Scanner s, SymTable st,
                                   Token tok, Context ctx, char *varName,
                                   enum ASTPRIM type, SymTableEntry cType,
                                   enum STORECLASS _class);
static SymTableEntry scalar_declare(Compiler c, Scanner s, SymTable st,
                                    Token tok, Context ctx, char *varName,
                                    enum ASTPRIM type, SymTableEntry cType,
                                    enum STORECLASS _class, ASTnode *tree);

static SymTableEntry symbol_declare(Compiler c, Scanner s, SymTable st,
                                    Token tok, Context ctx, enum ASTPRIM type,
                                    SymTableEntry cType, enum STORECLASS _class,
                                    ASTnode *tree) {
    SymTableEntry sym = nullptr;
    char *varName = strdup(s->text);

    ident(s, tok);

    debug("symbol_declare %s", varName);
    debug("token %s", tok->tokstr);
    if (tok->token == T_LPAREN) {
        debug("mama mia its a function");
        free(varName);
        return function_declare(c, s, st, tok, ctx, type, cType);
    }

    switch (_class) {
        case C_EXTERN:
        case C_STATIC:
        case C_GLOBAL:
            if (SymTable_FindGlob(st, s) != nullptr) {
                lfatala(s, "DuplicateError: Duplicate global variable %s",
                        s->text);
            }
            break;
        case C_LOCAL:
        case C_PARAM:
            if (SymTable_FindLocl(st, s, ctx) != nullptr) {
                lfatala(s, "DuplicateError: Duplicate local variable %s",
                        s->text);
            }
            break;
        case C_MEMBER:
            if (SymTable_FindMember(st, s) != nullptr) {
                lfatala(s, "DuplicateError: Duplicate member %s", s->text);
            }
            break;
        default:
            lfatal(s, "UnsupportedError: Unsupported _class");
    }

    if (tok->token == T_LBRACKET) {
        sym = array_declare(c, s, st, tok, ctx, varName, type, cType, _class);
    } else {
        sym = scalar_declare(c, s, st, tok, ctx, varName, type, cType, _class,
                             tree);
    }

    free(varName);

    return sym;
}

static int parse_literal(Compiler c, Scanner s, SymTable st, Token tok,
                         Context ctx, enum ASTPRIM type) {
    ASTnode tree = Optimise(ASTnode_Order(c, s, st, tok, ctx));

    // if cast then mark it having the type from cast
    if (tree->op == A_CAST) {
        tree->left->type = tree->type;
        ASTnode temp = tree;
        tree = tree->left;
        temp->left = nullptr;
        free(temp);
    }

    if (tree->op != A_INTLIT && tree->op != A_STRLIT) {
        lfatal(s, "SyntaxError: Cannot initialize with non-constant values");
    }

    if (type == pointer_to(P_CHAR) ||
        (inttype(type) &&
         type_size(type, nullptr) >= type_size(tree->type, nullptr))) {
        int value = tree->intvalue;
        ASTnode_Free(tree);
        return value;
    }

    lfatal(s, "TypeError: Incompatible types in initialization");
}

static SymTableEntry scalar_declare(Compiler c, Scanner s, SymTable st,
                                    Token tok, Context ctx, char *varName,
                                    enum ASTPRIM type, SymTableEntry cType,
                                    enum STORECLASS _class, ASTnode *tree) {
    SymTableEntry sym = nullptr;
    ASTnode varNode, exprNode;
    *tree = nullptr;

    switch (_class) {
        case C_STATIC:
        case C_EXTERN:
        case C_GLOBAL:
            debug("adding global %s", varName);
            sym =
                SymTable_AddGlob(st, varName, type, cType, S_VAR, _class, 1, 0);
            debug("nelems %d", sym->nElems);
            break;
        case C_LOCAL:
            debug("a");
            sym = SymTable_AddLocl(st, varName, type, cType, S_VAR, 1);
            break;
        case C_PARAM:
            debug("b");
            sym = SymTable_AddParam(st, varName, type, cType, S_VAR);
            break;
        case C_MEMBER:
            debug("c");
            sym = SymTable_AddMemb(st, varName, type, cType, S_VAR, 1);
            break;
        default:
            lfatal(s, "UnsupportedError: unsupported _class");
    }

    if (tok->token == T_ASSIGN) {
        debug("assigning %s nelems %d", varName, sym->nElems);
        if (_class != C_GLOBAL && _class != C_LOCAL && _class != C_STATIC) {
            fatal(
                "SyntaxError: initialization only allowed for local, global "
                "and static "
                "variables\n");
        }
        // eat =
        Scanner_Scan(s, tok);

        if (_class == C_GLOBAL || _class == C_STATIC) {
            sym->initList = new int[1];
            sym->initList[0] = parse_literal(c, s, st, tok, ctx, type);
        } else if (_class == C_LOCAL) {
            varNode = ASTnode_NewLeaf(A_IDENT, sym->type, sym->ctype, sym, 0);

            exprNode = ASTnode_Order(c, s, st, tok, ctx);
            exprNode->rvalue = 1;

            exprNode =
                modify_type(exprNode, varNode->type, varNode->ctype, A_NONE);
            if (exprNode == nullptr) {
                fatal("TypeError: incompatible types in assignment\n");
            }

            *tree = ASTnode_New(A_ASSIGN, exprNode->type, exprNode, nullptr,
                                varNode, exprNode->ctype, nullptr, 0);
        }
    }

    return sym;
}

static SymTableEntry array_declare(Compiler c, Scanner s, SymTable st,
                                   Token tok, Context ctx, char *varName,
                                   enum ASTPRIM type, SymTableEntry cType,
                                   enum STORECLASS _class) {
    SymTableEntry sym;
    int nelems = -1;
    int maxElems;
    int *initList;
    int i = 0;

    // eat [
    Scanner_Scan(s, tok);

    if (tok->token != T_RBRACKET) {
        nelems = parse_literal(c, s, st, tok, ctx, P_INT);
        if (nelems < 0) {
            lfatala(s, "InvalidValueError: negative array size: %d", nelems);
        }
    }

    // eat ]
    match(s, tok, T_RBRACKET, "]");

    switch (_class) {
        case C_EXTERN:
        case C_GLOBAL:
            sym = SymTable_AddGlob(st, varName, pointer_to(type), cType,
                                   S_ARRAY, _class, 0, 0);
            break;
        default:
            lfatal(s,
                   "UnsupportedError: array declaration only supported for "
                   "globals");
    }

    if (tok->token == T_ASSIGN) {
        if (_class != C_GLOBAL && _class != C_STATIC) {
            lfatal(s,
                   "SyntaxError: array initialization only allowed for global "
                   "and static variables");
        }
        Scanner_Scan(s, tok);

        match(s, tok, T_LBRACE, "{");

        maxElems = (nelems == -1) ? TABLE_INCREMENT : nelems;

        initList = new int[maxElems];

        while (true) {
            if (nelems != -1 && i == maxElems) {
                fatal(
                    "ArraySizeError: too many elements in array initializer\n");
            }

            // TODO: Figure out how the fuck I'm going to handle custom
            // TODO: LABELS
            initList[i++] = parse_literal(c, s, st, tok, ctx, type);

            if (nelems == -1 && i == maxElems) {
                int oldSize = maxElems;
                maxElems += TABLE_INCREMENT;
                int *newarr = new int[maxElems];
                memcpy( newarr, initList, oldSize * sizeof(int) );
                delete [] initList;
                initList = newarr;
            }

            if (tok->token == T_RBRACE) {
                Scanner_Scan(s, tok);
                break;
            }

            comma(s, tok);
        }

        for (int j = i; j < sym->nElems; j++) initList[j] = 0;
        if (i > nelems) nelems = i;
        sym->initList = initList;
    }

    sym->nElems = nelems;
    sym->size = sym->nElems * type_size(type, cType);

    return sym;
}

static int param_declare_list(Compiler c, Scanner s, SymTable st, Token tok,
                              Context ctx, SymTableEntry oldFuncSym,
                              SymTableEntry newFuncSym) {
    enum ASTPRIM type;
    int paramCnt = 0;

    SymTableEntry protoPtr = nullptr;
    SymTableEntry cType;

    // * if a proto, member exists

    if (oldFuncSym != nullptr) {
        protoPtr = oldFuncSym->member;
    }

    ASTnode unused;

    while (tok->token != T_RPAREN) {
        // i32 main(void)
        if (tok->token == T_VOID) {
            Scanner_Scan(s, tok);
            if (tok->token == T_RPAREN) {
                paramCnt = 0;
                break;
            } else {
                Scanner_RejectToken(s, tok);
            }
        }


        type = declare_list(c, s, st, tok, ctx, &cType, C_PARAM, T_COMMA,
                            T_RPAREN, &unused);

        debug("WE GOT THE TYPE FROM PARAM %d", type);

        if (type == -1) {
            lfatal(s, "InvalidTypeError: invalid parameter type");
        }

        if (protoPtr != nullptr) {
            if (type != protoPtr->type) {
                lfatal(s,
                       "InvalidParamsError: parameter type mismatch for proto");
            }
            protoPtr = protoPtr->next;
        }

        paramCnt++;
        
        if (tok->token == T_RPAREN) {
            break;
        } else {
            comma(s, tok);
        }
    }

    if (oldFuncSym != nullptr && paramCnt != oldFuncSym->nElems) {
        lfatal(s, "InvalidParamsError: parameter count mismatch for proto");
    }

    return paramCnt;
}

enum ASTPRIM declare_list(Compiler c, Scanner s, SymTable st, Token tok,
                          Context ctx, SymTableEntry *cType,
                          enum STORECLASS _class, enum OPCODES end1,
                          enum OPCODES end2, ASTnode *glueTree) {
    enum ASTPRIM initType, type;
    SymTableEntry sym;
    ASTnode tree;
    *glueTree = nullptr;

    if ((initType = parse_type(c, s, st, tok, ctx, cType, &_class)) == -1) {
        return initType;
    }

    while (true) {
        type = static_cast<enum ASTPRIM>(parse_stars(s, tok, initType));

        sym = symbol_declare(c, s, st, tok, ctx, type, *cType, _class, &tree);

        if (sym->stype == S_FUNC) {
            if (_class != C_GLOBAL && _class != C_STATIC) {
                lfatal(s,
                       "SyntaxError: function declaration only allowed at "
                       "global level");
            }
            debug("FUNCTION DECLARATION");
            return type;
        }
        debug("went over???");

        *glueTree = (*glueTree == nullptr)
                        ? tree
                        : ASTnode_New(A_GLUE, P_NONE, *glueTree, nullptr, tree,
                                      nullptr, nullptr, 0);

        if (tok->token == end1 || tok->token == end2) {
            return type;
        }

        comma(s, tok);
    }
}

int parse_stars(Scanner s, Token tok, enum ASTPRIM type) {
    while (tok->token == T_STAR) {
        type = pointer_to(type);
        Scanner_Scan(s, tok);
    }
    return type;
}

SymTableEntry function_declare(Compiler c, Scanner s, SymTable st, Token tok,
                               Context ctx, enum ASTPRIM type,
                               SymTableEntry cType) {
    int paramCnt;

    SymTableEntry oldFuncSym, newFuncSym = nullptr;

    ASTnode tree, finalstmt;

    debug("DECLARING FUNCTION");

    if ((oldFuncSym = SymTable_FindSymbol(st, s, ctx)) != nullptr) {
        if (oldFuncSym->stype != S_FUNC) {
            oldFuncSym = nullptr;
        }
    }

    if (oldFuncSym == nullptr) {
        newFuncSym = SymTable_AddGlob(st, s->text, type, nullptr, S_FUNC, C_GLOBAL,
                                      1, false);
    }

    lparen(s, tok);
    debug("GOING IN DA PARAM LIST");
    paramCnt = param_declare_list(c, s, st, tok, ctx, oldFuncSym, newFuncSym);
    rparen(s, tok);
    debug("GTFO PARAM LIST");

    if (newFuncSym) {
        newFuncSym->nElems = paramCnt;
        newFuncSym->member = st->paramHead;
        oldFuncSym = newFuncSym;
    } else {
        SymTable_FreeParams(st);
    }

    st->paramHead = st->paramTail = nullptr;

    // This is only a proto
    if (tok->token == T_SEMI) {
        debug("proto");
        Scanner_Scan(s, tok);
        return oldFuncSym;
    }

    Context_SetFunctionId(ctx, oldFuncSym);
    Context_ResetLoopLevel(ctx);

    lbrace(s, tok);
    tree = Compound_Statement(c, s, st, tok, ctx, false);
    rbrace(s, tok);

    if (type != P_VOID) {
        if (tree == nullptr) {
            fatal("NoReturnError: non-void function must return a value\n");
        }
        finalstmt = tree->op == A_GLUE ? tree->right : tree;
        if (finalstmt == nullptr || finalstmt->op != A_RETURN) {
            fatal("NoReturnError: non-void function must return a value\n");
        }
    }

    tree = ASTnode_NewUnary(A_FUNCTION, type, tree, cType, oldFuncSym, 0);

    // optimise!!!
    tree = Optimise(tree);

    if (flags.dumpAST) {
        ASTnode_Dump(tree, st, NO_LABEL, 0);
    }

    Compiler_Gen(c, st, ctx, tree);
    ASTnode_Free(tree);

    SymTable_FreeLocls(st);
    Context_SetFunctionId(ctx, nullptr);

    return oldFuncSym;
}

enum ASTPRIM parse_type(Compiler c, Scanner s, SymTable st, Token tok,
                        Context ctx, SymTableEntry *ctype,
                        enum STORECLASS *_class) {
    debug("parse_type");
    enum ASTPRIM type;
    bool exstatic = true;

    // extern extern extern extern extern ...
    while (exstatic) {
        switch (tok->token) {
            case T_EXTERN:
                if (*_class == C_STATIC) {
                    lfatal(s, "SyntaxError: Cannot have extern and static");
                }
                debug("extern hit");
                *_class = C_EXTERN;
                Scanner_Scan(s, tok);
                break;
            case T_STATIC:
                if (*_class == C_EXTERN) {
                    lfatal(s, "SyntaxError: Cannot have extern and static");
                }
                *_class = C_STATIC;
                Scanner_Scan(s, tok);
                break;
            default:
                exstatic = false;
        }
    }

    debug("token (PARSE TYPE) %s", tok->tokstr);

    switch (tok->token) {
        case T_INT:
            debug("type is int");
            type = P_INT;
            debug("before scan");
            Scanner_Scan(s, tok);
            debug("after scan");
            break;
        case T_CHAR:
            debug("type is char");
            type = P_CHAR;
            Scanner_Scan(s, tok);
            break;
        case T_VOID:
            debug("type is void");
            type = P_VOID;
            Scanner_Scan(s, tok);
            break;
        case T_STRUCT:
            debug("type is struct");
            type = P_STRUCT;
            *ctype = composite_declare(c, s, st, tok, ctx, P_STRUCT);
            if (tok->token == T_SEMI) type = static_cast<ASTPRIM>(-1);
            break;
        case T_UNION:
            debug("type is union");
            type = P_UNION;
            *ctype = composite_declare(c, s, st, tok, ctx, P_UNION);
            if (tok->token == T_SEMI) type = static_cast<ASTPRIM>(-1);
            break;
        case T_ENUM:
            type = P_INT;
            enum_declare(s, st, tok);
            // if after ;, theres no type
            if (tok->token == T_SEMI) type = static_cast<ASTPRIM>(-1);
            break;
        case T_TYPEDEF:
            debug("type is typedef");
            type = typedef_declare(c, s, st, tok, ctx, ctype);
            if (tok->token == T_SEMI) type = static_cast<ASTPRIM>(-1);
            break;
        case T_IDENT:  // typedef
            debug("type is ident (maybe a typedef)");
            type = typedef_type(s, st, tok, ctype);
            break;
        default:
            fatala("InternalError: unknown type %d", tok->token);
    }

    // allows the user to do int ******a
    debug("WOW THE TYPE ISSSS %d", type);
    while (true) {
        if (tok->token != T_STAR) break;
        type = pointer_to(type);
        Scanner_Scan(s, tok);
    }
    debug("END OF THE TYPE ISSSS %d", type);

    return type;
}

enum ASTPRIM parse_cast(Compiler c, Scanner s, SymTable st, Token tok,
                        Context ctx, SymTableEntry *cType) {
    enum ASTPRIM type;
    enum STORECLASS _class;

    type = static_cast<enum ASTPRIM>(parse_stars(s, tok, parse_type(c, s, st, tok, ctx, cType, &_class)));

    if (type == P_STRUCT || type == P_UNION || type == P_VOID) {
        fatal("InvalidTypeError: invalid cast type");
    }

    return type;
}

static SymTableEntry composite_declare(Compiler c, Scanner s, SymTable st,
                                       Token tok, Context ctx,
                                       enum ASTPRIM type) {
    SymTableEntry cType = nullptr;
    SymTableEntry memb;

    int offset;
    enum ASTPRIM t;
    ASTnode unused;

    Scanner_Scan(s, tok);

    if (tok->token == T_IDENT) {
        if (type == P_STRUCT) {
            cType = SymTable_FindStruct(st, s);
        } else if (type == P_UNION) {
            cType = SymTable_FindUnion(st, s);
        } else {
            fatal("InternalError: Unknown composite type");
        }
        Scanner_Scan(s, tok);
    }

    if (tok->token != T_LBRACE) {
        if (cType == nullptr) {
            lfatala(s, "UndefinedError: struct/union %s is not defined",
                    s->text);
        }
        return cType;
    }

    if (cType) {
        lfatal(s, "DuplicateError: struct/union already defined");
    }

    if (type == P_STRUCT) {
        cType = SymTable_AddStruct(st, s->text);
    } else if (type == P_UNION) {
        cType = SymTable_AddUnion(st, s->text);
    } else {
        fatal("InternalError: Unknown composite type");
    }

    debug("Declaring a struct: %s", s->text);

    Scanner_Scan(s, tok);

    while (true) {
        t = declare_list(c, s, st, tok, ctx, &memb, C_MEMBER, T_SEMI, T_RBRACE,
                         &unused);
        if (t == -1) {
            lfatal(s, "SyntaxError: invalid member type");
        }
        if (tok->token == T_SEMI) {
            Scanner_Scan(s, tok);
        }
        if (tok->token == T_RBRACE) {
            break;
        }
    }

    rbrace(s, tok);
    if (st->membHead == nullptr) {
        fatala("EmptyStructError: struct/union has no members, %s",
               cType->name);
    }

    cType->member = st->membHead;
    st->membHead = st->membTail = nullptr;

    memb = cType->member;
    memb->posn = 0;
    offset = type_size(memb->type, memb->ctype);

    // for union
    int maxSize = 0;

    for (memb = memb->next; memb != nullptr; memb = memb->next) {
        if (type == P_STRUCT) {
            memb->posn = MIPS_Align(memb->type, offset, 1);
            // Calculates offset of next free byte
            offset += type_size(memb->type, memb->ctype);
        } else {
            memb->posn = 0;
            int size = type_size(memb->type, memb->ctype);
            debug("Size of %s: %d", memb->name, size);
            // We only store the largest byte size
            if (size > maxSize) {
                maxSize = size;
            }
        }
    }

    debug("Size of struct %d", offset);

    cType->size = type == P_STRUCT ? offset : maxSize;

    return cType;
}

static void enum_declare(Scanner s, SymTable st, Token tok) {
    SymTableEntry eType = nullptr;
    char *name = nullptr;
    int intVal = 0;

    Scanner_Scan(s, tok);

    if (tok->token == T_IDENT) {
        eType = SymTable_FindEnumType(st, s);
        name = strdup(s->text);
        Scanner_Scan(s, tok);
    }

    if (tok->token != T_LBRACE) {
        if (eType == nullptr) {
            lfatala(s, "UndefinedError: enum %s is not defined", name);
        }
        if (name) free(name);
        return;
    }

    Scanner_Scan(s, tok);

    if (eType != nullptr) {
        lfatala(s, "DuplicateError: enum %s already defined", eType->name);
    }

    eType = SymTable_AddEnum(st, name, C_ENUMTYPE, 0);

    if (name) free(name);

    while (true) {
        ident(s, tok);
        name = strdup(s->text);

        eType = SymTable_FindEnumVal(st, s);
        if (eType != nullptr) {
            lfatala(s, "DuplicateError: enum value %s already defined", name);
        }

        if (tok->token == T_ASSIGN) {
            Scanner_Scan(s, tok);
            if (tok->token != T_INTLIT) {
                lfatal(s, "SyntaxError: expected integer literal");
            }

            intVal = tok->intvalue;
            Scanner_Scan(s, tok);
        }

        eType = SymTable_AddEnum(st, name, C_ENUMVAL, intVal++);
        free(name);

        if (tok->token == T_RBRACE) break;

        comma(s, tok);
    }

    // Consume the }
    Scanner_Scan(s, tok);
}

static enum ASTPRIM typedef_declare(Compiler c, Scanner s, SymTable st,
                                    Token tok, Context ctx,
                                    SymTableEntry *cType) {
    enum ASTPRIM type;
    enum STORECLASS _class = C_NONE;

    // typedef consumed
    Scanner_Scan(s, tok);
    // a ident should be here

    type = parse_type(c, s, st, tok, ctx, cType, &_class);
    if (_class != C_NONE) {
        lfatal(s, "TypeError: typedef cannot have extern");
    }

    if (SymTable_FindTypeDef(st, s) != nullptr) {
        lfatala(s, "DuplicateError: typedef %s already defined", s->text);
    }

    SymTable_AddTypeDef(st, s->text, type, *cType);
    Scanner_Scan(s, tok);

    return type;
}

static enum ASTPRIM typedef_type(Scanner s, SymTable st, Token tok,
                                 SymTableEntry *cType) {
    SymTableEntry type;

    type = SymTable_FindTypeDef(st, s);
    debug("typedef is %d :)", type->type);
    if (type == nullptr) {
        lfatala(s, "UndefinedError: typedef %s is not defined", s->text);
    }

    Scanner_Scan(s, tok);
    *cType = type->ctype;
    return type->type;
}

void global_declare(Compiler c, Scanner s, SymTable st, Token tok,
                    Context ctx) {
    SymTableEntry cType;
    ASTnode unused;
    while (tok->token != T_EOF) {
        declare_list(c, s, st, tok, ctx, &cType, C_GLOBAL, T_SEMI, T_EOF,
                     &unused);

        if (tok->token == T_SEMI) {
            Scanner_Scan(s, tok);
        }
    }
}
