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
#include "liveness.h"
#include "table.h"

Live_moveList Live_MoveList(G_node src, G_node dst, Live_moveList tail) {
	Live_moveList lm = (Live_moveList) checked_malloc(sizeof(*lm));
	lm->src = src;
	lm->dst = dst;
	lm->tail = tail;
	return lm;
}


 
static void enterLiveMap(G_table t,G_node flowNode,Temp_tempList temps){
    G_enter(t,flowNode,temps);
}

static Temp_tempList lookupLiveMap(G_table t,G_node flownode){
   return (Temp_tempList)G_look(t,flownode);
}

static G_nodeList reverse(G_nodeList list){
    G_nodeList newList = NULL;
    for(;list!= NULL;list = list->tail){
        G_node node = list->head;
        newList = G_NodeList(node,newList);
    }
    return newList;
}

static bool inSet(Temp_temp temp,Temp_tempList list){
     if(!temp)
       return FALSE;
     for(;list != NULL;list = list->tail){
         if(list->head == temp)
            return TRUE;
     }
     return FALSE;
}

static Temp_tempList tempUnion(Temp_tempList t1,Temp_tempList t2){
     Temp_tempList unionSet = NULL;
     Temp_tempList tail = NULL;
     Temp_tempList tmp = t1;
     for(;t1 != NULL;t1 = t1->tail){
          Temp_temp temp = t1->head;
          Temp_tempList templist = Temp_TempList(temp,NULL);
          if(tail == NULL)
             unionSet = tail = templist;
          else
             tail = tail->tail = templist;
     }
     for(;t2 != NULL;t2 = t2->tail){
         Temp_temp temp = t2->head;
         if(!inSet(temp,tmp)){
           Temp_tempList templist = Temp_TempList(temp,NULL);
           if(tail == NULL)
              unionSet = tail = templist;
           else
              tail = tail->tail = templist; 
         }
     }
     return unionSet;
}

static Temp_tempList tempDiffer(Temp_tempList t1,Temp_tempList t2){
    Temp_tempList differSet = NULL;
    Temp_tempList tail = NULL;
    for(;t1!=NULL;t1 = t1->tail){
        Temp_temp temp = t1->head;
        if(!inSet(temp,t2)){
          Temp_tempList templist = Temp_TempList(temp,NULL);
          if(tail == NULL)
             differSet = tail = templist;
          else
             tail = tail->tail = templist;
        }
    }
    return differSet;
}

static bool cmp(Temp_tempList t1,Temp_tempList t2){    //仅仅比较数量就可以了，只会多不会少
     int count1 = 0,count2 = 0;
     for(;t1 != NULL;t1 = t1->tail)
       count1++;
     for(;t2!= NULL; t2 = t2->tail)
       count2++;
     if(count1 == count2)
        return TRUE;
     return FALSE;
}
        
       

Temp_temp Live_gtemp(G_node n) {
	//your code here.
	return G_nodeInfo(n);
}

struct Live_graph Live_liveness(G_graph flow) {
	//your code here.
	struct Live_graph lg;
        G_graph graph = G_Graph();
        Live_moveList moves = NULL;
        bool isChange = TRUE;
        G_table inTable = G_empty();
        G_table outTable = G_empty();
        G_nodeList list = reverse(G_nodes(flow));
        while(isChange == TRUE){
           isChange = FALSE;
           G_nodeList tmp = list;
           for(;tmp!= NULL;tmp = tmp->tail){
              G_node node = tmp->head;
              Temp_tempList inBefore = lookupLiveMap(inTable,node);
              Temp_tempList outBefore = lookupLiveMap(outTable,node);
              Temp_tempList inAfter = NULL;
              Temp_tempList outAfter = NULL;
              inAfter = tempUnion(FG_use(node),tempDiffer(outBefore,FG_def(node)));
              G_nodeList succ = G_succ(node);
              for(;succ!=NULL;succ = succ->tail){
                 G_node succNode = succ->head;
                 Temp_tempList succIn = lookupLiveMap(inTable,succNode);
                 outAfter = tempUnion(outAfter,succIn);
              }
              if(!cmp(inBefore,inAfter)){
                 isChange = TRUE;
                 enterLiveMap(inTable,node,inAfter);
              }
              if(!cmp(outBefore,outAfter)){
                 isChange = TRUE;
                 enterLiveMap(outTable,node,outAfter);
              }
           }
        }
        G_nodeList tmp = list;
        TAB_table tempTable = TAB_empty();
        for(;tmp != NULL;tmp = tmp->tail){
           G_node node = tmp->head;
           Temp_tempList defs = FG_def(node);
           Temp_tempList uses = FG_use(node);
           for(;defs != NULL;defs = defs->tail){
              Temp_temp def = defs->head;
              G_node varNode = TAB_look(tempTable,def);
              if(varNode == NULL){
                 varNode = G_Node(graph,def);
                 TAB_enter(tempTable,def,varNode);
              }
           }
           for(;uses != NULL;uses = uses->tail){
              Temp_temp use = uses->head;
              if(use == NULL){
                AS_instr instr = G_nodeInfo(node);
              }
              G_node varNode = TAB_look(tempTable,use);
              if(varNode == NULL){
                 varNode = G_Node(graph,use);
                 TAB_enter(tempTable,use,varNode);
              }
           }
        }
        tmp = list;
        for(;tmp!= NULL;tmp = tmp->tail){
           G_node node = tmp->head;
           if(FG_isMove(node)){
              Temp_tempList defs = FG_def(node);
              Temp_tempList uses = FG_use(node);
              G_node dnode = TAB_look(tempTable,defs->head);   //move指令应该只有一个def和一个use
              G_node unode = TAB_look(tempTable,uses->head);
              moves = Live_MoveList(dnode,unode,moves);
              for(;defs != NULL;defs = defs->tail){
                 Temp_temp def = defs->head;
                 G_node defNode = TAB_look(tempTable,def);
                 Temp_tempList liveouts = lookupLiveMap(outTable,node);
                 Temp_tempList newliveouts = tempDiffer(liveouts,uses);
                 for(;newliveouts != NULL;newliveouts = newliveouts->tail){
                    Temp_temp liveout = newliveouts->head;
                    G_node outNode = TAB_look(tempTable,liveout);
                    if(def != liveout && (!G_goesTo(defNode,outNode))){
                         G_addEdge(defNode,outNode);
                         G_addEdge(outNode,defNode);
                    } 
                 }
              }
           }
           else{
              Temp_tempList defs = FG_def(node);
              for(;defs != NULL;defs = defs->tail){
                 Temp_temp def = defs->head;
                 G_node defNode = TAB_look(tempTable,def);
                 Temp_tempList liveouts = lookupLiveMap(outTable,node);
                 for(;liveouts != NULL;liveouts = liveouts->tail){
                    Temp_temp liveout = liveouts->head;
                    G_node outNode = TAB_look(tempTable,liveout);
                    if(def != liveout && (!G_goesTo(defNode,outNode))){
                         G_addEdge(defNode,outNode);
                         G_addEdge(outNode,defNode);
                    } 
                 }
              }
           }
        }
        lg.graph = graph;
        lg.moves = moves;               
	return lg;
}


