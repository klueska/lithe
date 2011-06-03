/*
 * Lithe
 *
 * TODO(benh): Include a discussion of the transition stack and
 * callbacks versus entry points.
 *
 * TODO(benh): Include a discussion of tasks, including the
 * distinction between implicit and explicit tasks.
 */

#ifndef LITHE_H
#define LITHE_H

#ifndef __GNUC__
#error "expecting __GNUC__ to be defined (are you using gcc?)"
#endif

#include <stdarg.h>
#include <ucontext.h>

#include <ht/ht.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Value used to inform parent of an unlimited hard thread request. */
#define LITHE_REQUEST_MAX (-1)


/* Opaque type for lithe schedulers. */
typedef struct lithe_sched lithe_sched_t;

/* Lithe task structure. */
typedef struct lithe_task {
  /* Userlevel context. */
  ucontext_t ctx;

  /* Owning scheduler. */
  lithe_sched_t *sched;

  /* Task-local storage. */
  void *tls;
} lithe_task_t;

/* Lithe scheduler callbacks/entrypoints. */
typedef struct lithe_sched_funcs {
  /* Entry point for hard threads from the parent scheduler. */
  void (*enter) (void *__this);

  /* Entry point for hard threads from a child scheduler. */
  void (*yield) (void *__this, lithe_sched_t *child);

  /* Callback (on the parent) to inform of a registerd child. */
  void (*reg) (void *__this, lithe_sched_t *child);

  /* Callback (on the parent) to inform of an unregistered child. */
  void (*unreg) (void *__this, lithe_sched_t *child);

  /* Callback (on the parent) to inform of child's request for hard threads. */
  void (*request) (void *__this, lithe_sched_t *child, int k);

  /* Callback (on any scheduler) to inform of a resumable task. */
  void (*unblock) (void *__this, lithe_task_t *task);
} lithe_sched_funcs_t;

/**
 * Initialize Lithe. This must be called on the main hard thread
 * before any other Lithe functions are used, otherwise, behavior is
 * undefined. Returns 0 on success and -1 on failures and sets errno
 * appropriately.
*/
int lithe_initialize();
  
/**
 * Return the internals of the current scheduler. This can be used in
 * conjunction with a macro to get a "poor man's this":
 *
 *   #define this ((type *) (lithe_sched_this()))
*/
void * lithe_sched_this();

/* TODO(benh): Put sysdep stuff someplace else. */

/**
 * Grant current hard thread to specified scheduler. The specified
 * scheduler must be non-null and a registered scheduler of the
 * current scheduler. This function never returns unless child is
 * NULL, in which case it sets errno appropriately and returns -1.
 */
#ifdef __x86_64__
#ifdef __PIC__
#define lithe_sched_enter(child)                                      \
({                                                                    \
  asm volatile ("mov %0, %%rdi;"                                      \
		"jmp ___lithe_sched_enter@PLT"                        \
		: : "r" (child));                                     \
})
#else
#define lithe_sched_enter(child)                                      \
({                                                                    \
  asm volatile ("mov %0, %%rdi;"                                      \
                "jmp ___lithe_sched_enter"                            \
		: : "r" (child));                                     \
})
#endif /* __PIC__ */
#elif __i386__
# ifdef __PIC__
# define lithe_sched_enter(child)                                      \
({                                                                    \
  asm volatile ("mov %0, %%edi;"                                      \
		"jmp ___lithe_sched_enter@PLT"                        \
		: : "r" (child));                                     \
})
# else
# define lithe_sched_enter(child)                                      \
({                                                                    \
  asm volatile ("mov %0, %%edi;"                                      \
                "jmp ___lithe_sched_enter"                            \
		: : "r" (child));                                     \
})
# endif /* __PIC__ */
#else
# error "missing implementations for your architecture"
#endif

/**
 * Yield current hard thread to parent scheduler. This function should
 * never return.
 */
#ifdef __PIC__
#define lithe_sched_yield()                                           \
({                                                                    \
  asm volatile ("jmp ___lithe_sched_yield@PLT");                      \
})
#else
#define lithe_sched_yield()                                           \
({                                                                    \
  asm volatile ("jmp ___lithe_sched_yield");                          \
})
#endif /* __PIC__ */


/**
 * Reenter current scheduler. This function should never return.
 */
#ifdef __PIC__
#define lithe_sched_reenter()                                         \
({                                                                    \
  asm volatile ("jmp ___lithe_sched_reenter@PLT");                    \
})
#else
#define lithe_sched_reenter()                                         \
({                                                                    \
  asm volatile ("jmp ___lithe_sched_reenter");                        \
})
#endif /* __PIC__ */

/**
 * Create a new scheduler instance (lithe_sched_t), registers it with
 * the current scheduler, and updates the current hard thread to be
 * scheduled by this new scheduler. Returns -1 if there is an error
 * and sets errno appropriately.
*/
int lithe_sched_register(const lithe_sched_funcs_t *funcs,
			 void *__this,
			 void (*func) (void *),
			 void *arg);

int lithe_sched_register_task(const lithe_sched_funcs_t *funcs,
			      void *__this,
			      lithe_task_t *task);

/**
 * Unregister the current scheduler. This call blocks the current
 * hard thread until all resources have been recovered.
 * TODO(benh): Add return semantics comment.
*/
int lithe_sched_unregister();

int lithe_sched_unregister_task(lithe_task_t **task);

/**
 * Request the specified number of hard threads from the parent. Note
 * that the parent is free to make a request using the calling hard
 * thread to their parent if necessary.
 * TODO(benh): Add return semantics comment.
 */
int lithe_sched_request(int k);

/*
 * Initialize a new task. Returns 0 on success and -1 on error and
 * sets errno appropriately.
 */
int lithe_task_init(lithe_task_t *task, stack_t *stack);

/*
 * Destroy an existing task. Returns 0 on success and -1 on error and
 * sets errno appropriately.
 */
int lithe_task_destroy(lithe_task_t *task);

/*
 * Returns the currently executing task. Returns 0 on success and -1
 * on error and sets errno appropriately.
 */
int lithe_task_get(lithe_task_t **task);

/*
 * Set the task local storage of the current task. Returns 0 on
 * success and -1 on error and sets errno appropriately.
 */
int lithe_task_settls(void *tls);

/*
 * Get the task local storage of the current task. Returns 0 on
 * success and -1 on error and sets errno appropriately.
 */
int lithe_task_gettls(void **tls);

/*
 * Use the specified task to call the specified function. This
 * function never returns on success and returns -1 on error and sets
 * errno appropriately.
 */
int lithe_task_do(lithe_task_t *task, void (*func) (void *), void *arg);

/**
 * Invoke the specified function with the current task. Note that you
 * can not block without a valid task (i.e. you can not block when
 * executing on the trampoline). Returns 0 on success (after the task
 * has been resumed) and -1 on error and sets errno appropriately.
*/
int lithe_task_block(void (*func) (lithe_task_t *, void *), void *arg);

/**
 * Notifies the current scheduler that the specified task is
 * resumable. Returns 0 on success and -1 on error and sets errno
 * appropriately.
 */
int lithe_task_unblock(lithe_task_t *task);

/*
 * Resume the specified task. This function never returns on success
 * and returns -1 on error and sets errno appropriately.
 */
int lithe_task_resume(lithe_task_t *task);


#ifdef __cplusplus
}
#endif

#endif  // LITHE_H
