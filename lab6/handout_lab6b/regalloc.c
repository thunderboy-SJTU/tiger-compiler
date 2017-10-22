#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "color.h"
#include "flowgraph.h"
#include "liveness.h"
#include "regalloc.h"
#include "table.h"


static bool inTempList(Temp_tempList templist,Temp_temp temp){
    for(;templist != NULL;templist = templist->tail){
        Temp_temp tmp = templist->head;
        if(tmp == temp)
           return TRUE;
    }
    return FALSE;
}



static void removeRedundantInstr(AS_instrList *il,Temp_map coloring){
   AS_instrList pre = NULL;
   AS_instrList curr = (*il);
   for(;curr != NULL;curr =curr->tail){
       AS_instr instr = curr->head;
       if(instr->kind == I_MOVE){
          Temp_tempList dst = instr->u.MOVE.dst;
          Temp_tempList src = instr->u.MOVE.src;
          Temp_temp d = dst->head;
          Temp_temp s = src->head;
          string dstt = Temp_look(coloring,d);
          string srcc = Temp_look(coloring,s);
          if(dstt == srcc){
             if(pre == NULL) 
                 (*il) = curr->tail;
             else
                 pre->tail = curr->tail;
          }
          else
             pre = curr;
       }
       else 
          pre = curr;
   }
}
      
             


static Temp_tempList changeTempList(Temp_tempList templist,Temp_temp old,Temp_temp new){
    Temp_tempList list = NULL;
    Temp_tempList tail = NULL;
    for(;templist != NULL;templist = templist->tail){
       Temp_temp tmp = templist->head;
       if(tmp == old)
          tmp = new;
       if(tail == NULL)
          list = tail = Temp_TempList(tmp,NULL);
       else
          tail = tail->tail = Temp_TempList(tmp,NULL);
    }
    return list;
}

static int getIndex(TAB_table t,F_frame f,Temp_temp temp){
   F_access acc = TAB_look(t,temp);
   if(acc == NULL){
      acc = F_allocLocal(f,TRUE);
      TAB_enter(t,temp,acc);
   }
   return F_getCount(acc);
}

void rewriteProgram(F_frame f,AS_instrList il,Temp_tempList spills){
   TAB_table offsetTable = TAB_empty();
   for(;spills != NULL;spills = spills->tail){
      Temp_temp temp = spills->head;
      AS_instrList pre = NULL;
      AS_instrList curr = il;
      for(;curr!= NULL;curr = curr->tail){
        AS_instr instr = curr->head;
        Temp_tempList uses = NULL;
        Temp_tempList defs = NULL;
        if(instr->kind == I_OPER){
           uses = instr->u.OPER.src;
           defs = instr->u.OPER.dst;
        }
        else if(instr->kind == I_MOVE){
           uses = instr->u.MOVE.src;
           defs = instr->u.MOVE.dst;
        }
        if(inTempList(uses,temp)){
            Temp_temp newtemp = Temp_newtemp();
            if(instr->kind == I_OPER)
               instr->u.OPER.src = changeTempList(uses,temp,newtemp);
            else if(instr->kind == I_MOVE)
               instr->u.MOVE.src = changeTempList(uses,temp,newtemp);
            int index = getIndex(offsetTable,f,temp);
            char* str = checked_malloc(50);
            sprintf(str,"movl %d(`s0),`d0\n",index);
            AS_instr newInstr = AS_Oper(String(str),Temp_TempList(newtemp,NULL),Temp_TempList(F_FP(),NULL),NULL);
            pre = pre->tail = AS_InstrList(newInstr,curr);
        }
        if(inTempList(defs,temp)){
            Temp_temp newtemp = Temp_newtemp();
            if(instr->kind == I_OPER)
               instr->u.OPER.dst = changeTempList(defs,temp,newtemp);
            else if(instr->kind == I_MOVE)
               instr->u.MOVE.dst = changeTempList(defs,temp,newtemp);
            int index = getIndex(offsetTable,f,temp);
            char* str = checked_malloc(50);
            sprintf(str,"movl `s0,%d(`s1)\n",index);
            AS_instr newInstr = AS_Oper(String(str),NULL,Temp_TempList(newtemp,Temp_TempList(F_FP(),NULL)),NULL);
            curr = curr->tail = AS_InstrList(newInstr,curr->tail);
        }
        pre = curr;
     }
   }
}

struct RA_result RA_regAlloc(F_frame f, AS_instrList il) {
	//your code here.
	struct RA_result ret;
        G_graph ig =  FG_AssemFlowGraph(il,f);
        Temp_map initial = F_initial();
        Temp_tempList regs = F_allocRegs();
        struct COL_result col_result = COL_color(ig,initial,regs);
        if(col_result.spills != NULL){
           rewriteProgram(f,il,col_result.spills);
           return RA_regAlloc(f,il);
        }
        removeRedundantInstr(&il,col_result.coloring);
        ret.coloring = col_result.coloring;
        ret.il = il;
	return ret;
}
