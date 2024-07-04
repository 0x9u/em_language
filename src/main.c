#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "gen.h"
#include "scan.h"
#include "stmt.h"
#include "tokens.h"
#include "sym.h"
#include "decl.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: [file] [outfile]\n");
        exit(-1);
    }

    Scanner s = Scanner_New(argv[1]);
    if (s == NULL) {
        fprintf(stderr, "Error: unable to initialise scanner\n");
        exit(-1);
    }

    Compiler c = Compiler_New(argv[2]);
    SymTable st = SymTable_New();
    Token tok = calloc(1, sizeof(struct token));

    //MIPS_Pre(c);
    Scanner_Scan(s, tok);
    
    ASTnode t;
    while (true) {
        t = function_declare(s, st, tok); //Compound_Statement(s, st, tok);
        printf("Tree rn: %p\n", t);
        Compiler_Gen(c, st, t);
        if (tok->token == T_EOF) break;
    }
    
    //Compiler_Gen(c, st, t);

    MIPS_Post(c);
    
    Compiler_GenData(c, st);
    
    Scanner_Free(s);
    Compiler_Free(c);
    SymTable_Free(st);
    free(tok);
    printf("Success!\n");

    exit(0);
}