/*
 * flowgraph.h - Function prototypes to represent control flow graphs.
 */

Temp_tempList FG_def(G_node n);
Temp_tempList FG_use(G_node n);
bool FG_isMove(G_node n);
G_graph FG_AssemFlowGraph(AS_instrList il, F_frame f);


typedef struct TAB_table_  *T_table;

T_table T_empty(void);

void T_enter(T_table t, Temp_label label, void *node);

void *T_look(T_table t, Temp_label label);
