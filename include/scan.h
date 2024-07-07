#ifndef SCAN_H
#define SCAN_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "defs.h"

#define TEXTLEN 512

struct scanner {
    // file to scan in
    FILE *infile;
    // character to put back
    char putback;
    // current line number
    int line;

    char text[TEXTLEN + 1];

    Token rejToken;
};

typedef struct scanner *Scanner;

Scanner Scanner_New(char *name);
void Scanner_Free(Scanner);
bool Scanner_Scan(Scanner, Token t);
void Scanner_Putback(Scanner this, char c);
void Scanner_RejectToken(Scanner this, Token t);

void match(Scanner s, Token t, enum OPCODES op, char *tok);
void semi(Scanner s, Token t);
void ident(Scanner s, Token t);
void lbrace(Scanner s, Token t);
void rbrace(Scanner s, Token t);
void lparen(Scanner s, Token t);
void rparen(Scanner s, Token t);


#endif