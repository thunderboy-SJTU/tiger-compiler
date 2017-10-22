#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "flowgraph.h"
#include "errormsg.h"
#include "table.h"


/* T_table functions */

T_table T_empty(void) {
  return TAB_empty();
}

void T_enter(T_table t, Temp_label label, void *node)
{
  TAB_enter(t, label, node);
}

void *T_look(T_table t, Temp_label label)
{
  return TAB_look(t, label);
}


Temp_tempList FG_def(G_node n) {
	//your code here.
        AS_instr instr = G_nodeInfo(n);
        switch(instr->kind){
           case I_OPER:
               return instr->u.OPER.dst;
           case I_LABEL:
               return NULL;
           case I_MOVE:
               return instr->u.MOVE.dst;
	}
        assert(0);
}

Temp_tempList FG_use(G_node n) {
	//your code here.
        AS_instr instr = G_nodeInfo(n);
        switch(instr->kind){
           case I_OPER:
               return instr->u.OPER.src;
           case I_LABEL:
               return NULL;
           case I_MOVE:
               return instr->u.MOVE.src;
	}
        assert(0);
}

bool FG_isMove(G_node n) {
	//your code here.
        AS_instr instr = G_nodeInfo(n);
        if(instr->kind == I_MOVE)
	   return TRUE;
        return FALSE;
}

G_graph FG_AssemFlowGraph(AS_instrList il, F_frame f) {
	//your code here.
        G_graph graph = G_Graph();
        G_node pre = NULL;
        T_table table = T_empty();
        G_nodeList list = NULL;
        for(;il!= NULL;il = il->tail){
           AS_instr instr = il->head;
           G_node node = G_Node(graph,instr);
           if(pre != NULL){   
               if(((AS_instr)G_nodeInfo(pre))->kind == I_OPER &&((AS_instr)G_nodeInfo(pre))->u.OPER.assem[0] == 'j' &&((AS_instr)G_nodeInfo(pre))->u.OPER.assem[1] == 'm')
                   ;
               else
                   G_addEdge(pre,node);
           }
           pre = node;
           if(instr->kind == I_LABEL)
           {
              Temp_label temp_label = instr->u.LABEL.label;
              T_enter(table,temp_label,node);
           }
        } 
       list = G_nodes(graph);
       for(;list != NULL;list = list->tail){
          G_node node = list->head;
          AS_instr instr = G_nodeInfo(node);
          if(instr->kind == I_OPER && instr->u.OPER.jumps != NULL){
              Temp_labelList templist = instr->u.OPER.jumps->labels;
              for(;templist!= NULL;templist = templist->tail){
                 Temp_label label = templist->head;
                 G_node to = T_look(table,label);
                 if(to != NULL){
                    if(!G_goesTo(node,to))
                        G_addEdge(node,to);
                 }
             }
          }
       }
       return graph;
}
