%{
#include "bf_ast.h"
#include <stdio.h>
#include <stdlib.h>

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

void yyerror(yyscan_t scanner, ast_node_t **result, const char *s);
%}

%define api.pure full
%define parse.error verbose
%parse-param {yyscan_t scanner} {ast_node_t **result}
%lex-param {yyscan_t scanner}

%union {
    ast_node_t *node;
}

%code {
    int yylex(YYSTYPE *yylval_param, yyscan_t yyscanner);
}

%token MOVE_RIGHT MOVE_LEFT INC_VAL DEC_VAL OUTPUT INPUT LOOP_START LOOP_END
%type <node> program statement_list statement loop

%start program

%%

program:
    statement_list          { *result = $1; }
    | /* empty */           { *result = NULL; }
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

void yyerror(yyscan_t scanner, ast_node_t **result, const char *s) {
    (void)scanner;
    (void)result;
    fprintf(stderr, "Parse error: %s\n", s);
}