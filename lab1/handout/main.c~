/* This file is not complete.  You should fill it in with your
   solution to the programming exercise. */

/*id:5140379064 
 *name:陈俊
 */

#include <stdio.h>
#include "prog1.h"
#include "slp.h"
int maxargs(A_stm stm);   //maxargs,用于查看一个statement中最多有几个print的参数
int maxargs_exp(A_exp exp);  //maxargs_exp，用于查看一个exp中最多有几个print的参数
int maxargs_exps(A_expList exps);  //maxargs_exps,用于查看在一个expList中最多有几个print的参数，同时与自身explist中exp个数作比较（因为explist只在print后出现），选出最大的数字
int max(int i,int j);  //找出i和j中的最大值，用于比较
int count_exps(A_expList exps);  //count_exps，查找一个expList中有几个exp



typedef struct table *Table_;
struct table {string id; int value; Table_ tail;};  //用于保存参数及对应的值
Table_ Table(string id, int value, struct table *tail)  
{
  Table_ t = checked_malloc(sizeof(*t));
  t-> id = id;
  t-> value = value;
  t-> tail = tail;
  return t;
}

typedef struct intAndTable* IntAndTable;  //在exp中既会更新table也有返回的表达式int值，因此建立新的struct
struct intAndTable {int i; Table_ t;};
Table_ interpStm(A_stm stm, Table_ t);    //解析statement
IntAndTable interpExp(A_exp e, Table_ t);  //解析expression
Table_ update(Table_ t, string key, int value);   //更新table的数值
int lookup(Table_ t, string key);    //通过key查找table中的数值
void interp(A_stm stm);     //总的解析，打印对应的数字

/*
 *Please don't modify the main() function
 */
int main()
{
	int args;

	printf("prog\n");
	args = maxargs(prog());
	printf("args: %d\n",args);
	interp(prog());

	printf("prog_prog\n");
	args = maxargs(prog_prog());
	printf("args: %d\n",args);
	interp(prog_prog());

	printf("right_prog\n");
	args = maxargs(right_prog());
	printf("args: %d\n",args);
	interp(right_prog());

	return 0;
}


/*
 *以下为maxargs相关函数部分
 */



int max(int i, int j)    //max函数实现
{
   return i > j ? i :j;
}

int maxargs(A_stm stm)   //maxargs函数实现，通过递归调用获取statement中print语句最多的参数
{
   switch(stm->kind)
   {
      case A_compoundStm:
         return max(maxargs(stm->u.compound.stm1),maxargs(stm->u.compound.stm2));
      case A_assignStm:
         return maxargs_exp(stm->u.assign.exp);
      case A_printStm:
         return maxargs_exps(stm->u.print.exps);
   }
}

int maxargs_exp(A_exp exp)   //在exp中寻找最大的maxargs
{
   switch(exp->kind)
   {
     case A_opExp:
        return max(maxargs_exp(exp->u.op.left),maxargs_exp(exp->u.op.right));
     case A_eseqExp:
        return max(maxargs(exp->u.eseq.stm),maxargs_exp(exp->u.eseq.exp));
     default:
        return 0;
   }
}

int maxargs_exps(A_expList exps)   //在explist中在每个exp中查找最大的maxargs，同时与explist中exp的个数作比较（因为print（expList））
{
   int maxargs = 0;
   switch(exps->kind)
   {
      case A_pairExpList:
         maxargs =  max(maxargs_exp(exps->u.pair.head),maxargs_exps(exps->u.pair.tail));
         break;
      case A_lastExpList:
         maxargs =  maxargs_exp(exps->u.last) ;
   }
   return max(count_exps(exps),maxargs);
}

int count_exps(A_expList exps)   //查找在一个expList中有几个exp
{
   switch(exps->kind)
   {
      case A_pairExpList:
         return 1 + count_exps(exps->u.pair.tail);
      case A_lastExpList:
         return 1 ;
   }
}

/*
 *以下为interp相关函数部分
 */


Table_ interpStm(A_stm stm, Table_ t)    //解析statement，生成对应的table以及打印结果
{
   IntAndTable intTable = NULL;
   switch(stm->kind)
   {
      case A_compoundStm:   //复合语句递归调用
         t = interpStm(stm->u.compound.stm1, t);            
         return interpStm(stm->u.compound.stm2, t);
      case A_assignStm:     //赋值语句解析expression，同时更新表格，返回IntAndTable
         intTable = interpExp(stm->u.assign.exp, t);
         t = update(intTable->t, stm->u.assign.id, intTable->i);
         return t;
      case A_printStm:   //打印语句，对每个explist中的语句进行解析，同时打印相关数值
      {
         A_expList expList = stm->u.print.exps;
         while(expList->kind == A_pairExpList)
         {
               intTable = interpExp(expList->u.pair.head, t) ;
               t = intTable->t;
               printf("%d " , intTable->i) ;
               expList = expList->u.pair.tail ;
         }
         intTable = interpExp(expList->u.last, t) ;
         printf("%d \n" , intTable->i);
         return intTable->t;
      }  
   }
}

IntAndTable interpExp(A_exp exp,Table_ t)   //表达式的解析
{
    IntAndTable it = checked_malloc(sizeof(*it)) ;
    int v,v2 ;
    switch(exp->kind)
    {
       case A_idExp:  //字母表达式，通过查找获得数值
         it->i = lookup(t, exp->u.id) ;
         it->t = t ;
         break;
       case A_numExp:  //数字表达式，直接返回数字和table
         it->i = exp->u.num ;
         it->t = t ;
         break;
       case A_opExp:   //有运算的表达式，分别解析左右表达式，更新table，然后计算，返回运算后的数值和table
         it = interpExp(exp->u.op.left,t) ;
         v = it->i ;
         it = interpExp(exp->u.op.right,it->t) ;
         v2 = it->i;
         switch(exp->u.op.oper)
         {
           case A_plus:
             v += v2 ;
             break ;
           case A_minus:
             v -= v2 ;
             break ;
           case A_times:
             v *= v2 ;
             break ;
           case A_div:
             v /= v2 ;
             break ;
         }
         it->i = v ;
         break;
       case A_eseqExp:  //eseq表达式，递归调用
          t = interpStm(exp->u.eseq.stm,t) ;
          it = interpExp(exp->u.eseq.exp,t) ;
          break;
   }
   return it;   //返回IntAndTable
}

Table_ update(Table_ t, string key, int value)   //update函数，直接在前面插入列表
{
    Table_ new = checked_malloc(sizeof(*new));
    new-> id = key;
    new-> value = value;
    new-> tail = t;
    return new;
}

int lookup(Table_ t, string key)   //查找到第一个对应的key对应的数值，返回数值
{
   Table_ tmp = t;
   while(tmp != NULL)
   {
       if(tmp->id == key)
          return tmp-> value;
       else
          tmp = tmp -> tail; 
   }
   return -1;
}


void interp(A_stm stm)   //interp函数，对interpStm进行封装
{
  Table_ t = NULL;
  interpStm(stm,t);
}
