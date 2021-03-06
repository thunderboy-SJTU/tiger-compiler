#include <stdio.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "env.h"
#include "temp.h"
#include "tree.h"
#include "frame.h"
#include "semant.h"
#include "escape.h"


/*Lab4: Your implementation of lab4*/
F_fragList SEM_transProg(A_exp exp){
   S_table venv = E_base_venv();
   S_table tenv = E_base_tenv();
   Esc_findEscape(exp);
   struct expty e = transExp(Tr_outermost(),venv,tenv,exp,NULL);
   Tr_addOutermost(e.exp);
   return Tr_getResult();
}

/*void SEM_transProg(A_exp exp){
  S_table venv = E_base_venv();
  S_table tenv = E_base_tenv();
  transExp(venv,tenv,exp);
}*/

struct expty expTy(Tr_exp exp, Ty_ty ty)
{
   struct expty e;
   e.exp = exp;
   e.ty = ty;
   return e;
}

Namelist nameList(S_symbol name,Namelist tail)
{
   Namelist list = checked_malloc(sizeof(*list));
   list->name = name;
   list->tail = tail;
   return list;
}

struct expty transVar(Tr_level level,S_table venv, S_table tenv, A_var v,Temp_label breakk)
{
   switch(v->kind){
     case A_simpleVar:{
       E_enventry x = S_look(venv,v->u.simple);
       if(x && x->kind == E_varEntry)
          return expTy(Tr_simpleVar(x->u.var.access,level),actual_ty(x->u.var.ty));
       else{
          EM_error(v->pos,"undefined variable %s",S_name(v->u.simple));
          return expTy(NULL,Ty_Int());
       }
     }
     case A_fieldVar:{
       struct expty var = transVar(level,venv,tenv,v->u.field.var,breakk);
       if(var.ty->kind == Ty_record)
       {
          Ty_fieldList record = var.ty->u.record;
          int i = 0;
          while(record != NULL)
          {
             S_symbol name = record->head->name;
             if(name == v->u.field.sym) 
                return expTy(Tr_fieldVar(var.exp,i),actual_ty(record->head->ty));
             record = record->tail;
             i++;
          }
          EM_error(v->pos,"field %s doesn't exist",S_name(v->u.field.sym));
          return expTy(NULL,Ty_Nil());
      }
      else{
          EM_error(v->pos,"not a record type");
          return expTy(NULL,Ty_Nil());
      }
    }
    case A_subscriptVar:{
      struct expty var = transVar(level,venv,tenv,v->u.subscript.var,breakk);
      struct expty exp = transExp(level,venv,tenv,v->u.subscript.exp,breakk);
      if(var.ty->kind == Ty_array)
      {
          if(exp.ty->kind == Ty_int)
            return expTy(Tr_subscriptVar(var.exp, exp.exp),actual_ty(var.ty->u.array));
          else{
            EM_error(v->pos,"incorrect subscript");
            return expTy(NULL,Ty_Array(NULL));
          }
      }
      else{
        EM_error(v->pos,"array type required");
        return expTy(NULL,Ty_Array(NULL));
      }
    }
  }
  assert(0);
}
                  
struct expty transExp(Tr_level level,S_table venv, S_table tenv, A_exp a, Temp_label breakk)
{
   switch(a->kind){
     case A_varExp:        
       return transVar(level,venv,tenv,a->u.var,breakk);
     case A_nilExp:
       return expTy(Tr_nilExp(),Ty_Nil());
     case A_intExp:
       return expTy(Tr_intExp(a->u.intt),Ty_Int());
     case A_stringExp:
       return expTy(Tr_stringExp(a->u.stringg),Ty_String());
     case A_callExp:{ 
       E_enventry func = S_look(venv,a->u.call.func);
       A_expList args = a->u.call.args;
       if(func && func->kind == E_funEntry){
           Ty_tyList formals = func->u.fun.formals;
           Ty_ty result = func->u.fun.result;
           Tr_expList list = checked_malloc(sizeof(*list));
           list->head = NULL;
           list->tail = NULL;
           Tr_expList tmp = list;
           while((args != NULL) && (formals!=NULL))
           {
              struct expty arg = transExp(level,venv,tenv,args->head,breakk);
              Tr_exp argexp = arg.exp;
              Ty_ty type = formals-> head;
              if(!typeCmp(arg.ty,type))
              {
                EM_error(a->pos,"para type mismatch");
                return expTy(NULL,actual_ty(result));
              }
              if(tmp->head == NULL)
                 list->head = argexp;
              else
              {
                 Tr_expList listtail = checked_malloc(sizeof(*listtail));
                 listtail->head = argexp;
                 listtail->tail = NULL;
                 tmp->tail = listtail;
                 tmp = tmp->tail;         
              }
              args = args->tail;
              formals = formals->tail;          
           }
           if(args == NULL && formals == NULL)
           {
              Temp_label label = func->u.fun.label;
              Tr_level declevel = func->u.fun.level;
              if(list->head == NULL)
                   list = NULL;
              return expTy(Tr_callExp(label, declevel, level,list),actual_ty(result));
           }
           else if(args != NULL && formals == NULL)
           {
              EM_error(a->pos,"too many params in function %s",S_name(a->u.call.func));
              return expTy(NULL,actual_ty(result));
           }
           else if(args == NULL && formals != NULL)
           {
             EM_error(a->pos,"few params in function %s",S_name(a->u.call.func));
             return expTy(NULL,actual_ty(result));
           }
       }
       else 
       {
          EM_error(a->pos,"undefined function %s",S_name(a->u.call.func));
          return expTy(NULL,Ty_Void());
       }
    }
    case A_opExp:{
       A_oper oper = a->u.op.oper;
       struct expty left = transExp(level,venv,tenv,a->u.op.left,breakk);
       struct expty right = transExp(level,venv,tenv,a->u.op.right,breakk);
       if(oper == A_plusOp || oper == A_minusOp || oper == A_timesOp || oper == A_divideOp )
       {
         if(left.ty->kind != Ty_int)
           EM_error(a->u.op.left->pos,"integer required");
         if(right.ty->kind != Ty_int)
           EM_error(a->u.op.right->pos,"integer required");
         return expTy(Tr_opExp(oper, left.exp, right.exp),Ty_Int());
       }
       if(oper == A_eqOp || oper == A_neqOp || oper == A_ltOp || oper == A_leOp || oper == A_gtOp || oper == A_geOp)
       {
          if(!typeCmp(left.ty,right.ty))
            EM_error(a->pos,"same type required");
          return expTy(Tr_opExp(oper, left.exp, right.exp),Ty_Int());
       }
      EM_error(a->pos,"unknown oper");
      return expTy(NULL,Ty_Int());
    }
    case A_recordExp:{
       Ty_ty type = actual_ty(S_look(tenv,a->u.record.typ));
       if(type == NULL)
       {
          EM_error(a->pos,"undefined type %s",S_name(a->u.record.typ));
          return expTy(NULL,Ty_Record(NULL));
       }
       if(type->kind != Ty_record)
       {
         EM_error(a->pos,"wrong record type");
         return expTy(NULL,Ty_Record(NULL));
       }
       A_efieldList efieldlist = a->u.record.fields;
       Ty_fieldList tylist = type->u.record;
       Tr_expList list = checked_malloc(sizeof(*list));
       list->head = NULL;
       list->tail = NULL;
       Tr_expList tmp = list;
       int cnt = 0;
       while((efieldlist != NULL) && (tylist != NULL))
       {
          A_efield efield = efieldlist->head;
          Ty_field tyfield = tylist->head;
          if(efield->name != tyfield->name)
          {
             EM_error(a->pos,"name not match");
             return expTy(NULL,type);
          }
          struct expty arg = transExp(level,venv,tenv,efield->exp,breakk);
          Tr_exp argexp = arg.exp;
          Ty_ty argType = tyfield->ty;
          if(!typeCmp(arg.ty,argType))
          {
             EM_error(a->pos,"arg type not match");
             return expTy(NULL,type);
          }
          if(tmp->head == NULL)
             list->head = argexp;
          else
          {
             Tr_expList listtail = checked_malloc(sizeof(*listtail));
             listtail->head = argexp;
             listtail->tail = NULL;
             tmp->tail = listtail;
             tmp = tmp->tail;         
          }
          efieldlist = efieldlist->tail;
          tylist = tylist->tail;
          cnt++;          
       }
       if(efieldlist == NULL && tylist == NULL)
       {
            if(list->head == NULL)
                 list = NULL;
           return expTy(Tr_recordExp(cnt, list),type);
       }
       else
       {
           EM_error(a->pos,"para type mismatch");
           return expTy(NULL,type);
       } 
   }
   case A_seqExp:{
       A_expList seq = a->u.seq;
       struct expty type = expTy(NULL,Ty_Void());
       Tr_expList list = checked_malloc(sizeof(*list));
       list->head = NULL;
       list->tail = NULL;
       Tr_expList tmp = list;
       Tr_exp tr_exp = NULL;
       while(seq != NULL)
       {
          A_exp exp = seq->head;
          type = transExp(level,venv,tenv,exp,breakk);
          Tr_exp argexp = type.exp;
          if(seq->tail == NULL)
          { 
             tr_exp = argexp;
             break;
          }
          if(tmp->head == NULL)
             list->head = argexp;
          else
          {
             Tr_expList listtail = checked_malloc(sizeof(*listtail));
             listtail->head = argexp;
             listtail->tail = NULL;
             tmp->tail = listtail;
             tmp = tmp->tail;         
          }
          seq = seq->tail;
       }
       if(list->head == NULL)
          list = NULL;
       type.exp = Tr_seqExp(list,tr_exp);
       return type;
   }
   case A_assignExp:{
       A_var var = a->u.assign.var;
       A_exp exp = a->u.assign.exp;  
       struct expty vartype = transVar(level,venv,tenv,var,breakk);
       struct expty exptype = transExp(level,venv,tenv,exp,breakk);
       if(!typeCmp(vartype.ty,exptype.ty))
          EM_error(a->pos,"unmatched assign exp");
       return expTy(Tr_assignExp(vartype.exp, exptype.exp),Ty_Void());
   }
   case A_ifExp:{
       A_exp iff = a->u.iff.test;  
       A_exp thenn = a->u.iff.then;
       A_exp elsee = a->u.iff.elsee;
       struct expty iffty = transExp(level,venv,tenv,iff,breakk);
       struct expty thenty = transExp(level,venv,tenv,thenn,breakk);
       struct expty elseety;
       if(iffty.ty->kind != Ty_int)
          EM_error(a->pos,"test must be int type");
       if(elsee == NULL && thenty.ty->kind != Ty_void)
          EM_error(a->pos,"if-then exp's body must produce no value");
       else if(elsee != NULL)
       {
          elseety = transExp(level,venv,tenv,elsee,breakk);
          if(!typeCmp(thenty.ty,elseety.ty))
              EM_error(a->pos,"then exp and else exp type mismatch");
       } 
       Tr_exp ifexp = iffty.exp;
       Tr_exp thenexp = thenty.exp;
       Tr_exp elseexp;
       if(elsee == NULL)
            elseexp = NULL;
       else 
            elseexp = elseety.exp;
       thenty.exp = Tr_ifExp(ifexp,thenexp,elseexp);   
       return thenty;
   }
   case A_whileExp:{
      A_exp test = a->u.whilee.test;      
      struct expty testty = transExp(level,venv,tenv,test,breakk);
      Temp_label done = Temp_newlabel();
      if(testty.ty->kind != Ty_int)
      {
         EM_error(a->pos,"test must be int type");
      }
      struct expty body = transExp(level,venv,tenv,a->u.whilee.body,done);
      if(body.ty->kind != Ty_void)
      {
         EM_error(a->pos,"while body must produce no value");
      }
      body.exp = Tr_whileExp(testty.exp,body.exp,done);
      return body;
   }
   case A_forExp:{
      A_exp lo = a->u.forr.lo;
      A_exp hi = a->u.forr.hi;
      Temp_label done = Temp_newlabel();
      struct expty loty = transExp(level,venv,tenv,lo,breakk);
      struct expty hity = transExp(level,venv,tenv,hi,breakk);   
      if(loty.ty->kind != Ty_int || hity.ty->kind != Ty_int)
         EM_error(a->pos,"for exp's range type is not integer");
      S_beginScope(venv);
      A_dec forDec = A_VarDec(a->pos, a->u.forr.var, S_Symbol("int"), lo);

      forDec->u.var.escape = a->u.forr.escape;
      transDec(level,venv,tenv,forDec,breakk);
      //transDec(level,venv,tenv,A_VarDec(a->pos, a->u.forr.var, S_Symbol("int"), lo),breakk);
      E_enventry entry = S_look(venv,a->u.forr.var);
      struct expty body = transExp(level,venv,tenv,a->u.forr.body,done);
      if(a->u.forr.body->kind == A_assignExp && a->u.forr.body->u.assign.var->kind == A_simpleVar && a->u.forr.body->u.assign.var->u.simple == a->u.forr.var)
         EM_error(a->pos,"loop variable can't be assigned");
      Tr_access acc = entry->u.var.access;
      Tr_exp var = Tr_simpleVar(acc,level);
      body.exp = Tr_forExp(var,loty.exp,hity.exp, body.exp,done);
      S_endScope(venv);
      return body;
   }
   case A_breakExp:{
    if(breakk == NULL)
    {
      EM_error(a->pos,"break should in while or for exp");
      return expTy(NULL,Ty_Void());
    }
    else
      return expTy(Tr_breakExp(breakk),Ty_Void());
   }
   case A_letExp:{
      struct expty exp;
      A_decList d;
      S_beginScope(venv);
      S_beginScope(tenv);
      Tr_expList list = checked_malloc(sizeof(*list));
      list->head = NULL;
      list->tail = NULL;
      Tr_expList tmp = list;
      for(d = a->u.let.decs;d;d = d->tail)
      {
         Tr_exp dec = transDec(level,venv,tenv,d->head,breakk);
         if(tmp->head == NULL)
              list->head = dec;
         else
         {
             Tr_expList listtail = checked_malloc(sizeof(*listtail));
             listtail->head = dec;
             listtail->tail = NULL;
             tmp->tail = listtail;
             tmp = tmp->tail;         
         }
      }
      exp = transExp(level,venv,tenv,a->u.let.body,breakk);
      S_endScope(venv);
      S_endScope(tenv);
      if(list->head == NULL)
             list = NULL;
      exp.exp = Tr_letExp(list,exp.exp);
      return exp;
   }
   case A_arrayExp:{
      Ty_ty type = actual_ty(S_look(tenv,a->u.array.typ));
      if(type == NULL)
         EM_error(a->pos,"no such array type");
      else if(type->kind != Ty_array)
         EM_error(a->pos,"not array type");
      else
      {
         A_exp size = a->u.array.size;
         A_exp init = a->u.array.init;
         struct expty sizety = transExp(level,venv,tenv,size,breakk);
         struct expty initty = transExp(level,venv,tenv,init,breakk);
         if(sizety.ty->kind != Ty_int)
           EM_error(a->pos,"size should be int");
         else if(!typeCmp(type->u.array,initty.ty))
           EM_error(a->pos,"type mismatch");
         else
           return expTy(Tr_arrayExp(sizety.exp,initty.exp),type);
      }
      return expTy(NULL,Ty_Array(NULL));
   }             
 }
 assert(0);
}


Tr_exp transDec(Tr_level level, S_table venv, S_table tenv, A_dec d, Temp_label breakk)
{  
   switch(d->kind){
      case A_functionDec:{
         A_fundecList list = d->u.function;
         Tr_exp exp;
         while(list != NULL){
           A_fundec f = list->head;
           U_boolList escape = makeEscapeList(f->params);
           Temp_label name = f->name;
           Tr_level newLevel = Tr_newLevel(level,name,escape);
           Ty_ty resultTy = Ty_Void();
           if(f->result != NULL)                 
              resultTy = S_look(tenv,f->result);
           if(resultTy == NULL)
              EM_error(d->pos,"no return type");
           A_fundecList copy_list = list->tail;
             while(copy_list != NULL)
             {
                S_symbol sym = copy_list->head->name;
                if(f->name == sym)
                   EM_error(d->pos,"two functions have the same name");
                copy_list = copy_list->tail;
             }
           Ty_tyList formalTys = makeFormalTyList(tenv,f->params);
           //S_enter(venv,f->name,E_FunEntry(newLevel,Temp_newlabel(),formalTys,resultTy));
           S_enter(venv,f->name,E_FunEntry(newLevel,name,formalTys,resultTy));
           list = list->tail;
         }
         list = d->u.function;
         while(list != NULL){
            A_fundec f = list->head;
            Ty_tyList formalTys = makeFormalTyList(tenv,f->params);
            E_enventry func = S_look(venv,f->name);
            Tr_level newLevel = func->u.fun.level;
            Tr_accessList accList = func->u.fun.level->formals;
            S_beginScope(venv);
            {
              A_fieldList l;
              Ty_tyList t;
              for(l = f->params, t = formalTys;l;l = l->tail, t= t->tail,accList = accList->tail)
                 S_enter(venv,l->head->name,E_VarEntry(accList->head,t->head));
            }      
            struct expty type = transExp(newLevel,venv,tenv,f->body,breakk);
            Ty_ty returnType = func->u.fun.result;
            if(returnType->kind == Ty_void && type.ty->kind != Ty_void)
               EM_error(d->pos,"procedure returns value");
            else if(!typeCmp(returnType,type.ty))
               EM_error(d->pos,"type mismatch");
            exp = Tr_funDec(newLevel,type.exp);
            S_endScope(venv);
            list = list->tail;
         }
         return exp;
      }
      case A_varDec:{
         struct expty e = transExp(level,venv,tenv,d->u.var.init,breakk);
         Tr_access access = NULL;
         if(d->u.var.typ == NULL)
         {
            if(e.ty == NULL)
              EM_error(d->pos,"exp type not correct");
            else if(e.ty->kind == Ty_nil)
              EM_error(d->pos,"init should not be nil without type specified");
            //access = Tr_allocLocal(level,TRUE);
            access = Tr_allocLocal(level,d->u.var.escape);
            S_enter(venv,d->u.var.var,E_VarEntry(access,e.ty));
         }
         else
         {
            Ty_ty type = actual_ty(S_look(tenv,d->u.var.typ));
            if(!typeCmp(e.ty,type))
              EM_error(d->pos,"type mismatch");
            access = Tr_allocLocal(level,d->u.var.escape);
            S_enter(venv,d->u.var.var,E_VarEntry(access,e.ty));            
         }
         //return Tr_simpleVar(access,level);    
         Tr_exp var = Tr_simpleVar(access,level);
         return Tr_assignExp(var, e.exp);   
      }
      case A_typeDec:{
         A_nametyList list;
         for (list = d->u.type;list;list = list->tail){
             S_symbol sym = list->head->name;
             A_nametyList copy_list = list->tail;
             while(copy_list != NULL)
             {
                S_symbol sym2 = copy_list->head->name;
                if(sym == sym2)
                   EM_error(d->pos,"two types have the same name");
                copy_list = copy_list->tail;
             }
	     S_enter(tenv, sym, Ty_Name(sym,NULL));
         } 
         for (list = d->u.type; list;list = list->tail){
	     Ty_ty type = S_look(tenv,list->head->name);
             type->u.name.ty = transTy(tenv,list->head->ty);
         }
         for(list = d->u.type;list;list = list->tail){
             Namelist nlist = NULL;
             Namelist copy_list = NULL;
             Ty_ty type = S_look(tenv,list->head->name);
             if(type->kind != Ty_name)
                continue;
             while(type->kind == Ty_name)
             {               
                while(copy_list != NULL)
                {
                   S_symbol name = copy_list->name;
                   if(name == type->u.name.sym)
                   {
                     EM_error(d->pos,"illegal type cycle");
                     return Tr_typeDec();
                   }
                   copy_list = copy_list->tail;
                }
                Namelist newlist = nameList(type->u.name.sym,nlist);
                nlist = newlist;
                copy_list = nlist;
                type = type->u.name.ty;
             }
      
         }
         return Tr_typeDec();
       }
     }
}

Ty_ty transTy(S_table tenv, A_ty a)
{
   Ty_ty ty = NULL;
   switch(a->kind){
      case A_nameTy:{
        ty = S_look(tenv,a->u.name);
        break;
      }
      case A_recordTy:{
        ty = checked_malloc(sizeof(*ty));
        ty->kind = Ty_record;
        ty->u.record = makeFieldTyList(tenv,a->u.record,a->pos);
        break;
      }
      case A_arrayTy:{
        ty = checked_malloc(sizeof(*ty));
        ty->kind = Ty_array;
        ty->u.array = S_look(tenv,a->u.array);
        break;
      }
  }
  return ty;
}
      
Ty_tyList makeFormalTyList(S_table tenv,A_fieldList list){
  Ty_tyList begin = NULL;
  Ty_tyList end = NULL;
  if(list != NULL)
  {
     A_field field = list->head;
     Ty_ty type = S_look(tenv,field->typ);
     end = Ty_TyList(type,NULL);
     begin = end;
     list = list->tail;
  }
  while(list != NULL){
     A_field field = list->head;
     Ty_ty type = S_look(tenv,field->typ);
     end->tail = Ty_TyList(type,NULL);
     end = end->tail;
     list = list->tail;
  }   
  return begin;
}
 
Ty_fieldList makeFieldTyList(S_table tenv,A_fieldList list,A_pos pos){
  Ty_fieldList begin = NULL;
  Ty_fieldList end = NULL;
  if(list != NULL)
  {
     A_field field = list->head;
     Ty_ty type = S_look(tenv,field->typ);
     if(type == NULL)
       EM_error(pos,"undefined type %s",S_name(field->typ));
     end = Ty_FieldList(Ty_Field(field->name,type),NULL);
     begin = end;
     list = list->tail;
  }
  while(list != NULL){
     A_field field = list->head;
     Ty_ty type = S_look(tenv,field->typ);
     if(type == NULL)
       EM_error(pos,"undefined type %s",S_name(field->typ));
     end->tail = Ty_FieldList(Ty_Field(field->name,type),NULL);
     end = end->tail;
     list = list->tail;
  }
  return begin;
}

U_boolList makeEscapeList(A_fieldList list)
{
   U_boolList boollist = NULL;
   for( ;list!= NULL;list = list->tail)
       boollist = U_BoolList(TRUE,boollist);
   return boollist;
}


Ty_ty actual_ty(Ty_ty ty){
  Ty_ty type = ty;
  while(type != NULL){
    if(type->kind != Ty_name)
       break;
    type = type->u.name.ty;
  }
  return type;
}
	           
int typeCmp(Ty_ty type1, Ty_ty type2)
{
   if(type1 == NULL || type2 == NULL)
      return 0;
   Ty_ty ty1 = actual_ty(type1);
   Ty_ty ty2 = actual_ty(type2);
   if(ty1 == ty2)
      return 1;
   if(ty1->kind == Ty_nil && ty2->kind == Ty_nil)
      return 0;
   if(ty1->kind == Ty_nil || ty2->kind == Ty_nil)
      return 1;
   return 0;
}

