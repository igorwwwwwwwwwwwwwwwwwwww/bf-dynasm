%{
#include "bf_ast.h"
#include <stdio.h>
#include <stdlib.h>

extern int yylex();
extern int yylineno;
extern char *yytext;

ast_node_t *parse_result = NULL;

void yyerror(const char *s);
%}

%define parse.error verbose

%union {
    ast_node_t *node;
}

%token MOVE_RIGHT MOVE_LEFT INC_VAL DEC_VAL OUTPUT INPUT LOOP_START LOOP_END
%type <node> program statement_list statement loop

%start program

%%

program:
    statement_list          { parse_result = $1; }
    | /* empty */           { parse_result = NULL; }
    ;

statement_list:
    statement               { $$ = $1; }
    | statement_list statement {
        if ($1 == NULL) {
            $$ = $2;
        } else if ($2 == NULL) {
            $$ = $1;
        } else {
            $$ = ast_create_sequence($1, $2);
        }
    }
    ;

statement:
    MOVE_RIGHT              { $$ = ast_create_move(1); }
    | MOVE_LEFT             { $$ = ast_create_move(-1); }
    | INC_VAL               { $$ = ast_create_add(1); }
    | DEC_VAL               { $$ = ast_create_add(-1); }
    | OUTPUT                { $$ = ast_create_output(); }
    | INPUT                 { $$ = ast_create_input(); }
    | loop                  { $$ = $1; }
    ;

loop:
    LOOP_START statement_list LOOP_END { $$ = ast_create_loop($2); }
    | LOOP_START LOOP_END               { $$ = ast_create_loop(NULL); }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Parse error at line %d: %s\n", yylineno, s);
}