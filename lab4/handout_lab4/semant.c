#include <stdio.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "env.h"
#include "semant.h"

/*Lab4: Your implementation of lab4*/
void SEM_transProg(A_exp exp){
   S_table venv = E_base_venv();
   S_table tenv = E_base_tenv();
   transExp(venv,tenv,exp);
}

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

struct expty transVar(S_table venv, S_table tenv, A_var v)
{
   switch(v->kind){
     case A_simpleVar:{
       E_enventry x = S_look(venv,v->u.simple);
       if(x && x->kind == E_varEntry)
          return expTy(NULL,actual_ty(x->u.var.ty));
       else{
          EM_error(v->pos,"undefined variable %s",S_name(v->u.simple));
          return expTy(NULL,Ty_Int());
       }
     }
     case A_fieldVar:{
       struct expty var = transVar(venv,tenv,v->u.field.var);
       if(var.ty->kind == Ty_record)
       {
          Ty_fieldList record = var.ty->u.record;
          while(record != NULL)
          {
             S_symbol name = record->head->name;
             if(name == v->u.field.sym) 
                return expTy(NULL,actual_ty(record->head->ty));
             record = record->tail;
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
      struct expty var = transVar(venv,tenv,v->u.subscript.var);
      struct expty exp = transExp(venv,tenv,v->u.subscript.exp);
      if(var.ty->kind == Ty_array)
      {
          if(exp.ty->kind == Ty_int)
            return expTy(NULL,actual_ty(var.ty->u.array));
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
                  
struct expty transExp(S_table venv, S_table tenv, A_exp a)
{
   switch(a->kind){
     case A_varExp:        
       return transVar(venv,tenv,a->u.var);
     case A_nilExp:
       return expTy(NULL,Ty_Nil());
     case A_intExp:
       return expTy(NULL,Ty_Int());
     case A_stringExp:
       return expTy(NULL,Ty_String());
     case A_callExp:{ 
       E_enventry func = S_look(venv,a->u.call.func);
       A_expList args = a->u.call.args;
       if(func && func->kind == E_funEntry){
           Ty_tyList formals = func->u.fun.formals;
           Ty_ty result = func->u.fun.result;
           while((args != NULL) && (formals!=NULL))
           {
              struct expty arg = transExp(venv,tenv,args->head);
              Ty_ty type = formals-> head;
              if(!typeCmp(arg.ty,type))
              {
                EM_error(a->pos,"para type mismatch");
                return expTy(NULL,actual_ty(result));
              }
              args = args->tail;
              formals = formals->tail;          
           }
           if(args == NULL && formals == NULL)
              return expTy(NULL,actual_ty(result));
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
       struct expty left = transExp(venv,tenv,a->u.op.left);
       struct expty right = transExp(venv,tenv,a->u.op.right);
       if(oper == A_plusOp || oper == A_minusOp || oper == A_timesOp || oper == A_divideOp )
       {
         if(left.ty->kind != Ty_int)
           EM_error(a->u.op.left->pos,"integer required");
         if(right.ty->kind != Ty_int)
           EM_error(a->u.op.right->pos,"integer required");
         return expTy(NULL,Ty_Int());
       }
       if(oper == A_eqOp || oper == A_neqOp || oper == A_ltOp || oper == A_leOp || oper == A_gtOp || oper == A_geOp)
       {
          if(!typeCmp(left.ty,right.ty))
            EM_error(a->pos,"same type required");
          return expTy(NULL,Ty_Int());
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
       while((efieldlist != NULL) && (tylist != NULL))
       {
          A_efield efield = efieldlist->head;
          Ty_field tyfield = tylist->head;
          if(efield->name != tyfield->name)
          {
             EM_error(a->pos,"name not match");
             return expTy(NULL,type);
          }
          struct expty arg = transExp(venv,tenv,efield->exp);
          Ty_ty argType = tyfield->ty;
          if(!typeCmp(arg.ty,argType))
          {
             EM_error(a->pos,"arg type not match");
             return expTy(NULL,type);
          }
            efieldlist = efieldlist->tail;
            tylist = tylist->tail;          
       }
       if(efieldlist == NULL && tylist == NULL)
           return expTy(NULL,type);
       else
       {
           EM_error(a->pos,"para type mismatch");
           return expTy(NULL,type);
       } 
   }
   case A_seqExp:{
       A_expList seq = a->u.seq;
       struct expty type = expTy(NULL,Ty_Void());
       while(seq != NULL)
       {
          A_exp exp = seq->head;
          type = transExp(venv,tenv,exp);
          seq = seq->tail;
       }
       return type;
   }
   case A_assignExp:{
       A_var var = a->u.assign.var;
       A_exp exp = a->u.assign.exp;  
       struct expty vartype = transVar(venv,tenv,var);
       struct expty exptype = transExp(venv,tenv,exp);
       if(!typeCmp(vartype.ty,exptype.ty))
          EM_error(a->pos,"unmatched assign exp");
       return expTy(NULL,Ty_Void());
   }
   case A_ifExp:{
       A_exp iff = a->u.iff.test;  
       A_exp thenn = a->u.iff.then;
       A_exp elsee = a->u.iff.elsee;
       struct expty iffty = transExp(venv,tenv,iff);
       struct expty thenty = transExp(venv,tenv,thenn);
       if(iffty.ty->kind != Ty_int)
          EM_error(a->pos,"test must be int type");
       if(elsee == NULL && thenty.ty->kind != Ty_void)
          EM_error(a->pos,"if-then exp's body must produce no value");
       else if(elsee != NULL)
       {
          struct expty elseety = transExp(venv,tenv,elsee);
          if(!typeCmp(thenty.ty,elseety.ty))
              EM_error(a->pos,"then exp and else exp type mismatch");
       }      
       return thenty;
   }
   case A_whileExp:{
      A_exp test = a->u.whilee.test;      
      struct expty testty = transExp(venv,tenv,test);
      if(testty.ty->kind != Ty_int)
      {
         EM_error(a->pos,"test must be int type");
      }
      struct expty body = transExp(venv,tenv,a->u.whilee.body);
      if(body.ty->kind != Ty_void)
      {
         EM_error(a->pos,"while body must produce no value");
      }
      return body;
   }
   case A_forExp:{
      A_exp lo = a->u.forr.lo;
      A_exp hi = a->u.forr.hi;
      struct expty loty = transExp(venv,tenv,lo);
      struct expty hity = transExp(venv,tenv,hi);   
      if(loty.ty->kind != Ty_int || hity.ty->kind != Ty_int)
         EM_error(a->pos,"for exp's range type is not integer");
      S_beginScope(venv);
      transDec(venv,tenv,A_VarDec(a->pos, a->u.forr.var, S_Symbol("int"), lo));
      struct expty body = transExp(venv,tenv,a->u.forr.body);
      if(a->u.forr.body->kind == A_assignExp && a->u.forr.body->u.assign.var->kind == A_simpleVar && a->u.forr.body->u.assign.var->u.simple == a->u.forr.var)
         EM_error(a->pos,"loop variable can't be assigned");
      S_endScope(venv);
      return body;
   }
   case A_breakExp:
      return expTy(NULL,Ty_Void());
   case A_letExp:{
      struct expty exp;
      A_decList d;
      S_beginScope(venv);
      S_beginScope(tenv);
      for(d = a->u.let.decs;d;d = d->tail)
        transDec(venv,tenv,d->head);
      exp = transExp(venv,tenv,a->u.let.body);
      S_endScope(venv);
      S_endScope(tenv);
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
         struct expty sizety = transExp(venv,tenv,size);
         struct expty initty = transExp(venv,tenv,init);
         if(sizety.ty->kind != Ty_int)
           EM_error(a->pos,"size should be int");
         else if(!typeCmp(type->u.array,initty.ty))
           EM_error(a->pos,"type mismatch");
         else
           return expTy(NULL,type);
      }
      return expTy(NULL,Ty_Array(NULL));
   }             
 }
 assert(0);
}


void transDec(S_table venv, S_table tenv, A_dec d)
{  
   switch(d->kind){
      case A_functionDec:{
         A_fundecList list = d->u.function;
         while(list != NULL){
           A_fundec f = list->head;
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
           S_enter(venv,f->name,E_FunEntry(formalTys,resultTy));
           list = list->tail;
         }
         list = d->u.function;
         while(list != NULL){
            A_fundec f = list->head;
            Ty_tyList formalTys = makeFormalTyList(tenv,f->params);
            S_beginScope(venv);
            {
              A_fieldList l;
              Ty_tyList t;
              for(l = f->params, t = formalTys;l;l = l->tail, t= t->tail)
                 S_enter(venv,l->head->name,E_VarEntry(t->head));
            }      
            struct expty type = transExp(venv,tenv,f->body);
            E_enventry func = S_look(venv,f->name);
            Ty_ty returnType = func->u.fun.result;
            if(returnType->kind == Ty_void && type.ty->kind != Ty_void)
               EM_error(d->pos,"procedure returns value");
            else if(!typeCmp(returnType,type.ty))
               EM_error(d->pos,"type mismatch");
            S_endScope(venv);
            list = list->tail;
         }
         break;
      }
      case A_varDec:{
         struct expty e = transExp(venv,tenv,d->u.var.init);
         if(d->u.var.typ == NULL)
         {
            if(e.ty == NULL)
              EM_error(d->pos,"exp type not correct");
            else if(e.ty->kind == Ty_nil)
              EM_error(d->pos,"init should not be nil without type specified");
            S_enter(venv,d->u.var.var,E_VarEntry(e.ty));
         }
         else
         {
            Ty_ty type = actual_ty(S_look(tenv,d->u.var.typ));
            if(!typeCmp(e.ty,type))
              EM_error(d->pos,"type mismatch");
            S_enter(venv,d->u.var.var,E_VarEntry(e.ty));
         }
         break;         
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
                     return;
                   }
                   copy_list = copy_list->tail;
                }
                Namelist newlist = nameList(type->u.name.sym,nlist);
                nlist = newlist;
                copy_list = nlist;
                type = type->u.name.ty;
             }
      
         }
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

