#ifndef ESCAPE_H
#define ESCAPE_H

typedef struct Esc_entry_* Esc_entry;

struct Esc_entry_{
   int depth;
   bool* escape;
};

Esc_entry Esc_Entry(int depth,bool* escape);

void Esc_findEscape(A_exp exp);

#endif
