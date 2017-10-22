#ifndef TRANSLATE_H
#define TRANSLATE_H

/* Lab5: your code below */

typedef struct Tr_exp_ *Tr_exp;

typedef struct Tr_expList_ *Tr_expList;

typedef struct Tr_access_ *Tr_access;

typedef struct Tr_accessList_ *Tr_accessList;

typedef struct Tr_level_ *Tr_level;

typedef struct patchList_ *patchList;

struct Cx{                   //这些类的定义放在.c里面无法编译通过，显示dereferencing pointer to incomplete type，故迁移到.h
        patchList trues;
        patchList falses;
        T_stm stm;
};

struct Tr_access_ {
	//Lab5: your code here
        Tr_level level;
        F_access access;
};


struct Tr_accessList_ {
	Tr_access head;
	Tr_accessList tail;	
};



struct Tr_level_ {
	//Lab5: your code here
        Tr_level parent;
        F_frame frame;
        Temp_label name;
        Tr_accessList formals;
};

struct Tr_exp_ {
	//Lab5: your code here
        enum{Tr_ex,Tr_nx,Tr_cx} kind;
        union{
           T_exp ex;
           T_stm nx;
           struct Cx cx;
        }u;  
};

struct Tr_expList_ {

    Tr_exp head;
    Tr_expList tail;
};

struct patchList_{
        Temp_label *head;
        patchList tail;
};

static patchList PatchList(Temp_label *head, patchList tail);

Tr_accessList Tr_AccessList(Tr_access head,Tr_accessList tail);

Tr_expList Tr_ExpList(Tr_exp head,Tr_expList tail);

T_expList toT_ExpList(Tr_expList list);
T_stmList toT_StmList(Tr_expList list);

Tr_level Tr_outermost(void);
Tr_level Tr_newLevel(Tr_level parent,Temp_label name,U_boolList formals);

Tr_accessList Tr_formals(Tr_level level);
Tr_access Tr_allocLocal(Tr_level level,bool escape);

void Tr_procEntryExit(Tr_level level, Tr_exp body, Tr_accessList formals);
F_fragList Tr_getResult(void);
void Tr_addOutermost(Tr_exp exp);

void doPatch(patchList tList,Temp_label label);
patchList joinPatch(patchList first, patchList second);

static patchList PatchList(Temp_label *head, patchList tail);
static Tr_exp Tr_Ex(T_exp ex);
static Tr_exp Tr_Nx(T_stm nx);
static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm);
static T_exp unEx(Tr_exp e);
static T_stm unNx(Tr_exp e);
static struct Cx unCx(Tr_exp e);

T_exp getLink(Tr_level dec_level,Tr_level use_level);
T_exp getFunLink(Tr_level dec_level,Tr_level use_level);

Tr_exp Tr_simpleVar(Tr_access acc ,Tr_level level);
Tr_exp Tr_fieldVar(Tr_exp record, int index);
Tr_exp Tr_subscriptVar(Tr_exp array, Tr_exp index);
Tr_exp Tr_nilExp();
Tr_exp Tr_intExp(int number);
Tr_exp Tr_stringExp(string str);
Tr_exp Tr_callExp(Temp_label name, Tr_level dec, Tr_level use,Tr_expList args);
Tr_exp Tr_opExp(A_oper oper, Tr_exp left, Tr_exp right);
Tr_exp Tr_recordExp(int count, Tr_expList paras);
Tr_exp Tr_seqExp(Tr_expList stm,Tr_exp exp);
Tr_exp Tr_assignExp(Tr_exp left, Tr_exp right);
Tr_exp Tr_ifExp(Tr_exp ifexp, Tr_exp thenexp, Tr_exp elseexp);
Tr_exp Tr_whileExp(Tr_exp test,Tr_exp body, Temp_label done);
Tr_exp Tr_forExp(Tr_exp var, Tr_exp lo, Tr_exp hi, Tr_exp body, Temp_label done);
Tr_exp Tr_breakExp(Temp_label done);
Tr_exp Tr_letExp(Tr_expList dec,Tr_exp exp);
Tr_exp Tr_arrayExp(Tr_exp size,Tr_exp value);
Tr_exp Tr_varDec(Tr_access acc, Tr_exp value);
Tr_exp Tr_funDec(Tr_level level, Tr_exp body);
Tr_exp Tr_typeDec();

#endif
