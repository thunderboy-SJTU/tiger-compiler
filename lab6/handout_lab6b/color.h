/*
 * color.h - Data structures and function prototypes for coloring algorithm
 *             to determine register allocation.
 */
#include"liveness.h"

struct COL_result {Temp_map coloring; Temp_tempList spills;};
struct COL_result COL_color(G_graph ig, Temp_map initial, Temp_tempList regs);

typedef struct C_node_* C_node;
typedef struct C_nodeList_* C_nodeList;

typedef struct C_move_* C_move;
typedef struct C_moveList_* C_moveList;

typedef struct C_colorList_* C_colorList;

struct C_node_{
   enum {C_PRECOLORED,C_INITIAL,C_SIMPLIFYWORKLIST,C_FREEZEWORKLIST,C_SPILLWORKLIST,C_SPILLEDNODES,C_COALESCEDNODES,C_COLOREDNODES,C_SELECTSTACK,C_DEFAULT1} kind;
   G_node node;
};

struct C_nodeList_{
   C_node node;
   C_nodeList pre;
   C_nodeList succ;
};

struct C_move_{
   enum {C_COALESCEDMOVES,C_CONSTRAINEDMOVES,C_FROZENMOVES,C_WORKLISTMOVES,C_ACTIVEMOVES,C_DEFAULT2} kind;
   Live_moveList move;
};

struct C_moveList_{
   C_move move;
   C_moveList pre;
   C_moveList succ;
};

struct C_colorList_{
  string color;
  C_colorList pre;
  C_colorList succ;
};


   


