#include <stdio.h>
#include "util.h"
#include "table.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"

#include "env.h"

/*Lab4: Your implementation of lab4*/
E_enventry E_VarEntry(Tr_access access,Ty_ty ty)
{
  E_enventry entry = checked_malloc(sizeof(*entry));
  entry->kind = E_varEntry;
  entry->u.var.access = access;
  entry->u.var.ty = ty;
  return entry;
}

E_enventry E_FunEntry(Tr_level level, Temp_label label,Ty_tyList formals, Ty_ty result)
{
  E_enventry entry = checked_malloc(sizeof(*entry));
  entry->kind = E_funEntry;
  entry->u.fun.level = level;
  entry->u.fun.label = label;
  entry->u.fun.formals = formals;
  entry->u.fun.result = result;
  return entry;
}

S_table E_base_tenv(void)
{
   S_table tenv = S_empty();
   S_enter(tenv,S_Symbol("int"),Ty_Int());
   S_enter(tenv,S_Symbol("string"),Ty_String());
   return tenv;
}

S_table E_base_venv(void)
{
   S_table venv = S_empty();
   S_enter(venv,S_Symbol("print"),E_FunEntry(Tr_outermost(),Temp_newlabel(),Ty_TyList(Ty_String(),NULL),Ty_Void()));
   S_enter(venv,S_Symbol("flush"),E_FunEntry(Tr_outermost(),Temp_newlabel(),NULL,Ty_Void()));
   S_enter(venv,S_Symbol("getchar"),E_FunEntry(Tr_outermost(),Temp_newlabel(),NULL,Ty_String()));
   S_enter(venv,S_Symbol("ord"),E_FunEntry(Tr_outermost(),Temp_newlabel(),Ty_TyList(Ty_String(),NULL),Ty_Int()));
   S_enter(venv,S_Symbol("chr"),E_FunEntry(Tr_outermost(),Temp_newlabel(),Ty_TyList(Ty_Int(),NULL),Ty_String()));
   S_enter(venv,S_Symbol("size"),E_FunEntry(Tr_outermost(),Temp_newlabel(),Ty_TyList(Ty_String(),NULL),Ty_Int()));
   S_enter(venv,S_Symbol("substring"),E_FunEntry(Tr_outermost(),Temp_newlabel(),Ty_TyList(Ty_String(),Ty_TyList(Ty_Int(),Ty_TyList(Ty_Int(),NULL))),Ty_String()));
   S_enter(venv,S_Symbol("concat"),E_FunEntry(Tr_outermost(),Temp_newlabel(),Ty_TyList(Ty_String(),Ty_TyList(Ty_String(),NULL)),Ty_String()));
   S_enter(venv,S_Symbol("not"),E_FunEntry(Tr_outermost(),Temp_newlabel(),Ty_TyList(Ty_Int(),NULL),Ty_Int()));
   S_enter(venv,S_Symbol("exit"),E_FunEntry(Tr_outermost(),Temp_newlabel(),Ty_TyList(Ty_Int(),NULL),Ty_Void()));
   return venv;
}
