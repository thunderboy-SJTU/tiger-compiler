#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "table.h"
#include "tree.h"
#include "frame.h"


/*Lab5: Your implementation here.*/

const int F_wordSize = 4;

struct F_frame_ {
   Temp_label name;
   F_accessList formals;
   int count;
};

struct F_access_
{
   enum{inFrame,inReg} kind;
   union{
     int offset;     
     Temp_temp reg; 
   }u;
 
};

static F_access InFrame(int offset)
{
   F_access access = checked_malloc(sizeof(*access));
   access->kind = inFrame;
   access->u.offset = offset;
   return access;
}

static F_access InReg(Temp_temp reg)
{
   F_access access = checked_malloc(sizeof(*access));
   access->kind = inReg;
   access->u.reg = reg;
   return access;
}

F_frame F_newFrame(Temp_label name, U_boolList formals)
{
    F_frame frame = checked_malloc(sizeof(*frame));
    F_accessList list = checked_malloc(sizeof(*list));
    F_accessList tmpList = list;
    list->head = NULL;
    list->tail = NULL;
    frame->name = name;
    frame->count = 0;
    for( ;formals != NULL; formals = formals->tail)
    {
       F_access access = F_allocLocal(frame,formals->head);
       if(tmpList->head == NULL)
          tmpList->head = access;
       else 
       {
          F_accessList listtail = checked_malloc(sizeof(*listtail));
          listtail->head = access;
          listtail->tail = NULL;
          tmpList->tail = listtail;
          tmpList = tmpList->tail;          
       }      
    }
    frame->formals = list;
    return frame;
}


Temp_label F_name(F_frame f)
{
   return f->name;
}


F_accessList F_formals(F_frame f)
{
   return f->formals;
}


F_access F_allocLocal(F_frame frame, bool escape)
{
    F_access access;
    frame->count++;
    if(escape == 1)
        access = InFrame((-1)*frame->count* F_wordSize);
    else
        access = InReg(Temp_newtemp());
    return access;
}

T_exp F_externalCall(string s,T_expList args)
{
   return T_Call(T_Name(Temp_namedlabel(s)),args);
}


Temp_temp F_FP()
{
    static Temp_temp fp = NULL;
    if(fp == NULL)
       fp = Temp_newtemp();
    return fp;
}

T_exp F_Exp(F_access acc,T_exp framePtr){
  T_exp exp;
  if(acc->kind == inFrame)
    exp = T_Mem(T_Binop(T_plus,framePtr,T_Const(acc->u.offset*F_wordSize)));
  else
    exp = T_Temp(acc->u.reg);
  return exp; 
}



F_frag F_StringFrag(Temp_label label, string str) {
        F_frag frag = checked_malloc(sizeof(*frag));
        frag->kind = F_stringFrag;
        frag->u.stringg.label = label;
        frag->u.stringg.str = str;
	return frag;
}

F_frag F_ProcFrag(T_stm body, F_frame frame) {
        F_frag frag = checked_malloc(sizeof(*frag));
        frag->kind = F_procFrag;
        frag->u.proc.body = body;
        frag->u.proc.frame = frame;
	return frag;
}

F_fragList F_FragList(F_frag head, F_fragList tail) {
        F_fragList fragList = checked_malloc(sizeof(*fragList));
        fragList->head = head;
        fragList->tail = tail;
	return fragList;
}

Temp_temp F_RV(void)
{
    return NULL;
}

T_stm F_procEntryExit1(F_frame frame, T_stm stm)
{
   return stm; //暂且这样吧
}



