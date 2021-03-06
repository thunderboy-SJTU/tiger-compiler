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

Temp_temp F_EAX(void)
{
   static Temp_temp eax = NULL;
    if(eax == NULL)
       eax = Temp_newtemp();
    return eax;
}

Temp_temp F_EBX(void)
{
    static Temp_temp ebx = NULL;
    if(ebx == NULL)
       ebx = Temp_newtemp();
    return ebx;
}

Temp_temp F_ECX(void)
{
    static Temp_temp ecx = NULL;
    if(ecx == NULL)
       ecx = Temp_newtemp();
    return ecx;
}
Temp_temp F_EDX(void)
{
    static Temp_temp edx = NULL;
    if(edx == NULL)
       edx = Temp_newtemp();
    return edx;
}
Temp_temp F_ESI(void)
{
   static Temp_temp esi = NULL;
    if(esi == NULL)
       esi = Temp_newtemp();
    return esi;

}

Temp_temp F_EDI(void)
{
   static Temp_temp edi = NULL;
    if(edi == NULL)
       edi = Temp_newtemp();
    return edi;
}

Temp_temp F_ESP(void)
{
   static Temp_temp esp = NULL;
    if(esp == NULL)
       esp = Temp_newtemp();
    return esp;
}

Temp_temp F_EBP(void)
{
   static Temp_temp ebp = NULL;
    if(ebp == NULL)
       ebp = Temp_newtemp();
    return ebp;
}

Temp_temp F_FP()
{
    return F_EBP();
}

Temp_temp F_RV(void)
{
    return F_EAX();
}

Temp_temp F_SP(void)
{
    return F_ESP();
}

Temp_tempList F_SpecialRegs()
{
   static Temp_tempList list = NULL;
   if(list == NULL)
      list = Temp_TempList(F_ESP(),Temp_TempList(F_EBP(),NULL));
   return list;
}

Temp_tempList F_CallerSaves()
{
   static Temp_tempList list = NULL;
   if(list == NULL)
      list = Temp_TempList(F_EAX(),Temp_TempList(F_EDX(),Temp_TempList(F_ECX(),NULL)));
   return list;
}


Temp_tempList F_CalleeSaves()
{
   static Temp_tempList list = NULL;
   if(list == NULL)
      list = Temp_TempList(F_EBX(),Temp_TempList(F_ESI(),Temp_TempList(F_EDI(),NULL)));
   return list;
}


T_stm F_procEntryExit1(F_frame frame, T_stm stm)
{
   return stm; //暂且这样吧
}

static Temp_tempList returnSink = NULL;

AS_instrList F_procEntryExit2(AS_instrList body)
{
   if(!returnSink)
     returnSink = Temp_TempList(F_FP(),Temp_TempList(F_SP(),F_CalleeSaves()));
   return AS_splice(body,AS_InstrList(AS_Oper("",NULL,returnSink,NULL),NULL));
}
  
AS_proc F_procEntryExit3(F_frame frame,AS_instrList body)
{
   char buf[100];
   sprintf(buf,"PROCEDURE %s\n",S_name(frame->name));
   return AS_Proc(String(buf),body,"END\n");
}



