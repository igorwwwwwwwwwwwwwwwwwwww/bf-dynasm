%{
#define _GNU_SOURCE
#include "bf_parser.h"

static int current_column = 1;

void reset_column(void) { current_column = 1; }

#define YY_USER_ACTION yylloc->first_line = yylloc->last_line = yylineno; \
                       yylloc->first_column = current_column; yylloc->last_column = current_column + yyleng - 1; \
                       current_column += yyleng;
%}

%option yylineno
%option noyywrap
%option nounput
%option noinput
%option reentrant
%option bison-bridge
%option bison-locations
%option header-file="bf_lexer.h"

%%

">"         { return MOVE_RIGHT; }
"<"         { return MOVE_LEFT; }
"+"         { return INC_VAL; }
"-"         { return DEC_VAL; }
"."         { return OUTPUT; }
","         { return INPUT; }
"["         { return LOOP_START; }
"]"         { return LOOP_END; }

[ \t]+      { /* ignore whitespace */ }
\n          { current_column = 1; /* reset column on newline */ }
.           { /* ignore all other characters (comments) */ }

%%
