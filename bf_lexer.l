%{
#include "bf_ast.h"
#include "bf_parser.h"
%}

%option yylineno
%option noyywrap

%%

">"         { return MOVE_RIGHT; }
"<"         { return MOVE_LEFT; }
"+"         { return INC_VAL; }
"-"         { return DEC_VAL; }
"."         { return OUTPUT; }
","         { return INPUT; }
"["         { return LOOP_START; }
"]"         { return LOOP_END; }

[ \t\n]+    { /* ignore whitespace and newlines */ }
.           { /* ignore all other characters (comments) */ }

%%