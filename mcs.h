#ifndef MCS_H
#define MCS_H

struct qnode {
  struct qnode *next;
  int locked;
};

typedef struct qnode * mcs_lock_t;

// parameter I, below, points to a qnode record allocated
// (in an enclosing scope) in shared memory locally-accessible
// to the invoking processor

void mcs_lock(mcs_lock_t *L, struct qnode *I);

void mcs_unlock(mcs_lock_t *L, struct qnode *I);

#endif /* MCS_H */
