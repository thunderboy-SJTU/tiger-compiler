#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "color.h"
#include "table.h"
#include "liveness.h"

static C_nodeList Precolored = NULL;
static C_nodeList Initial = NULL;
static C_nodeList SimplifyWorklist = NULL;
static C_nodeList FreezeWorklist = NULL;
static C_nodeList SpillWorklist = NULL;
static C_nodeList SpilledNodes = NULL;
static C_nodeList CoalescedNodes = NULL;
static C_nodeList ColoredNodes = NULL;
static C_nodeList SelectStack = NULL;

static C_moveList CoalescedMoves = NULL;
static C_moveList ConstrainedMoves = NULL;
static C_moveList FrozenMoves = NULL;
static C_moveList WorklistMoves = NULL;
static C_moveList ActiveMoves = NULL;

static bool* adjSet = NULL;
static C_nodeList* adjList = NULL;
static int* degree = NULL;
static TAB_table moveList = NULL;
static TAB_table alias = NULL;
static Temp_map color = NULL;

static TAB_table nodeTable = NULL;

static int K = 0;
static int count = 0;

static int length(Temp_tempList regs){
    for(;regs!= NULL;regs = regs->tail){
        K++;
    }
    return K;
}

void restart(G_graph graph,Temp_map initial, Temp_tempList regs){
  Precolored = NULL;
  Initial = NULL;
  SimplifyWorklist = NULL;
  FreezeWorklist = NULL;
  SpillWorklist = NULL;
  SpilledNodes = NULL;
  CoalescedNodes = NULL;
  ColoredNodes = NULL;
  SelectStack = NULL;
  CoalescedMoves = NULL;
  ConstrainedMoves = NULL;
  FrozenMoves = NULL;
  WorklistMoves = NULL;
  ActiveMoves = NULL;
  adjSet = NULL;
  adjList = NULL;
  degree = NULL;
  moveList = NULL;
  alias = NULL;
  color = NULL;
  nodeTable = NULL;
  K = 0;
  length(regs);
  count = G_nodeCount(graph);
  adjSet = checked_malloc(count*count*sizeof(bool));
  for(int i = 0;i<count*count;i++)
      adjSet[i] = FALSE;
  adjList = checked_malloc(count*sizeof(C_nodeList));
  for(int i = 0;i<count;i++)
      adjList[i] = NULL;
  degree = checked_malloc(count*sizeof(int));
  for(int i = 0;i<count;i++)
      degree[i] = 0;
  moveList = TAB_empty();
  alias = TAB_empty();
  nodeTable = TAB_empty();
  color = initial;
}



static C_node C_Node(G_node gn){
    C_node cn = checked_malloc(sizeof(*cn));
    cn->kind = C_DEFAULT1;
    cn->node = gn;
    TAB_enter(nodeTable,gn,cn);
    return cn;
}

static C_nodeList C_NodeList(C_node cn,C_nodeList pre,C_nodeList succ){
    C_nodeList clist = checked_malloc(sizeof(*clist));
    clist->node = cn;
    clist->pre = pre;
    clist->succ = succ;
    return clist;
}

static C_move C_Move(Live_moveList movelist){
    C_move move = checked_malloc(sizeof(*move));
    move->move = movelist;
    move->kind = C_DEFAULT2;
    return move;
}

static C_moveList C_MoveList(C_move move,C_moveList pre,C_moveList succ){
    C_moveList moveList = checked_malloc(sizeof(*moveList));
    moveList->move = move;
    moveList->pre = pre;
    moveList->succ = succ;
    return moveList;
}

static C_colorList C_ColorList(string ccolor,C_colorList pre,C_colorList succ){
   C_colorList colorList = checked_malloc(sizeof(*colorList));
   colorList->color = ccolor;
   colorList->pre = pre;
   colorList->succ = succ;
   return colorList;
}

static bool inNodeSet(C_node cn,C_nodeList list){
    for(;list != NULL;list = list->succ){
         if(list->node == cn)
            return TRUE;
     }
     return FALSE;
}

static bool inMoveSet(C_move cm,C_moveList list){
     for(;list != NULL;list = list->succ){
         if(list->move == cm)
            return TRUE;
     }
     return FALSE;
}

static void add(C_node cn,C_nodeList* clist){
    C_nodeList newList = C_NodeList(cn,NULL,NULL);
    if((*clist) == NULL)
        (*clist) = newList;
    else{
        (*clist)->pre = newList;
        newList->succ = (*clist);
        (*clist) = newList;
    }
}

static void addMove(C_move cm,C_moveList* mlist){
    C_moveList newList = C_MoveList(cm,NULL,NULL);
    if((*mlist) == NULL)
       (*mlist) = newList;
    else{
       (*mlist)->pre = newList;
       newList->succ = (*mlist);
       (*mlist) = newList;
    }
}

static void nodeRemove(C_node cn,C_nodeList* clist){
    C_nodeList list = *clist;
    for(;list != NULL;list = list->succ){
       if(list->node == cn){
          if(list->pre != NULL)
             list->pre->succ = list->succ;
          if(list->succ != NULL) 
             list->succ->pre = list->pre;
          break;
       }
    }
    if((*clist) == NULL)
      return;
    if((*clist)->node == cn)
       (*clist) = (*clist)->succ;
}

static void moveRemove(C_move cm,C_moveList* mlist){
   C_moveList list = *mlist;
   for(;list != NULL;list = list->succ){
      if(list->move == cm){
         if(list->pre != NULL)
            list->pre->succ = list->succ;
         if(list->succ != NULL)
            list->succ->pre = list->pre;
         break;
      }
   }
   if((*mlist) == NULL)
      return;
   if((*mlist)->move == cm)
     (*mlist) = (*mlist)->succ;
}

static void colorRemove(string c,C_colorList* clist){
  C_colorList list = *clist;
  for(;list != NULL;list = list->succ){
      if(list->color == c){
         if(list->pre != NULL)
            list->pre->succ = list->succ;
         if(list->succ != NULL)
            list->succ->pre = list->pre;
         break;
      }
  }
  if((*clist) == NULL)
      return;
  if((*clist)->color == c)
     (*clist) = (*clist)->succ;
}



static C_nodeList nodeUnion(C_nodeList l1,C_nodeList l2){
   C_nodeList unionSet = NULL;
   C_nodeList tail = NULL;
   C_nodeList tmp = l1;
   for(;l1 != NULL;l1 = l1->succ){
         C_node cnode = l1->node;
         C_nodeList clist = C_NodeList(cnode,tail,NULL);
         if(tail == NULL)
            unionSet = tail = clist;
         else
            tail = tail->succ = clist;
     }
   for(;l2 != NULL;l2 = l2->succ){
         C_node cnode = l2->node;
         if(!inNodeSet(cnode,tmp)){
            C_nodeList clist = C_NodeList(cnode,tail,NULL);
            if(tail == NULL)
               unionSet = tail = clist;
            else
               tail = tail->succ = clist; 
         }
   }
   return unionSet;
}

static C_moveList moveUnion(C_moveList l1,C_moveList l2){
   C_moveList unionSet = NULL;
   C_moveList tail = NULL;
   C_moveList tmp = l1;
   for(;l1 != NULL;l1 = l1->succ){
         C_move cmove = l1->move;
         C_moveList clist = C_MoveList(cmove,tail,NULL);
         if(tail == NULL)
            unionSet = tail = clist;
         else
            tail = tail->succ = clist;
     }
   for(;l2 != NULL;l2 = l2->succ){
         C_move cmove = l2->move;
         if(!inMoveSet(cmove,tmp)){
            C_moveList clist = C_MoveList(cmove,tail,NULL);
            if(tail == NULL)
               unionSet = tail = clist;
            else
               tail = tail->succ = clist; 
         }
   }
   return unionSet;
}

static C_nodeList nodeIntersection(C_nodeList l1,C_nodeList l2){
   C_nodeList interSet = NULL;
   C_nodeList tail = NULL;
   for(;l1 != NULL;l1 = l1->succ){
      C_node cnode = l1->node;
      if (inNodeSet(cnode,l2)){
	  if (tail == NULL){
		interSet = tail = C_NodeList(cnode, NULL, NULL);
	  }
	  else{
		C_nodeList node = C_NodeList(cnode, tail, NULL);
		tail = tail->succ = node;
	  }
      }
   }
   return interSet;
}

static C_moveList moveIntersection(C_moveList l1,C_moveList l2){
   C_moveList interSet = NULL;
   C_moveList tail = NULL;
   for(;l1 != NULL;l1 = l1->succ){
      C_move cmove = l1->move;
      if (inMoveSet(cmove,l2)){
	  if (tail == NULL){
		interSet = tail = C_MoveList(cmove, NULL, NULL);
	  }
	  else{
		C_moveList move = C_MoveList(cmove, tail, NULL);
		tail = tail->succ = move;
	  }
      }
   }
   return interSet;
}

static C_nodeList nodeDiffer(C_nodeList l1,C_nodeList l2){
   C_nodeList differSet = NULL;
   C_nodeList tail = NULL;
   for(;l1 != NULL;l1 = l1->succ){
      C_node cnode = l1->node;
      if (!inNodeSet(cnode,l2)){
	  if (tail == NULL){
		differSet = tail = C_NodeList(cnode, NULL, NULL);
	  }
	  else{
		C_nodeList node = C_NodeList(cnode, tail, NULL);
		tail = tail->succ = node;
	  }
      }
   }
   return differSet;
}

static C_moveList moveDiffer(C_moveList l1,C_moveList l2){
   C_moveList differSet = NULL;
   C_moveList tail = NULL;
   for(;l1 != NULL;l1 = l1->succ){
      C_move cmove = l1->move;
      if (!inMoveSet(cmove,l2)){
	  if (tail == NULL){
		differSet = tail = C_MoveList(cmove, NULL, NULL);
	  }
	  else{
		C_moveList move = C_MoveList(cmove, tail, NULL);
		tail = tail->succ = move;
	  }
      }
   }
   return differSet;
}

   
static bool isNodeEmpty(C_nodeList list){
   return (list == NULL);
}

static bool isMoveEmpty(C_moveList list){
   return (list == NULL);
} 

static C_nodeList adjacent(C_node cn){
   int mykey = G_mykey(cn->node);
   return nodeDiffer(adjList[mykey],nodeUnion(SelectStack,CoalescedNodes));
} 

static bool hasEdge(C_node n1,C_node n2){
   int mykey1 = G_mykey(n1->node);
   int mykey2 = G_mykey(n2->node);
   if(mykey1 == mykey2)
      return FALSE;
   return (adjSet[mykey1* count + mykey2] == TRUE); 
} 
   


static void addEdge(C_node n1,C_node n2){
   int mykey1 = G_mykey(n1->node);
   int mykey2 = G_mykey(n2->node);
   if(mykey1 == mykey2 || hasEdge(n1,n2))
       return;
   adjSet[mykey1*count+mykey2] = TRUE;
   adjSet[mykey2*count+mykey1] = TRUE;
   if(n1->kind != C_PRECOLORED){
       add(n2,&adjList[mykey1]);
       degree[mykey1]++;
   }
   if(n2->kind != C_PRECOLORED){
       add(n1,&adjList[mykey2]);
       degree[mykey2]++;
   }
   return;
}

static void build(G_graph graph,Live_moveList moves){
    
    G_nodeList nl = G_nodes(graph);
    for(;nl != NULL;nl = nl->tail){
        G_node gnode = nl->head;
        C_node cnode = TAB_look(nodeTable,gnode);
        G_nodeList gnlist = G_adj(gnode);
        for(;gnlist!= NULL;gnlist = gnlist->tail){
           G_node adjGnode = gnlist->head;
           C_node adjCnode = TAB_look(nodeTable,adjGnode);
           addEdge(cnode,adjCnode);
        }
    }
    for(;moves != NULL;moves = moves->tail){
        C_move cmove = C_Move(moves);
        cmove->kind = C_WORKLISTMOVES;
        addMove(cmove,&WorklistMoves);
        C_node dst = TAB_look(nodeTable,moves->dst);
        C_node src = TAB_look(nodeTable,moves->src);
        TAB_enter(moveList,dst,moveUnion(TAB_look(moveList,dst),C_MoveList(cmove,NULL,NULL)));
        TAB_enter(moveList,src,moveUnion(TAB_look(moveList,src),C_MoveList(cmove,NULL,NULL)));
    }
}

static C_moveList nodeMoves(C_node cn){
   C_moveList movelist = TAB_look(moveList,cn);
   return moveIntersection(movelist,moveUnion(ActiveMoves,WorklistMoves));
}

static bool moveRelated(C_node cn){
    return (!isMoveEmpty(nodeMoves(cn)));
}


static void makeWorklist(){
   for(;Initial != NULL; Initial = Initial->succ){
      C_node cnode = Initial->node;
      int key = G_mykey(cnode->node);
      if(degree[key] >= K){
         cnode->kind = C_SPILLWORKLIST;
         add(cnode,&SpillWorklist);
      }
      else if(moveRelated(cnode)){
         cnode->kind = C_FREEZEWORKLIST;
         add(cnode,&FreezeWorklist);
      }
      else{
         cnode->kind = C_SIMPLIFYWORKLIST;
         add(cnode,&SimplifyWorklist);
      }
   }
}

static void push(C_node cn,C_nodeList* stack){
   cn->kind = C_SELECTSTACK;
   add(cn,stack);
}

static C_node pop(C_nodeList* stack){
    C_node node = (*stack)->node;
    nodeRemove(node,stack);
    return node;
}

static void enableMoves(C_nodeList list){
   for(;list != NULL; list = list->succ){
      C_node node = list->node;
      C_moveList moves = nodeMoves(node);
      for(;moves!= NULL;moves = moves->succ){
         C_move move = moves->move;
         if(move->kind == C_ACTIVEMOVES){
            moveRemove(move,&ActiveMoves);
            move->kind = C_WORKLISTMOVES;
            addMove(move,&WorklistMoves);
         }
      }
   }
}
            

static void decrementDegree(C_node cn){
   int mykey = G_mykey(cn->node);
   int d = degree[mykey];
   degree[mykey]--;
   if(d == K){
      enableMoves(nodeUnion(C_NodeList(cn,NULL,NULL),adjacent(cn)));
      if(cn->kind == C_FREEZEWORKLIST){
          return;
      }
      nodeRemove(cn,&SpillWorklist);
      if(moveRelated(cn)){
         cn->kind = C_FREEZEWORKLIST;
         add(cn,&FreezeWorklist);
      }
      else{
         cn->kind = C_SIMPLIFYWORKLIST;
         add(cn,&SimplifyWorklist);
      }
   }
}


static void simplify(){
   if(SimplifyWorklist != NULL){
      C_node node = SimplifyWorklist->node;
      nodeRemove(node,&SimplifyWorklist);
      push(node,&SelectStack);
      C_nodeList list = adjacent(node);
      for(;list != NULL;list = list->succ){
         C_node adjNode = list->node;
         decrementDegree(adjNode);
      }
   }
}

static bool OK(C_node t,C_node r){
   int mykey = G_mykey(t->node);
   return ((degree[mykey] < K) || (t->kind == C_PRECOLORED) || hasEdge(t,r));
}

static bool allOK(C_node u,C_node v){
   bool flag = TRUE;
   C_nodeList adjList = adjacent(v);
   for(;adjList != NULL;adjList = adjList->succ){
      C_node t = adjList->node;
      flag = OK(t,u);
      if(flag == FALSE)
         break;
   }
   return flag;
}

static bool conservative(C_nodeList list){
   int k = 0;
   for(;list != NULL;list = list->succ){
     C_node node = list->node;
     int mykey = G_mykey(node->node);
     if(node->kind == C_PRECOLORED){
        k++;
     }
     else if(degree[mykey] >= K)
        k++;
   }
   return k < K;
}

static C_node getAlias(C_node n){
   if(n->kind == C_COALESCEDNODES){
      return getAlias(TAB_look(alias,n));
   }
   else
      return n;
}

static void addWorkList(C_node u){
   if(u->kind != C_PRECOLORED && (!(moveRelated(u))) && (degree[G_mykey(u->node)] < K)){
      nodeRemove(u,&FreezeWorklist);
      u->kind = C_SIMPLIFYWORKLIST;
      add(u,&SimplifyWorklist);
   }
}

static void combine(C_node u,C_node v){
   if(v->kind == C_FREEZEWORKLIST)
      nodeRemove(v,&FreezeWorklist);
   else{
      nodeRemove(v,&SpillWorklist);
   }
   v->kind = C_COALESCEDNODES;
   add(v,&CoalescedNodes);
   TAB_enter(alias,v,u);
   TAB_enter(moveList,u,moveUnion(TAB_look(moveList,u),TAB_look(moveList,v)));
   enableMoves(C_NodeList(v,NULL,NULL));
   C_nodeList adjList = adjacent(v);
   for(;adjList!= NULL;adjList = adjList->succ){
     C_node t = adjList->node;
     addEdge(t,u);
     decrementDegree(t);
   }
   if((degree[G_mykey(u->node)] >= K) && (u->kind == C_FREEZEWORKLIST)){
     nodeRemove(u,&FreezeWorklist);
     u->kind = C_SPILLWORKLIST;
     add(u,&SpillWorklist);
   }
}

static void coalesce(){
   if(WorklistMoves != NULL){
     C_move move = WorklistMoves->move;
     C_node x = getAlias(TAB_look(nodeTable,move->move->dst));
     C_node y = getAlias(TAB_look(nodeTable,move->move->src));
     C_node u,v;
     if(y->kind == C_PRECOLORED){
         u = y;
         v = x;
     }
     else{
         u = x;
         v = y;
     }
     moveRemove(move,&WorklistMoves);
     if(u == v){
       move->kind = C_COALESCEDMOVES;
       addMove(move,&CoalescedMoves);
       addWorkList(u);
     }
     else if(v->kind == C_PRECOLORED || hasEdge(u,v)){
       move->kind = C_CONSTRAINEDMOVES;
       addMove(move,&ConstrainedMoves);
       addWorkList(u);
       addWorkList(v);
     }
     else if(((u->kind == C_PRECOLORED) && allOK(u,v)) ||((u->kind != C_PRECOLORED) && conservative(nodeUnion(adjacent(u),adjacent(v))))){
       move->kind = C_COALESCEDMOVES;
       addMove(move,&CoalescedMoves);
       combine(u,v);
       addWorkList(u);
     }
     else{
       move->kind = C_ACTIVEMOVES;
       addMove(move,&ActiveMoves);
     }
   }
}

static void freezeMoves(C_node u){
    C_moveList mlist = nodeMoves(u);
    for(;mlist != NULL;mlist = mlist->succ){
      C_move move = mlist->move;
      C_node x = TAB_look(nodeTable,move->move->dst);
      C_node y = TAB_look(nodeTable,move->move->src);
      C_node v = NULL;
      if(getAlias(y) == getAlias(u))
         v = getAlias(x);
      else
         v = getAlias(y);
      moveRemove(move,&ActiveMoves);
      move->kind = C_FROZENMOVES;
      addMove(move,&FrozenMoves);
      if(v->kind == C_PRECOLORED)
         continue;
      if(isMoveEmpty(nodeMoves(v)) && degree[G_mykey(v->node)] < K){
        nodeRemove(v,&FreezeWorklist);
        v->kind = C_SIMPLIFYWORKLIST;
        add(v,&SimplifyWorklist);
      }
   }
}

static void freeze(){
  if(FreezeWorklist != NULL){
     C_node u = FreezeWorklist->node;
     nodeRemove(u,&FreezeWorklist);
     u->kind = C_SIMPLIFYWORKLIST;
     add(u,&SimplifyWorklist);
     freezeMoves(u);
  }
}

static C_node getMaxSpill(){
  int max = -1;
  C_node chooseNode = NULL;
  C_nodeList list = SpillWorklist;
  for(;list != NULL;list = list->succ){
    C_node node = list->node;
    if(degree[G_mykey(node->node)] > max){
       max = degree[G_mykey(node->node)];
       chooseNode = node;
    }
  }
  return chooseNode;
}

static void selectSpill(){

  C_node m = getMaxSpill();
  nodeRemove(m,&SpillWorklist);
  m->kind = C_SIMPLIFYWORKLIST;
  add(m,&SimplifyWorklist);
  freezeMoves(m);
}


static C_colorList colors(Temp_tempList regs){
   C_colorList colorList = NULL;
   C_colorList tail = NULL;
   for(;regs!= NULL;regs = regs->tail){
      string reg = Temp_look(color,regs->head); 
      C_colorList tmplist = C_ColorList(reg,tail,NULL);
      if(tail == NULL)
         colorList = tail = tmplist;
      else
         tail = tail->succ = tmplist;
   }
   return colorList;
}

        


static void assignColors(Temp_tempList regs){
   while(!isNodeEmpty(SelectStack)){
      C_node n = pop(&SelectStack);
      C_colorList okColors = colors(regs);
      C_nodeList adjNodes = adjList[G_mykey(n->node)];
      for(;adjNodes!= NULL;adjNodes = adjNodes->succ){
          C_node w = adjNodes->node;
          if(inNodeSet(getAlias(w),nodeUnion(ColoredNodes,Precolored))){
              string c = Temp_look(color,G_nodeInfo(getAlias(w)->node));
              colorRemove(c,&okColors);
          }
      }
      if(okColors == NULL){
         n->kind = C_SPILLEDNODES;
         add(n,&SpilledNodes);
      }
      else{
         n->kind = C_COLOREDNODES;
         add(n,&ColoredNodes);
         Temp_enter(color,G_nodeInfo(n->node),okColors->color);
      }  
   }
   C_nodeList coalescedNodes = CoalescedNodes;   
   for(;coalescedNodes!= NULL;coalescedNodes = coalescedNodes->succ){
         C_node n = coalescedNodes->node;
         Temp_enter(color,G_nodeInfo(n->node),Temp_look(color,G_nodeInfo(getAlias(n)->node)));    
   }
}   
        
        


struct COL_result COL_color(G_graph ig, Temp_map initial, Temp_tempList regs) {
	//your code here.
	struct COL_result ret;
        struct Live_graph lg = Live_liveness(ig);
        G_graph graph = lg.graph;
        Live_moveList moves = lg.moves;
        restart(graph,initial,regs);
        G_nodeList nl = G_nodes(graph);
        for(;nl != NULL;nl = nl->tail){
            G_node gnode = nl->head;
            C_node cnode = C_Node(gnode);
            if(Temp_look(initial,G_nodeInfo(gnode))!= NULL)
            {
               cnode->kind = C_PRECOLORED;
               add(cnode,&Precolored);
            }
            else{
               cnode->kind = C_INITIAL;
               add(cnode,&Initial);
            }
        }        
        build(graph,moves);
        makeWorklist();
        do{
            if(!isNodeEmpty(SimplifyWorklist)){
               simplify();
            }
            else if(!isMoveEmpty(WorklistMoves)){
               coalesce();
            }
            else if(!isNodeEmpty(FreezeWorklist)){
               freeze();
            }
            else if(!isNodeEmpty(SpillWorklist)){
               selectSpill();
            }
       }while((!isNodeEmpty(SimplifyWorklist)) || (!isMoveEmpty(WorklistMoves)) || (!isNodeEmpty(FreezeWorklist))||(!isNodeEmpty(SpillWorklist)));
       C_nodeList pre = Precolored;
       assignColors(regs);
       ret.coloring = color;
       Temp_tempList spillNodes = NULL;
       for(;SpilledNodes != NULL;SpilledNodes = SpilledNodes->succ){
          Temp_temp spill = G_nodeInfo(SpilledNodes->node->node);
          spillNodes = Temp_TempList(spill,spillNodes);
       }
       ret.spills = spillNodes;                    
       return ret;
}


