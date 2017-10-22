/*
 * main.c
 */

#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "types.h"
#include "absyn.h"
#include "errormsg.h"
#include "temp.h" /* needed by translate.h */
#include "tree.h" /* needed by frame.h */
#include "assem.h"
#include "frame.h" /* needed by translate.h and printfrags prototype */
#include "translate.h"
#include "env.h"
#include "semant.h" /* function prototype for transProg */
#include "canon.h"
#include "prabsyn.h"
#include "printtree.h"
//#include "escape.h" /* needed by escape analysis */
#include "parse.h"
#include "codegen.h"
#include "regalloc.h"
#include "string.h"

extern bool anyErrors;

/* print the assembly language instructions to filename.s */
static void doProc(FILE *out, F_frame frame, T_stm body)
{
 AS_proc proc;
 T_stmList stmList;
 AS_instrList iList;

 F_tempMap = Temp_empty();
 body = F_procEntryExit1(frame,body);
 stmList = C_linearize(body);
 stmList = C_traceSchedule(C_basicBlocks(stmList));
 /* printStmList(stdout, stmList);*/
 iList  = F_codegen(frame, stmList); /* 9 */
 iList = F_procEntryExit2(iList);
 struct RA_result ra = RA_regAlloc(frame, iList);  /* 10, 11 */
 proc = F_procEntryExit3(frame,iList);
 fprintf(out,"%s",proc->prolog);
 AS_printInstrList (out, iList,Temp_layerMap(F_tempMap, ra.coloring));
 fprintf(out, "%s\n",proc->epilog);
}

int main(int argc, string *argv)
{
 A_exp absyn_root;
 S_table base_env, base_tenv;
 F_fragList frags;
 char outfile[100];
 FILE *out = stdout;

 if (argc==2) {
   absyn_root = parse(argv[1]);
   if (!absyn_root)
     return 1;
     
#if 0
   pr_exp(out, absyn_root, 0); /* print absyn data structure */
   fprintf(out, "\n");
#endif
	//If you have implemented escape analysis, uncomment this
   //Esc_findEscape(absyn_root); /* set varDec's escape field */

   frags = SEM_transProg(absyn_root);
   if (anyErrors) return 1; /* don't continue */

   /* convert the filename */
   sprintf(outfile, "%s.s", argv[1]);
   out = fopen(outfile, "w");
   /* Chapter 8, 9, 10, 11 & 12 */
   for (;frags;frags=frags->tail)
     if (frags->head->kind == F_procFrag) 
       doProc(out, frags->head->u.proc.frame, frags->head->u.proc.body);
     else if (frags->head->kind == F_stringFrag){
       int len = strlen(frags->head->u.stringg.str);
       string str = frags->head->u.stringg.str;
       /*char* newstr = checked_malloc(2*len);
       int newlen = strlen(newstr);
       for(int i = 0;i < newlen;i++)
           newstr[i] = 0;
       int count =0;
       for(int i =0;i<len;i++){
          if(str[i] == '\n'){
             newstr[count++] = '\\';
             newstr[count++] = 'n';
          }
          else
            newstr[count++] = str[i];
       }
       newlen = strlen(newstr);
       char* lenascii = (char*)(&newlen);*/
       char* lenascii = (char*)(&len);
       string p = checked_malloc(5);
       for(int i =0;i<4;i++)
          p[i] = lenascii[i];
      //fprintf(out, ".section .rodata\n.%s:\n.string \"%c%c%c%c%s\"\n", S_name(frags->head->u.stringg.label),p[0],p[1],p[2],p[3],newstr);
       fprintf(out, ".section .rodata\n.%s:\n.string \"%c%c%c%c%s\"\n", S_name(frags->head->u.stringg.label),p[0],p[1],p[2],p[3],str);
     }

   fclose(out);
   return 0;
 }
 EM_error(0,"usage: tiger file.tig");
 return 1;
}
