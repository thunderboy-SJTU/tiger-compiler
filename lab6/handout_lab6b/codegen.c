#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "errormsg.h"
#include "tree.h"
#include "assem.h"
#include "frame.h"
#include "assem.h"
#include "codegen.h"
#include "table.h"

//Lab 6: your code here
static AS_instrList iList = NULL, last = NULL;
static void emit(AS_instr inst){
   if(last != NULL)
      last = last->tail = AS_InstrList(inst,NULL);
   else
      last = iList = AS_InstrList(inst,NULL);
}

static Temp_tempList L(Temp_temp temp,Temp_tempList list){
  return Temp_TempList(temp,list);
}

static Temp_tempList calldefs(){
   return F_CallerSaves();
}

static Temp_temp munchExp(T_exp e);

static Temp_tempList munchArgs(int i,T_expList args)
{
   if(args == NULL)
      return NULL;
   Temp_temp r = munchExp(args->head);
   Temp_tempList l = munchArgs(i+1,args->tail);
   emit(AS_Oper("pushl `s0\n",NULL,L(r,NULL),NULL));
   return Temp_TempList(r,l);
}

static Temp_temp munchExp(T_exp e){
   switch(e->kind){
      case T_MEM:{
         Temp_temp r = Temp_newtemp();
         if(e->u.MEM->kind == T_BINOP && e->u.MEM->u.BINOP.op == T_plus && e->u.MEM->u.BINOP.right->kind == T_CONST){
            int i = e->u.MEM->u.BINOP.right->u.CONST;
            T_exp e1 = e->u.MEM->u.BINOP.left;
            char* str = checked_malloc(50);
            sprintf(str,"movl %d(`s0),`d0\n",i);
            emit(AS_Oper(String(str),L(r,NULL),L(munchExp(e1),NULL),NULL));
            free(str);
            return r;
         }
         else if(e->u.MEM->kind == T_BINOP && e->u.MEM->u.BINOP.op == T_plus && e->u.MEM->u.BINOP.left->kind == T_CONST){
            int i = e->u.MEM->u.BINOP.left->u.CONST;
            T_exp e1 = e->u.MEM->u.BINOP.right;
            char* str = checked_malloc(50);
            sprintf(str,"movl %d(`s0),`d0\n",i);
            emit(AS_Oper(String(str),L(r,NULL),L(munchExp(e1),NULL),NULL));
            free(str);
            return r;
         }
         else if(e->u.MEM->kind == T_CONST){
            int i = e->u.MEM->u.CONST;
            char* str = checked_malloc(50);
            sprintf(str,"movl %d,`d0\n",i);
            emit(AS_Oper(String(str),L(r,NULL),NULL,NULL));
            free(str);
            return r;
         }
         else{
            T_exp e1 = e->u.MEM;
            emit(AS_Oper("movl (`s0),`d0\n",L(r,NULL),L(munchExp(e1),NULL),NULL));
            return r;
         }
      }
      case T_BINOP:{
        switch(e->u.BINOP.op){
           case T_plus:{
             Temp_temp r = Temp_newtemp();
             T_exp e1 = e->u.BINOP.left;
             T_exp e2 = e->u.BINOP.right;
             Temp_temp temp = munchExp(e1);
             Temp_temp temp2 = munchExp(e2);
             emit(AS_Move("movl `s0,`d0\n",L(r,NULL),L(temp,NULL)));
             emit(AS_Oper("addl `s1,`d0\n",L(r,NULL),L(r,L(temp2,NULL)),NULL));
             return r;
           }
           case T_minus:{
             Temp_temp r = Temp_newtemp();
             T_exp e1 = e->u.BINOP.left;
             T_exp e2 = e->u.BINOP.right;
             Temp_temp temp = munchExp(e1);
             Temp_temp temp2 = munchExp(e2);
             emit(AS_Move("movl `s0,`d0\n",L(r,NULL),L(temp,NULL)));
             emit(AS_Oper("subl `s1,`d0\n",L(r,NULL),L(r,L(temp2,NULL)),NULL));
             return r;
           }
           case T_mul:{
             Temp_temp r = Temp_newtemp();
             T_exp e1 = e->u.BINOP.left;
             T_exp e2 = e->u.BINOP.right;
             Temp_temp temp = munchExp(e1);
             Temp_temp temp2 = munchExp(e2);
             emit(AS_Move("movl `s0,`d0\n",L(r,NULL),L(temp,NULL)));
             emit(AS_Oper("imull `s1,`d0\n",L(r,NULL),L(r,L(temp2,NULL)),NULL));
             return r;
           }
           case T_div:{  
             Temp_temp r = Temp_newtemp();           
             T_exp e1 = e->u.BINOP.left;
             T_exp e2 = e->u.BINOP.right;
             Temp_temp temp = munchExp(e1);
             Temp_temp temp2 = munchExp(e2);
             emit(AS_Move("movl `s0,`d0\n",L(F_EAX(),NULL),L(temp,NULL)));
             emit(AS_Oper("cltd\n",L(F_EAX(),L(F_EDX(),NULL)),NULL,NULL));
             emit(AS_Oper("idivl `s0\n",L(F_EAX(),L(F_EDX(),NULL)),L(temp2,NULL),NULL));
             emit(AS_Move("movl `s0,`d0\n",L(r,NULL),L(F_EAX(),NULL)));
             return r;
           }
           default:
             assert(0);
        }
      }
      case T_CONST:{
           Temp_temp r = Temp_newtemp();
           int i = e->u.CONST;
           char* str = checked_malloc(50);
           sprintf(str,"movl $%d,`d0\n",i);
           emit(AS_Oper(String(str),L(r,NULL),NULL,NULL));
           free(str);
           return r;
       }
       case T_TEMP:{
           return e->u.TEMP;
       }
       case T_NAME:{     //不知道应不应该这样写
           Temp_temp r = Temp_newtemp();
           char* str = checked_malloc(50);
           sprintf(str,"movl $.%s,`d0\n",Temp_labelstring(e->u.NAME));
           emit(AS_Oper(String(str),L(r,NULL),NULL,NULL));
           free(str);
           return r;
       }  
       case T_CALL:{
           Temp_label name = e->u.CALL.fun->u.NAME;          
           Temp_tempList l = munchArgs(0,e->u.CALL.args);
           char* str = checked_malloc(50);
           sprintf(str,"call %s\n",S_name(name));
           emit(AS_Oper(String(str),calldefs(),l,NULL));
           return F_RV();
       }   
   }
   assert(0);
}

static void munchStm(T_stm s){
    switch(s->kind){
       case T_MOVE:{
           if(s->u.MOVE.dst->kind == T_MEM){
               if(s->u.MOVE.dst->u.MEM->kind == T_BINOP && s->u.MOVE.dst->u.MEM->u.BINOP.op == T_plus && s->u.MOVE.dst->u.MEM->u.BINOP.right->kind == T_CONST){
                   int i = s->u.MOVE.dst->u.MEM->u.BINOP.right->u.CONST;
                   T_exp e1 = s->u.MOVE.dst->u.MEM->u.BINOP.left;
                   T_exp e2 = s->u.MOVE.src;
                   char* str = checked_malloc(50);
                   sprintf(str,"movl `s0,%d(`s1)\n",i);
                   emit(AS_Oper(String(str),NULL,L(munchExp(e2),L(munchExp(e1),NULL)),NULL));
                   free(str);
                   return;
               }
               else if(s->u.MOVE.dst->u.MEM->kind == T_BINOP && s->u.MOVE.dst->u.MEM->u.BINOP.op == T_plus && s->u.MOVE.dst->u.MEM->u.BINOP.left->kind == T_CONST){
                   int i = s->u.MOVE.dst->u.MEM->u.BINOP.left->u.CONST;
                   T_exp e1 = s->u.MOVE.dst->u.MEM->u.BINOP.right;
                   T_exp e2 = s->u.MOVE.src;
                   char* str = checked_malloc(50);
                   sprintf(str,"movl `s0,%d(`s1)\n",i);
                   emit(AS_Oper(String(str),NULL,L(munchExp(e2),L(munchExp(e1),NULL)),NULL));
                   free(str);
                   return;
               }
               else if(s->u.MOVE.dst->u.MEM->kind == T_CONST){
                   int i = s->u.MOVE.dst->u.MEM->u.CONST;
                   T_exp e2 = s->u.MOVE.src;
                   char* str = checked_malloc(50);
                   sprintf(str,"movl `s0,%d\n",i);
                   emit(AS_Oper(String(str),NULL,L(munchExp(e2),NULL),NULL));
                   free(str);
                   return;
               }
               else{
                   T_exp e1 = s->u.MOVE.dst->u.MEM;
                   T_exp e2 = s->u.MOVE.src;
                   emit(AS_Oper("movl `s0,(`s1)\n",NULL,L(munchExp(e2),L(munchExp(e1),NULL)),NULL));
                   return;
               }
           }
           else if(s->u.MOVE.dst->kind == T_TEMP){
                   T_exp e2 = s->u.MOVE.src;
                   Temp_temp temp = s->u.MOVE.dst->u.TEMP;
                   emit(AS_Move("movl `s0,`d0\n",L(temp,NULL),L(munchExp(e2),NULL)));
                   return;
           }
       }
       case T_LABEL:{
           char* str = checked_malloc(50);
           sprintf(str,"%s:\n",Temp_labelstring(s->u.LABEL));
           emit(AS_Label(String(str),s->u.LABEL));
           free(str);
           return;
       }
       case T_JUMP:{
           Temp_labelList labs = s->u.JUMP.jumps;
           emit(AS_Oper("jmp `j0\n",NULL,NULL,AS_Targets(labs)));
           return;
       }
       case T_CJUMP:{
           Temp_temp left = munchExp(s->u.CJUMP.left);
           Temp_temp right = munchExp(s->u.CJUMP.right);
           emit(AS_Oper("cmpl `s0,`s1\n",NULL,L(right,L(left,NULL)),NULL));
           switch(s->u.CJUMP.op){
              case T_eq:{
                 emit(AS_Oper("je `j0\n",NULL,NULL,AS_Targets(Temp_LabelList(s->u.CJUMP.true,NULL))));
                 return;
              }
              case T_ne:{
                 emit(AS_Oper("jne `j0\n",NULL,NULL,AS_Targets(Temp_LabelList(s->u.CJUMP.true,NULL))));
                 return;
              }
              case T_lt:{
                 emit(AS_Oper("jl `j0\n",NULL,NULL,AS_Targets(Temp_LabelList(s->u.CJUMP.true,NULL))));
                 return;
              }
              case T_gt:{
                 emit(AS_Oper("jg `j0\n",NULL,NULL,AS_Targets(Temp_LabelList(s->u.CJUMP.true,NULL))));
                 return;
              }  
              case T_le:{
                 emit(AS_Oper("jle `j0\n",NULL,NULL,AS_Targets(Temp_LabelList(s->u.CJUMP.true,NULL))));
                 return;
              }
              case T_ge:{
                 emit(AS_Oper("jge `j0\n",NULL,NULL,AS_Targets(Temp_LabelList(s->u.CJUMP.true,NULL))));
                 return;
              }
              default:
                 assert(0);
           }
       }
       case T_EXP:{
           munchExp(s->u.EXP);
           return;
       }
   }
}

AS_instrList F_codegen(F_frame f, T_stmList stmList) {
   AS_instrList list;
   T_stmList sl;
   for(sl = stmList;sl;sl = sl->tail)
      munchStm(sl->head);
   list = iList;
   iList = last = NULL;
   return list;
}
          
                   
                    
              
             
          
           
           
    


