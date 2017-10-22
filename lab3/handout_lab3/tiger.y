%{
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h" 
#include "errormsg.h"
#include "absyn.h"

int yylex(void); /* function prototype */

A_exp absyn_root;

void yyerror(char *s)
{
 EM_error(EM_tokPos, "%s", s);
 exit(1);
}
%}

%union {
	int pos;
	int ival;
	string sval;
	A_var var;
        A_ty ty;
	A_exp exp;        
        A_expList explist;
        A_dec  dec;
        A_decList declist;
        A_efield  efield;
        A_efieldList efieldlist;       
        A_namety namety;
        A_nametyList nametylist;
        A_field field;
        A_fieldList fieldlist;
        A_fundec fundec;
        A_fundecList fundeclist;             
	/* et cetera */
	}

%token <sval> ID STRING
%token <ival> INT

%token 
  COMMA COLON SEMICOLON LPAREN RPAREN LBRACK RBRACK 
  LBRACE RBRACE DOT 
  PLUS MINUS TIMES DIVIDE EQ NEQ LT LE GT GE
  AND OR ASSIGN
  ARRAY IF THEN ELSE WHILE FOR TO DO LET IN END OF 
  BREAK NIL
  FUNCTION VAR TYPE  

%type <ty> ty
%type <exp> exp program ifexp whileexp forexp letexp
%type <explist> exp_list explist exp_seq expseq
%type <var>  lvalue lvalue1 lvalue2
%type <fundec> fundec
%type <fundeclist> fundecs
%type <namety> tydec
%type <nametylist> tydecs
%type <dec>  dec vardec
%type <declist> decs 
%type <efield> rec 
%type <efieldlist> recseq rec_seq
%type <field> tyfield
%type <fieldlist> tyfields tyfield_s
/* et cetera */


%nonassoc DO 
%nonassoc OF
%nonassoc THEN
%nonassoc ELSE
%right ASSIGN
%left OR
%left AND
%nonassoc EQ NEQ LT LE GT GE
%left PLUS MINUS
%left TIMES DIVIDE
%left UMINUS
%left LPAREN RPAREN

%start program

%%

program:   exp    {absyn_root=$1;}
      
    
exp:   lvalue  {$$=A_VarExp(EM_tokPos, $1);}
       | NIL   {$$=A_NilExp(EM_tokPos);}
       | LPAREN expseq RPAREN   {$$=A_SeqExp(EM_tokPos, $2);}
       | INT   {$$=A_IntExp(EM_tokPos,$1);}
       | STRING  {if(strcmp($1,"(null)") == 0) $$=A_StringExp(EM_tokPos,""); else $$=A_StringExp(EM_tokPos,$1);}
       | exp PLUS exp  {$$=A_OpExp(EM_tokPos, A_plusOp, $1, $3);}
       | exp MINUS exp  {$$=A_OpExp(EM_tokPos, A_minusOp, $1, $3);}
       | exp TIMES exp  {$$=A_OpExp(EM_tokPos, A_timesOp, $1, $3);}
       | exp DIVIDE exp  {$$=A_OpExp(EM_tokPos, A_divideOp, $1, $3);}
       | exp EQ exp      {$$=A_OpExp(EM_tokPos, A_eqOp, $1, $3);}
       | exp NEQ exp      {$$=A_OpExp(EM_tokPos, A_neqOp, $1, $3);}
       | exp LT exp      {$$=A_OpExp(EM_tokPos, A_ltOp, $1, $3);}
       | exp LE exp      {$$=A_OpExp(EM_tokPos, A_leOp, $1, $3);}
       | exp GT exp      {$$=A_OpExp(EM_tokPos, A_gtOp, $1, $3);}
       | exp GE exp      {$$=A_OpExp(EM_tokPos, A_geOp, $1, $3);}
       | exp AND exp     {$$=A_IfExp(EM_tokPos, $1, $3, A_IntExp(EM_tokPos,0));}
       | exp OR exp      {$$=A_IfExp(EM_tokPos, $1, A_IntExp(EM_tokPos,1), $3);}
       | MINUS exp  %prec UMINUS   {$$=A_OpExp(EM_tokPos, A_minusOp, A_IntExp(EM_tokPos, 0), $2);}
       | ifexp           {$$ = $1;}
       | whileexp        {$$ = $1;}
       | forexp          {$$ = $1;}
       | letexp          {$$ = $1;}
       | BREAK           {$$=A_BreakExp(EM_tokPos);}
       | ID LPAREN explist RPAREN  {$$=A_CallExp(EM_tokPos, S_Symbol($1), $3);}
       | ID LBRACE recseq RBRACE   {$$=A_RecordExp(EM_tokPos, S_Symbol($1), $3);}
       | ID LBRACK exp RBRACK OF exp  {$$=A_ArrayExp(EM_tokPos,S_Symbol($1),$3,$6);}
       | lvalue ASSIGN exp    {$$=A_AssignExp(EM_tokPos,$1,$3);}

exp_list: exp   {$$ = A_ExpList($1, NULL);}
       | exp COMMA exp_list  {$$ = A_ExpList($1,$3);}

explist: exp_list  {$$ = $1;}
       | /*empty*/ {$$ = NULL;}  

exp_seq: exp {$$ = A_ExpList($1, NULL);}
       | exp SEMICOLON exp_seq {$$ = A_ExpList($1,$3);}

expseq: exp_seq {$$ = $1;}
       | /*empty*/ {$$ = NULL;}


rec: ID EQ exp  {$$ = A_Efield(S_Symbol($1),$3);}

rec_seq: rec  {$$ = A_EfieldList($1,NULL);}
       | rec COMMA rec_seq {$$ = A_EfieldList($1,$3);}

recseq: rec_seq  {$$ = $1;}
       | /*empty*/  {$$ = NULL;}


decs:  dec decs   {$$ = A_DecList($1, $2);}
       | /*empty*/  {$$ = NULL;}

dec: tydecs  {$$ = A_TypeDec(EM_tokPos, $1);}
     | vardec {$$ = $1;}
     | fundecs {$$ = A_FunctionDec(EM_tokPos,$1);}

tydec: TYPE ID EQ ty  {$$ = A_Namety(S_Symbol($2), $4);}

tydecs : tydec   {$$=A_NametyList($1,NULL);}
         |tydec tydecs {$$ = A_NametyList($1,$2);}

vardec: VAR ID ASSIGN exp {$$ = A_VarDec(EM_tokPos, S_Symbol($2), NULL, $4);}
        | VAR ID COLON ID ASSIGN exp  {$$ = A_VarDec(EM_tokPos,S_Symbol($2),S_Symbol($4),$6);}

fundec: FUNCTION ID LPAREN tyfields RPAREN EQ exp  {$$ = A_Fundec(EM_tokPos,S_Symbol($2),$4,NULL,$7);}
        | FUNCTION ID LPAREN tyfields RPAREN COLON ID EQ exp {$$ = A_Fundec(EM_tokPos,S_Symbol($2),$4,S_Symbol($7),$9);}

fundecs: fundec  {$$ = A_FundecList($1,NULL);}
         |fundec fundecs  {$$ = A_FundecList($1,$2);}

ty: ID  {$$ = A_NameTy(EM_tokPos,S_Symbol($1));}
   | LBRACE tyfields RBRACE {$$ = A_RecordTy(EM_tokPos, $2);}
   | ARRAY OF ID  {$$ = A_ArrayTy(EM_tokPos, S_Symbol($3));}

tyfield: ID COLON ID {$$ = A_Field(EM_tokPos, S_Symbol($1), S_Symbol($3));}

tyfield_s:  tyfield {$$ = A_FieldList($1, NULL);}
          | tyfield COMMA tyfield_s  {$$ = A_FieldList($1, $3);}

tyfields: tyfield_s {$$ = $1;}
          | /*empty*/ {$$ = NULL;}


/*lvalue: ID   {$$ = A_SimpleVar(EM_tokPos, S_Symbol($1));}
        | lvalue DOT ID {$$ = A_FieldVar(EM_tokPos, $1,S_Symbol($3));}
        | lvalue LBRACK exp RBRACK {$$=A_SubscriptVar(EM_tokPos, $1, $3);}*/


lvalue: ID   {$$ = A_SimpleVar(EM_tokPos, S_Symbol($1));}
        | lvalue2 {$$ = $1;}

lvalue1: ID DOT ID {$$ = A_FieldVar(EM_tokPos,A_SimpleVar(EM_tokPos,S_Symbol($1)),S_Symbol($3));}
        | ID LBRACK exp RBRACK {$$ = A_SubscriptVar(EM_tokPos,A_SimpleVar(EM_tokPos,S_Symbol($1)),$3);}

lvalue2: lvalue1 {$$ = $1;}
        | lvalue2 DOT ID {$$ = A_FieldVar(EM_tokPos, $1,S_Symbol($3));}
        | lvalue2 LBRACK exp RBRACK{$$=A_SubscriptVar(EM_tokPos, $1, $3);}
        


ifexp: IF exp THEN exp ELSE exp  {$$=A_IfExp(EM_tokPos, $2, $4, $6);}
       |  IF exp THEN exp       {$$=A_IfExp(EM_tokPos, $2, $4, A_NilExp(EM_tokPos));}

whileexp: WHILE exp DO exp       {$$=A_WhileExp(EM_tokPos, $2, $4);}

forexp: FOR ID ASSIGN exp TO exp DO exp  {$$=A_ForExp(EM_tokPos, S_Symbol($2), $4, $6, $8);}

letexp: LET decs IN expseq END  {$$=A_LetExp(EM_tokPos, $2, A_SeqExp(EM_tokPos, $4));}



