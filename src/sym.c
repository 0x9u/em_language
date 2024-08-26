#include "sym.h"

#include "misc.h"
#include "scan.h"
#include "context.h"


static void freeList(SymTableEntry head);

static SymTableEntry SymTableEntryNew(char *name, enum ASTPRIM type,
                                      SymTableEntry ctype,
                                      enum STRUCTTYPE stype,
                                      enum STORECLASS class, int size,
                                      int offset);
static void pushSym(SymTableEntry *head, SymTableEntry *tail, SymTableEntry e);

static SymTableEntry SymTableEntryNew(char *name, enum ASTPRIM type,
                                      SymTableEntry ctype,
                                      enum STRUCTTYPE stype,
                                      enum STORECLASS class, int size,
                                      int offset) {
    SymTableEntry e = calloc(1, sizeof(struct symTableEntry));
    if (e == NULL) {
        fatal("InternalError: Unable to allocate memory for SymTableEntry");
    }

    e->name = strdup(name);
    e->type = type;
    e->ctype = ctype;

    e->stype = stype;
    e->class = class;

    e->size = size;
    e->offset = offset;

    return e;
}

static void pushSym(SymTableEntry *head, SymTableEntry *tail, SymTableEntry e) {
    if (head == NULL || tail == NULL || e == NULL) {
        fatal("InternalError: head, tail or e is NULL");
    }

    if (tail != NULL) {
        (*tail)->next = e;
        *tail = e;
    } else {
        *head = *tail = e;
    }

    e->next = NULL;
}

SymTableEntry SymTable_AddGlob(SymTable this, Scanner s, enum ASTPRIM type,
                             SymTableEntry ctype, enum STRUCTTYPE stype,
                             int size) {
    SymTableEntry e =
        SymTableEntryNew(s->text, type, ctype, stype, C_GLOBAL, size, 0);

    pushSym(&this->globHead, &this->globTail, e);
    return e;
}

SymTableEntry SymTable_AddLocl(SymTable this, Scanner s, enum ASTPRIM type,
                             SymTableEntry ctype, enum STRUCTTYPE stype,
                             int size) {
    SymTableEntry e =
        SymTableEntryNew(s->text, type, ctype, stype, C_LOCAL, size, 0);

    pushSym(&this->loclHead, &this->loclTail, e);
    return e;
}

SymTableEntry SymTable_AddParam(SymTable this, Scanner s, enum ASTPRIM type,
                              SymTableEntry ctype, enum STRUCTTYPE stype,
                              int size) {
    SymTableEntry e =
        SymTableEntryNew(s->text, type, ctype, stype, C_PARAM, size, 0);

    pushSym(&this->paramHead, &this->paramTail, e);
    return e;
}

SymTableEntry SymTable_AddMemb(SymTable this, Scanner s, enum ASTPRIM type,
                             SymTableEntry ctype, enum STRUCTTYPE stype,
                             int size) {
    SymTableEntry e =
        SymTableEntryNew(s->text, type, ctype, stype, C_MEMBER, size, 0);

    pushSym(&this->membHead, &this->membTail, e);
    return e;
}

SymTableEntry SymTable_AddStruct(SymTable this, Scanner s, enum ASTPRIM type,
                               SymTableEntry ctype, enum STRUCTTYPE stype,
                               int size) {
    SymTableEntry e =
        SymTableEntryNew(s->text, type, ctype, stype, C_STRUCT, size, 0);

    pushSym(&this->structHead, &this->structTail, e);
    return e;
}

SymTableEntry SymTable_FindSymInList(Scanner s, SymTableEntry head) {
    for (; head != NULL; head = head->next) {
        if (head->name != NULL && !strcmp(s->text, head->name)) {
            return head;
        }
    }
    return NULL;
}

SymTableEntry SymTable_FindGlob(SymTable this, Scanner s) {
    return SymTable_FindSymInList(s, this->globHead);
}

SymTableEntry SymTable_FindLocl(SymTable this, Scanner s) {
    return SymTable_FindSymInList(s, this->loclHead);
}

SymTableEntry SymTable_FindMember(SymTable this, Scanner s) {
    return SymTable_FindSymInList(s, this->membHead);
}

SymTableEntry SymTable_FindStruct(SymTable this, Scanner s) {
    return SymTable_FindSymInList(s, this->structHead);
}

SymTableEntry SymTable_FindSymbol(SymTable this, Scanner s, Context c) {

    SymTableEntry e;
    if (c->functionId) {
        e = SymTable_FindSymInList(s, c->functionId->member);
        if (e) return e;
    }
    e = SymTable_FindLocl(this, s);
    if (e) return e;

    return SymTable_FindGlob(this, s);
}

SymTable SymTable_New(void) {
    SymTable g = calloc(1, sizeof(struct symTable));
    if (g == NULL) {
        fatal("InternalError: Unable to allocate memory for SymTable");
    }

    return g;
}

static void freeList(SymTableEntry head) {
    SymTableEntry tmp;
    while (head != NULL) {
        tmp = head;
        head = head->next;
        free(tmp);
    }
}

void SymTable_Free(SymTable this) {
    freeList(this->globHead);
    freeList(this->loclHead);
    freeList(this->paramHead);
    freeList(this->membHead);
    freeList(this->structHead);

    free(this);
}

void SymTable_FreeLocls(SymTable this) { 

    // TODO: Clean up the memory later
    this->loclHead = this->loclTail = NULL;
    this->paramHead = this->paramTail = NULL;
 }

void SymTable_SetValue(SymTable this, SymTableEntry e, int intvalue) {
    e->value = intvalue;
}

void SymTable_SetText(SymTable this, Scanner s, SymTableEntry e) {
    e->strValue = strdup(s->text);
}