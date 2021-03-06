%option noyywrap
%{
/* RCS information: $Id: shell.l,v 1.1 2006/04/05 22:46:33 elm Exp $ */

#include <stdio.h>
#include <strings.h>

int _numargs = 100;
char *_args[100];
int _argcount = 0;
%}

WORD [a-zA-Z0-9\/\.-]+
SPECIAL [()><|&;*]

%%
  _argcount = 0; _args[0] = NULL;

{WORD}|{SPECIAL} {
  if(_argcount < _numargs-1) {
    _args[_argcount++] = (char *)strdup(yytext);
    _args[_argcount] = NULL;
  }
}

\n return (int)_args;

[ \t]+

.

%%

char **getline_custom() { return (char **)yylex(); }
