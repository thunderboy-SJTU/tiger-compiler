#include "util.h"
#include "symbol.h" 
#include "absyn.h"  
#include <stdlib.h>
#include <stdio.h>
#include "table.h"
#include "escape.h"

Esc_entry Esc_Entry(int depth,bool* escape){
   Esc_entry escapeEntry = checked_malloc(sizeof(*escapeEntry));
   escapeEntry->depth = depth;
   escapeEntry->escape = escape;
   return escapeEntry;
}

static void traverseExp(S_table env,int depth,A_exp e);
static void traverseDec(S_table env,int depth,A_dec d);
static void traverseVar(S_table env,int depth,A_var v);

static void traverseExp(S_table env,int depth,A_exp e){
   switch(e->kind){
     case A_varExp:{
        traverseVar(env,depth,e->u.var);
        break;
     }
     case A_callExp:{
        A_expList args = e->u.call.args;
        for(;args != NULL;args = args->tail){
           traverseExp(env,depth,args->head);
        }
        break;
     }
     case A_opExp:{
        traverseExp(env,depth,e->u.op.left);
        traverseExp(env,depth,e->u.op.right);
        break;
     }
     case A_recordExp:{
        A_efieldList fields = e->u.record.fields;
        for(;fields != NULL;fields = fields->tail){
          traverseExp(env,depth,fields->head->exp);
        }
        break;
     }
     case A_seqExp:{
        A_expList seq = e->u.seq;
        for(;seq != NULL;seq = seq->tail){
           traverseExp(env,depth,seq->head);
        }
        break;
     }
     case A_assignExp:{
        traverseVar(env,depth,e->u.assign.var);
        traverseExp(env,depth,e->u.assign.exp);
        break;
     }
     case A_ifExp:{
        traverseExp(env,depth,e->u.iff.test);
        traverseExp(env,depth,e->u.iff.then);
        if(e->u.iff.elsee != NULL)
           traverseExp(env,depth,e->u.iff.elsee);
        break;
     }
     case A_whileExp:{
        traverseExp(env,depth,e->u.whilee.test);
        traverseExp(env,depth,e->u.whilee.body);
        break;
     }
     case A_forExp:{
        traverseExp(env,depth,e->u.forr.lo);
        traverseExp(env,depth,e->u.forr.hi);
        S_beginScope(env);
        S_enter(env,e->u.forr.var,Esc_Entry(depth,&(e->u.forr.escape)));
        e->u.forr.escape = FALSE;
        traverseExp(env,depth,e->u.forr.body);
        S_endScope(env);
        break;
     }
     case A_letExp:{
        A_decList decs = e->u.let.decs;
        S_beginScope(env);
        for(;decs != NULL;decs = decs->tail){
           traverseDec(env,depth,decs->head);
        }
        traverseExp(env,depth,e->u.let.body);
        S_endScope(env);
        break;
     }
     case A_arrayExp:{
        traverseExp(env,depth,e->u.array.size);
        traverseExp(env,depth,e->u.array.init);
        break;
     }
     default:
        break;
  }
}
        
static void traverseDec(S_table env,int depth,A_dec d){
  switch(d->kind){
     case A_functionDec:{
        A_fundecList list = d->u.function;
        for(;list != NULL;list = list->tail){
           S_beginScope(env);
           traverseExp(env,depth+1,list->head->body);
           S_endScope(env);
        }
        break;
     }
     case A_varDec:{
        traverseExp(env,depth,d->u.var.init);
        S_enter(env,d->u.var.var,Esc_Entry(depth,&(d->u.var.escape)));
        d->u.var.escape = FALSE;
        break;
     }
     case A_typeDec:{
       break;
     }
  }
}
     
     
static void traverseVar(S_table env,int depth,A_var v){
  switch(v->kind){
     case A_simpleVar:{
         Esc_entry entry = S_look(env,v->u.simple);
         if(entry != NULL){
         int decDepth = entry->depth;
         if(depth > decDepth)
             (*(entry->escape)) = TRUE;
         }
         break;
     }
     case A_fieldVar:{
         traverseVar(env,depth,v->u.field.var);
         break;
     }
     case A_subscriptVar:{
         traverseVar(env,depth,v->u.subscript.var);
         break;
     }
   }
}

void Esc_findEscape(A_exp exp) {
   S_table escEnv = S_empty();
   traverseExp(escEnv,0,exp);
}
   

