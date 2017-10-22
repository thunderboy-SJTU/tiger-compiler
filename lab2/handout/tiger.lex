%{
#include <string.h>
#include "util.h"
#include "tokens.h"
#include "errormsg.h"

int charPos=1;

int yywrap(void)
{
 charPos=1;
 return 1;
}

void adjust(void)
{
 EM_tokPos=charPos;
 charPos+=yyleng;
}
/*
* Please don't modify the lines above.
* You can add C declarations of your own below.
*/


string toString(char* s)
{
   string str = checked_malloc(strlen(s)+1);
   string s2 = s + 1;
   strcpy(str,s2);
   str[strlen(str)-1]='\0';
   string str2 = checked_malloc(strlen(s)+1);
   int count = 0;
   for(int i = 0; i < strlen(str);i++)
   {
      if(str[i] == '\\')
      {
        i++;
        if(str[i] == 'n')   
           str2[count++] = '\n';
        else if(str[i] == 't')
          str2[count++] = '\t';
        else if(str[i] == '\\')
          str2[count++] = '\\';
        else if(str[i] == '\"')
          str2[count++] = '\"';
        else
       {
          str2[count++] = '\\';
          str2[count++] = str[i];
       }
      }
      else
        str2[count++] = str[i];
   }
   if(strlen(str) == 0)
   {
      strcpy(str2,"(null)");
   }
   free(str);
   return str2;
}


%}
  /* You can add lex definitions here. */
%start COMMENT 
%%
  /* 
  * Below are some examples, which you can wipe out
  * and write reguler expressions and actions of your own.
  */ 
<INITIAL>"/*" {adjust();BEGIN COMMENT;}
<INITIAL>\"[^\"]*\"    {adjust(); yylval.sval = toString(yytext);return STRING;}
<INITIAL>array {adjust();return ARRAY;}
<INITIAL>if  {adjust(); return IF;}
<INITIAL>then  {adjust(); return THEN;}
<INITIAL>else  {adjust(); return ELSE;}
<INITIAL>while  {adjust(); return WHILE;}
<INITIAL>for  {adjust(); return FOR;}
<INITIAL>to  {adjust(); return TO;}
<INITIAL>do  {adjust(); return DO;}
<INITIAL>let  {adjust(); return LET;}
<INITIAL>in  {adjust(); return IN;}
<INITIAL>end  {adjust(); return END;}
<INITIAL>of  {adjust(); return OF;}
<INITIAL>break  {adjust(); return BREAK;}
<INITIAL>nil  {adjust(); return NIL;}
<INITIAL>function  {adjust(); return FUNCTION;}
<INITIAL>var  {adjust(); return VAR;}
<INITIAL>type  {adjust(); return TYPE;}
<INITIAL>[a-zA-Z][a-zA-Z0-9_]* {adjust();  yylval.sval = String(yytext); return ID;}
<INITIAL>" "  {adjust(); continue;} 
<INITIAL>\t   {adjust();continue;}
<INITIAL>\n	 {adjust(); EM_newline();continue; }
<INITIAL>":="     {adjust(); return ASSIGN;}
<INITIAL>","	 {adjust(); return COMMA;}
<INITIAL>":"     {adjust(); return COLON;}
<INITIAL>";"     {adjust(); return SEMICOLON;}
<INITIAL>"("     {adjust(); return LPAREN;}
<INITIAL>")"     {adjust(); return RPAREN;}
<INITIAL>"["     {adjust(); return LBRACK;}
<INITIAL>"]"     {adjust(); return RBRACK;}
<INITIAL>"{"     {adjust(); return LBRACE;}
<INITIAL>"}"     {adjust(); return RBRACE;}
<INITIAL>"."     {adjust(); return DOT;}
<INITIAL>"+"     {adjust(); return PLUS;}
<INITIAL>"-"     {adjust(); return MINUS;}
<INITIAL>"*"     {adjust(); return TIMES;}
<INITIAL>"/"     {adjust(); return DIVIDE;}
<INITIAL>"="     {adjust(); return EQ;}
<INITIAL>"<>"     {adjust(); return NEQ;}
<INITIAL>"<="     {adjust(); return LE;}
<INITIAL>"<"     {adjust(); return LT;}
<INITIAL>">="     {adjust(); return GE;}
<INITIAL>">"     {adjust(); return GT;}
<INITIAL>"&"     {adjust(); return AND;}
<INITIAL>"|"     {adjust(); return OR;}
<INITIAL>[0-9]+	 {adjust(); yylval.ival=atoi(yytext); return INT;}
<INITIAL>.	 {adjust(); EM_error(EM_tokPos,"illegal token");}
<COMMENT>"*/" {adjust();BEGIN INITIAL;}
<COMMENT>\n	 {adjust(); EM_newline();continue; }
<COMMENT>.    {adjust();continue;}




