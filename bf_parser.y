%{
#include "bf_ast.h"
#include <stdio.h>
#include <stdlib.h>
%}

%code requires {
#include "bf_ast.h"

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

#ifndef YY_TYPEDEF_YY_BUFFER_STATE
#define YY_TYPEDEF_YY_BUFFER_STATE
typedef struct yy_buffer_state *YY_BUFFER_STATE;
#endif

extern int yylex_init(yyscan_t *scanner);
extern int yylex_destroy(yyscan_t scanner);
extern YY_BUFFER_STATE yy_scan_string(const char *str, yyscan_t scanner);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer, yyscan_t scanner);

void yyerror(yyscan_t scanner, ast_node_t **result, const char *s);
ast_node_t* parse_bf_program(const char *program);
}

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
    | INC_VAL               { $$ = ast_create_add(1, 0); }
    | DEC_VAL               { $$ = ast_create_add(-1, 0); }
    | OUTPUT                { $$ = ast_create_output(0); }
    | INPUT                 { $$ = ast_create_input(0); }
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

ast_node_t* parse_bf_program(const char *program) {
    yyscan_t scanner;
    YY_BUFFER_STATE buffer;
    ast_node_t *result = NULL;
    
    if (yylex_init(&scanner) != 0) {
        fprintf(stderr, "Error: Failed to initialize scanner\n");
        exit(1);
    }
    
    buffer = yy_scan_string(program, scanner);
    
    if (yyparse(scanner, &result) != 0) {
        yy_delete_buffer(buffer, scanner);
        yylex_destroy(scanner);
        fprintf(stderr, "Error: Parser error\n");
        exit(1);
    }
    
    yy_delete_buffer(buffer, scanner);
    yylex_destroy(scanner);
    return result;
}