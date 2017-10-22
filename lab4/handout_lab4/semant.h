void SEM_transProg(A_exp exp);

typedef void * Tr_exp;

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

struct expty transVar(S_table venv, S_table tenv, A_var v);
struct expty transExp(S_table venv, S_table tenv, A_exp a);
void transDec(S_table venv, S_table tenv, A_dec d);
Ty_ty transTy(S_table tenv, A_ty a);

Ty_tyList makeFormalTyList(S_table tenv,A_fieldList list);
Ty_fieldList makeFieldTyList(S_table tenv,A_fieldList list,A_pos pos);
int typeCmp(Ty_ty type1, Ty_ty type2);
Ty_ty actual_ty(Ty_ty ty);
