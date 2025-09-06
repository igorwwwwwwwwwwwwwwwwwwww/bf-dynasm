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

#ifndef YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
#endif

extern int yylex_init(yyscan_t *scanner);
extern int yylex_destroy(yyscan_t scanner);
extern YY_BUFFER_STATE yy_scan_string(const char *str, yyscan_t scanner);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer, yyscan_t scanner);
extern void yyset_lineno(int line_number, yyscan_t yyscanner);
extern void reset_column(void);

void yyerror(YYLTYPE *yylloc, yyscan_t scanner, ast_node_t **result, const char *s);
ast_node_t* parse_bf_program(const char *program);
}

%define api.pure full
%define parse.error verbose
%locations
%parse-param {yyscan_t scanner} {ast_node_t **result}
%lex-param {yyscan_t scanner}

%union {
    ast_node_t *node;
}

%code {
    int yylex(YYSTYPE *yylval_param, YYLTYPE *yylloc_param, yyscan_t yyscanner);
}

%token MOVE_RIGHT MOVE_LEFT INC_VAL DEC_VAL OUTPUT INPUT LOOP_START LOOP_END DEBUG_LOG
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
    MOVE_RIGHT              { $$ = ast_create_move(1); ast_set_location($$, @1.first_line, @1.first_column); }
    | MOVE_LEFT             { $$ = ast_create_move(-1); ast_set_location($$, @1.first_line, @1.first_column); }
    | INC_VAL               { $$ = ast_create_add(1, 0); ast_set_location($$, @1.first_line, @1.first_column); }
    | DEC_VAL               { $$ = ast_create_add(-1, 0); ast_set_location($$, @1.first_line, @1.first_column); }
    | OUTPUT                { $$ = ast_create_output(0); ast_set_location($$, @1.first_line, @1.first_column); }
    | INPUT                 { $$ = ast_create_input(0); ast_set_location($$, @1.first_line, @1.first_column); }
    | DEBUG_LOG             { $$ = ast_create_debug_log(); ast_set_location($$, @1.first_line, @1.first_column); }
    | loop                  { $$ = $1; }
    ;

loop:
    LOOP_START statement_list LOOP_END { $$ = ast_create_loop($2); ast_set_location($$, @1.first_line, @1.first_column); }
    | LOOP_START LOOP_END               { $$ = ast_create_loop(NULL); ast_set_location($$, @1.first_line, @1.first_column); }
    ;

%%

void yyerror(YYLTYPE *yylloc, yyscan_t scanner, ast_node_t **result, const char *s) {
    (void)scanner;
    (void)result;
    fprintf(stderr, "Parse error at line %d, column %d: %s\n", yylloc->first_line, yylloc->first_column, s);
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

    // Reset line and column counters
    yyset_lineno(1, scanner);
    reset_column();

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
