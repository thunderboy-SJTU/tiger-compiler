#include"translate.h"

F_fragList SEM_transProg(A_exp exp);


struct expty
{
   Tr_exp exp;
   Ty_ty ty;
};

typedef struct namelist* Namelist;

struct namelist
{
   S_symbol name;
   Namelist tail;
};


Namelist nameList(S_symbol name,Namelist tail);

struct expty expTy(Tr_exp, Ty_ty ty);

struct expty transVar(Tr_level level,S_table venv, S_table tenv, A_var v,Temp_label breakk);
struct expty transExp(Tr_level level,S_table venv, S_table tenv, A_exp a, Temp_label breakk);
Tr_exp transDec(Tr_level level,S_table venv, S_table tenv, A_dec d, Temp_label breakk);
Ty_ty transTy(S_table tenv, A_ty a);

Ty_tyList makeFormalTyList(S_table tenv,A_fieldList list);
Ty_fieldList makeFieldTyList(S_table tenv,A_fieldList list,A_pos pos);
U_boolList makeEscapeList(A_fieldList list);
int typeCmp(Ty_ty type1, Ty_ty type2);
Ty_ty actual_ty(Ty_ty ty);
