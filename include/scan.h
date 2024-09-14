#ifndef SCAN_H
#define SCAN_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "defs.h"

// May god forgive me for this
#define TEXTLEN 400000

struct scanner {
    // current line number
    int line;

    char text[TEXTLEN + 1];

    char comment[TEXTLEN + 1];
    int commentLen;
};

typedef struct scanner *Scanner;

Scanner Scanner_New(void);
void Scanner_Free(Scanner);
void Scanner_Scan(Scanner this, Token t);
void Scanner_RejectToken(Scanner this, Token t);
void Scanner_EndComment(Scanner this);

void match(Scanner s, Token t, enum OPCODES op, char *tok);
void semi(Scanner s, Token t);
void ident(Scanner s, Token t);
void lbrace(Scanner s, Token t);
void rbrace(Scanner s, Token t);
void lparen(Scanner s, Token t);
void rparen(Scanner s, Token t);
void lbracket(Scanner s, Token t);
void rbracket(Scanner s, Token t);
void comma(Scanner s, Token t);

#endif