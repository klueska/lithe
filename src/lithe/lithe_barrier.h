#ifndef LITHE_BARRIER_H
#define LITHE_BARRIER_H


#include <lithe/lithe.h>
#include <spinlock.h>
#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus 
extern "C" {
#endif


/* TODO: Figure out more elegant & portable solution to
 * creating padded arrays. For now, assume that the L1 cache 
 * line is CACHE_LINE_SIZE bytes to avoid false sharing. */

#define CACHE_LINE_SIZE 64


typedef union {
  bool val;
  uint8_t padding[CACHE_LINE_SIZE];
} padded_bool_t;  


typedef struct {
  lithe_task_t **queue;
  int len;
  int mtx;
  int maxlen;
} taskq_t;  


typedef struct {
  int N; 
  int arrived;
  bool wait;
  padded_bool_t *signals;
  taskq_t blocked[2];
} lithe_barrier_t;


void lithe_barrier_init(lithe_barrier_t *barrier, int nthreads);
void lithe_barrier_destroy(lithe_barrier_t *barrier);
void lithe_barrier_wait(lithe_barrier_t *barrier);


#ifdef __cplusplus 
}
#endif


#endif  // LITHE_BARRIER_H
