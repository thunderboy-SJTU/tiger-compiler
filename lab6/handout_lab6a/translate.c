#include <stdio.h>
#include "util.h"
#include "table.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "tree.h"
#include "printtree.h"
#include "frame.h"
#include "translate.h"



F_fragList fragList = NULL;      

Tr_expList Tr_ExpList(Tr_exp head,Tr_expList tail)
{
   Tr_expList list = checked_malloc(sizeof(*list));
   list->head = head;
   list->tail = tail;
   return list;
}


Tr_accessList Tr_AccessList(Tr_access head,Tr_accessList tail)
{
  Tr_accessList list = checked_malloc(sizeof(*list));
  list->head = head;
  list->tail = tail;
  return list;
}

Tr_level Tr_outermost(void)
{
   static Tr_level outermost = NULL;
   if(outermost == NULL)
      outermost = Tr_newLevel(NULL,Temp_newlabel(),NULL);
   return outermost;
}

Tr_level Tr_newLevel(Tr_level parent,Temp_label name,U_boolList formals)
{
    Tr_level level = checked_malloc(sizeof(*level));
    Tr_accessList list = checked_malloc(sizeof(*list));
    Tr_accessList tmpList = list;
    list->head = NULL;
    list->tail = NULL;
    level->parent = parent;
    level->frame = F_newFrame(name,U_BoolList(TRUE,formals));
    level->name = name;
    F_accessList f_formals = F_formals(level->frame);
    f_formals = f_formals->tail;         //跳过静态链
    for( ;f_formals != NULL; f_formals = f_formals->tail)
    {
       Tr_access acc = checked_malloc(sizeof(*acc));
       acc->level = level;
       acc->access = f_formals->head;
       if(tmpList->head == NULL)
          tmpList->head = acc;
       else 
       {
          Tr_accessList listtail = checked_malloc(sizeof(*listtail));
          listtail->head = acc;
          listtail->tail = NULL;
          tmpList->tail = listtail;
          tmpList = tmpList->tail;          
       }      
    }
    level->formals = list;
    return level;
}

Tr_accessList Tr_formals(Tr_level level) 
{
   return level->formals;    
}

Tr_access Tr_allocLocal(Tr_level level, bool escape)
{
   F_frame frame = level->frame;
   Tr_access acc = checked_malloc(sizeof(*acc));
   acc->level = level;
   acc->access = F_allocLocal(frame,escape);
   return acc;
}


static patchList PatchList(Temp_label *head, patchList tail)
{
   patchList patchlist = checked_malloc(sizeof(*patchlist));
   patchlist->head = head;
   patchlist->tail = tail;
   return patchlist;
}

static Tr_exp Tr_Ex(T_exp ex)
{
   Tr_exp tr_ex = checked_malloc(sizeof(*tr_ex));
   tr_ex->kind = Tr_ex;
   tr_ex->u.ex = ex;
   return tr_ex;
}

static Tr_exp Tr_Nx(T_stm nx)
{
  Tr_exp tr_nx = checked_malloc(sizeof(*tr_nx));
  tr_nx->kind = Tr_nx;
  tr_nx->u.nx = nx;
  return tr_nx;
}

static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm)
{
  Tr_exp tr_cx = checked_malloc(sizeof(*tr_cx));
  tr_cx->kind = Tr_cx;
  tr_cx->u.cx.trues = trues;
  tr_cx->u.cx.falses = falses;
  tr_cx->u.cx.stm = stm;
  return tr_cx;
}

void doPatch(patchList tList,Temp_label label)
{
  for(;tList;tList = tList->tail)
    *(tList->head) = label;
}

patchList joinPatch(patchList first, patchList second)
{
  if(!first)
     return second;
  for(;first->tail;first = first->tail)
       ;
  first->tail = second;
  return first;
}

static T_exp unEx(Tr_exp e)
{
  switch(e->kind){
     case Tr_ex:
       return e->u.ex;
     case Tr_cx:{
       Temp_temp r = Temp_newtemp();
       Temp_label t = Temp_newlabel(), f = Temp_newlabel();
       doPatch(e->u.cx.trues,t);
       doPatch(e->u.cx.falses,f);
       return T_Eseq(T_Move(T_Temp(r),T_Const(1)),
                T_Eseq(e->u.cx.stm,
                   T_Eseq(T_Label(f),
                     T_Eseq(T_Move(T_Temp(r),T_Const(0)),
                        T_Eseq(T_Label(t),T_Temp(r))))));
     }
     case Tr_nx:
       return T_Eseq(e->u.nx,T_Const(0));
    }
    assert(0);
}

static T_stm unNx(Tr_exp e)
{
  switch(e->kind){
    case Tr_ex:
       return T_Exp(e->u.ex);
    case Tr_nx:
       return e->u.nx;
    case Tr_cx:
       return T_Exp(unEx(e));
  }
  assert(0);
}

static struct Cx unCx(Tr_exp e)
{
  switch(e->kind){
    case Tr_ex:{
       struct Cx cx;
       cx.stm = T_Cjump(T_ne,e->u.ex,T_Const(0),NULL,NULL);
       cx.trues = PatchList(&cx.stm->u.CJUMP.true,NULL);
       cx.falses = PatchList(&cx.stm->u.CJUMP.false,NULL);
       return cx;
    }
    case Tr_cx:{
       return e->u.cx;
    }
    case Tr_nx:
       assert(0);
  }
  assert(0);
}

T_expList toT_ExpList(Tr_expList list)
{
   if(list == NULL)
      return NULL;
   T_expList tlist = checked_malloc(sizeof(*tlist));
   tlist->head = NULL;
   tlist->tail = NULL;
   T_expList tmp = tlist;
   T_exp exp;
   for(; list != NULL; list= list->tail)
   {
     exp = unEx(list->head);
     if(tmp->head == NULL)
        tmp->head = exp;
     else 
     {
       T_expList endlist = checked_malloc(sizeof(*endlist));     
       endlist->head = exp;
       endlist->tail = NULL;
       tmp->tail = endlist;
       tmp = tmp->tail;   
     }
   }
   return tlist;
}

T_stmList toT_StmList(Tr_expList list)
{
   if(list == NULL)
      return NULL;
   T_stmList tlist = NULL;
   T_stm stm;
   for(; list != NULL; list= list->tail)
   {
     stm = unNx(list->head);
     tlist =  T_StmList(stm, tlist);     
   }
   return tlist;
}

T_exp getFunLink(Tr_level dec_level,Tr_level use_level){   //let语句中，声明的函数比原函数高一层，所以调用函数的话，需要用parent比，这bug找了半天，特记之
   T_exp fp = T_Temp(F_FP());
   while(dec_level->parent != use_level){
     fp = T_Mem(T_Binop(T_plus,T_Const((-1)*F_wordSize),fp));
     use_level = use_level->parent;
  }
  return fp;
}

T_exp getLink(Tr_level dec_level,Tr_level use_level){
   T_exp fp = T_Temp(F_FP());
   while(dec_level != use_level){
     fp = T_Mem(T_Binop(T_plus,T_Const((-1)*F_wordSize),fp));
     use_level = use_level->parent;
  }
  return fp;
}


Tr_exp Tr_simpleVar(Tr_access acc,Tr_level level)
{
   Tr_level dec_level = acc->level;
   T_exp fp = getLink(dec_level,level);
   return Tr_Ex(F_Exp(acc->access,fp));
}

Tr_exp Tr_fieldVar(Tr_exp record, int index)
{
   T_exp exp = T_Mem(T_Binop(T_plus,unEx(record),T_Binop(T_mul,T_Const(index), T_Const(F_wordSize))));
   return Tr_Ex(exp);
}

Tr_exp Tr_subscriptVar(Tr_exp array, Tr_exp index)
{
   T_exp exp = T_Mem(T_Binop(T_plus,unEx(array),T_Binop(T_mul,unEx(index), T_Const(F_wordSize))));
   return Tr_Ex(exp);
}

Tr_exp Tr_nilExp()
{
   return Tr_Ex(T_Const(0));
}

Tr_exp Tr_intExp(int number)
{
   return Tr_Ex(T_Const(number));
}

Tr_exp Tr_stringExp(string str)
{
   Temp_label label = Temp_newlabel();
   F_frag frag = F_StringFrag(label,str);               
   fragList = F_FragList(frag,fragList);   
   return Tr_Ex(T_Name(label));
}

Tr_exp Tr_callExp(Temp_label name, Tr_level dec, Tr_level use,Tr_expList args)
{
   T_exp staticLink = getFunLink(dec,use);
   T_expList targs = toT_ExpList(args);
   return Tr_Ex(T_Call(T_Name(name),T_ExpList(staticLink,targs)));
}


Tr_exp Tr_opExp(A_oper oper, Tr_exp left, Tr_exp right)
{
   int isBinop = 1;
   T_binOp binop;
   T_relOp relop;
   switch(oper){
     case A_plusOp:
        binop = T_plus;
        break;
     case A_minusOp: 
        binop = T_minus;
        break;
     case A_timesOp:
        binop = T_mul;
        break;
     case A_divideOp:
        binop = T_div;
        break;
     case A_eqOp:
        relop = T_eq;
        isBinop = 0;
        break;
     case A_neqOp:
        relop = T_ne;
        isBinop = 0;
        break;
     case A_ltOp:
        relop = T_lt;
        isBinop = 0;
        break;
     case A_leOp:
        relop = T_le;
        isBinop = 0;
        break;
     case A_gtOp:
        relop = T_gt;
        isBinop = 0;
        break;
     case A_geOp:
        relop = T_ge;
        isBinop = 0;
        break;
     default:
        isBinop = 0;
        break;
     }
     if(isBinop == 1)
       return Tr_Ex(T_Binop(binop,unEx(left),unEx(right)));
     else
     {
        T_stm stm = T_Cjump(relop,unEx(left),unEx(right),NULL,NULL);
        patchList trues = PatchList(&stm->u.CJUMP.true,NULL);
        patchList falses = PatchList(&stm->u.CJUMP.false,NULL);
        Tr_exp cx = Tr_Cx(trues,falses,stm);
        return cx;
     }
}

Tr_exp Tr_recordExp(int count, Tr_expList paras)
{
   Temp_temp temp = Temp_newtemp();
   T_expList tparas = toT_ExpList(paras);
   T_expList size = T_ExpList(T_Const(count*F_wordSize),NULL);
   T_stm seq = T_Move(T_Temp(temp),F_externalCall("malloc",size));  
   T_stm stm = NULL;
   int i = 0;
   while(tparas != NULL){
     if(stm == NULL)
        stm = T_Move(T_Mem(T_Binop(T_plus,T_Temp(temp),T_Const(i*F_wordSize))),tparas->head);
     else
        stm = T_Seq(T_Move(T_Mem(T_Binop(T_plus,T_Temp(temp),T_Const(i*F_wordSize))),tparas->head),stm);
     tparas = tparas->tail;
     i++;
  }
  return Tr_Ex(T_Eseq(T_Seq(seq,stm),T_Temp(temp)));
}

Tr_exp Tr_seqExp(Tr_expList stm,Tr_exp exp)
{
    T_stmList stmlist =  toT_StmList(stm);
    T_exp texp;
    if(exp != NULL)
       texp = unEx(exp);
    else
       texp = NULL;
    if(stmlist == NULL)
       return Tr_Ex(texp);
    T_stm head = stmlist->head;
    stmlist = stmlist->tail;
    T_exp seq = T_Eseq(head,texp);
    for(;stmlist != NULL;stmlist = stmlist->tail)
       seq = T_Eseq(stmlist->head,seq);
    return Tr_Ex(seq);
}

Tr_exp Tr_assignExp(Tr_exp left, Tr_exp right)
{
   T_exp t_left = unEx(left);
   T_exp t_right = unEx(right);
   return Tr_Nx(T_Move(t_left,t_right));
}

Tr_exp Tr_ifExp(Tr_exp ifexp, Tr_exp thenexp, Tr_exp elseexp)
{
   struct Cx cx = unCx(ifexp);
   Temp_label t = Temp_newlabel();
   Temp_label f = Temp_newlabel();
   if(elseexp == NULL)
   {
      doPatch(cx.trues,t);
      doPatch(cx.falses,f);
      return Tr_Nx(T_Seq(cx.stm,T_Seq(T_Label(t),T_Seq(unNx(thenexp),T_Label(f)))));
   }
   else
   {
      if(thenexp->kind == Tr_ex && elseexp->kind == Tr_ex)
      {
         Temp_temp r = Temp_newtemp();
         doPatch(cx.trues,t);
         doPatch(cx.falses,f);
         T_exp tvalue = unEx(thenexp);
         T_exp fvalue = unEx(elseexp);
         return Tr_Ex(T_Eseq(T_Move(T_Temp(r),tvalue),T_Eseq(cx.stm,T_Eseq(T_Label(f),T_Eseq(T_Move(T_Temp(r),fvalue),T_Eseq(T_Label(t),T_Temp(r)))))));
      }
      else if(thenexp->kind == Tr_nx && elseexp->kind == Tr_nx)
      {
         doPatch(cx.trues,t);
         doPatch(cx.falses,f);
         T_stm tstm = unNx(thenexp);
         T_stm fstm = unNx(elseexp);
         Temp_label conbine = Temp_newlabel();
         return Tr_Nx(T_Seq(cx.stm,T_Seq(T_Label(t),T_Seq(tstm,T_Seq(T_Jump(T_Name(conbine),Temp_LabelList(conbine,NULL)),T_Seq(T_Label(f),T_Seq(fstm,T_Label(conbine))))))));
      }
      else  //偷懒cx直接用ex实现
      {
         Temp_temp r = Temp_newtemp();
         doPatch(cx.trues,t);
         doPatch(cx.falses,f);
         T_exp tvalue = unEx(thenexp);
         T_exp fvalue = unEx(elseexp);
         return Tr_Ex(T_Eseq(T_Move(T_Temp(r),tvalue),T_Eseq(cx.stm,T_Eseq(T_Label(f),T_Eseq(T_Move(T_Temp(r),fvalue),T_Eseq(T_Label(t),T_Temp(r)))))));
      }
   }
}

Tr_exp Tr_whileExp(Tr_exp test,Tr_exp body, Temp_label done)
{
    struct Cx testt = unCx(test);
    Temp_label t = Temp_newlabel();
    Temp_label test_label = Temp_newlabel();
    doPatch(testt.trues,t);
    doPatch(testt.falses,done);
    return Tr_Nx(T_Seq(T_Label(test_label),T_Seq(testt.stm,T_Seq(T_Label(t),T_Seq(unNx(body),T_Seq(T_Jump(T_Name(test_label),Temp_LabelList(test_label,NULL)),T_Label(done)))))));
}

Tr_exp Tr_forExp(Tr_exp var, Tr_exp lo, Tr_exp hi, Tr_exp body, Temp_label done)
{
   Temp_label begin = Temp_newlabel();
   Temp_label t = Temp_newlabel();
   Temp_label add = Temp_newlabel();
   Temp_temp i = Temp_newtemp();
   T_exp exvar = unEx(var);
   T_exp exhi = unEx(hi);
   T_exp exlo = unEx(lo);
   T_stm begin_jump = T_Cjump(T_le,exvar,exhi,begin,done);
   T_stm end_jump = T_Cjump(T_lt,exvar,exhi,add,done);
   T_stm add_op = T_Seq(T_Move(T_Temp(i),exvar),T_Move(exvar,T_Binop(T_plus,T_Temp(i),T_Const(1))));
   T_stm forexp = T_Seq(T_Move(exvar,exlo),T_Seq(begin_jump,T_Seq(T_Label(begin),T_Seq(unNx(body),T_Seq(end_jump,T_Seq(T_Label(add),T_Seq(add_op,T_Seq(T_Jump(T_Name(begin),Temp_LabelList(begin,NULL)),T_Label(done)))))))));
   return Tr_Nx(forexp);
}  


Tr_exp Tr_breakExp(Temp_label done)
{
    return Tr_Nx(T_Jump(T_Name(done),Temp_LabelList(done,NULL)));
}

Tr_exp Tr_letExp(Tr_expList dec,Tr_exp exp)
{
    T_stmList declist =  toT_StmList(dec);
    T_exp texp = unEx(exp);
    T_stm head = declist->head;
    declist = declist->tail;
    T_exp seq = T_Eseq(head,texp);
    for(;declist != NULL;declist = declist->tail)
       seq = T_Eseq(declist->head,seq);
    return Tr_Ex(seq);
}


Tr_exp Tr_arrayExp(Tr_exp size,Tr_exp value)
{
   T_exp t_size = unEx(size);
   T_exp t_value = unEx(value);
   return Tr_Ex(F_externalCall(String("initArray"),T_ExpList(t_size,T_ExpList(t_value,NULL))));
}

Tr_exp Tr_varDec(Tr_access acc, Tr_exp value) {
    T_exp var = F_Exp(acc->access, T_Temp(F_FP()));
    T_exp val = unEx(value);
    return Tr_Nx(T_Move(var,val));
}

Tr_exp Tr_funDec(Tr_level level, Tr_exp body) {
    T_stm bodystm = unNx(body);
    fragList = F_FragList(F_ProcFrag(bodystm,level->frame),fragList);
    return Tr_Ex(T_Const(0));
}

Tr_exp Tr_typeDec() {
    return Tr_Ex(T_Const(0));
}


F_fragList Tr_getResult(void) {
    return fragList;
}

void Tr_init(void){
   if(fragList == NULL)
     fragList = F_FragList(F_ProcFrag(T_Exp(T_Const(0)),Tr_outermost()->frame),NULL);
}










   









