/**
 * Memory consistency issues will vary between machines ... here are
 * a few helpful guarantees for x86_64 machines:
 *
 *  - Loads are _not_ reordered with other loads.
 *  - Stores are _not_ reordered with other stores.
 *  - Stores are _not_ reordered with older loads.
 *  - In a multiprocessor system, memory ordering obeys causality (memory
 *    ordering respects transitive visibility).
 *  - In a multiprocessor system, stores to the same location have a
 *    total order.
 *  - In a multiprocessor system, locked instructions have a total order.
 *  - Loads and stores are not reordered with locked instructions.
 *
 * However, loads _may be_ reordered with older stores to different
 * locations. This sometimes necesitates the need for memory fences
 * (and no the keyword 'volatile' will not _always_ do that for
 * you!). In addition, a memory fence may sometimes be helpful to
 * avoid letting the compiler do any of its own fancy reorderings.
*/

#ifndef ATOMIC_H
#define ATOMIC_H

#ifndef __GNUC__
#error "expecting __GNUC__ to be defined (are you using gcc?)"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __x86_64__

#define mfence() ({ asm volatile ("mfence" : : :"memory"); })


#define rdfence() (mfence())


#define wrfence() ({ asm volatile ("" : : :"memory"); })


#define cmpxchg(ptr, value, comparand)            \
({                                                \
  int result;                                     \
  asm volatile ("lock\n\t"                        \
		"cmpxchg %2,%1\n"                 \
		: "=a" (result), "=m" (*ptr)      \
		: "q" (value), "0" (comparand)    \
		: "memory");                      \
  result;                                         \
})


#define fetch_and_add(ptr, addend)                \
({                                                \
  int result;                                     \
  asm volatile ("lock\n\t"                        \
		"xadd %0,%1\n"                    \
		: "=r" (result), "=m" (*ptr)      \
		: "0" (addend)                    \
		: "memory");                      \
  result;                                         \
})


#define fetch_and_store(ptr, value)               \
({                                                \
  void *result;                                   \
  asm volatile ("lock\n\t"                        \
		"xchg %0,%1\n"                    \
		: "=r" (result), "=m" (*ptr)      \
		: "0" (value)                     \
		: "memory");                      \
  result;                                         \
})


#define coherent_read(v)                          \
({                                                \
  rdfence();                                      \
  *(&(v));                                        \
})


#define __arch_compare_and_exchange_val_8_acq(mem, newval, oldval) \
  ({ __typeof (*mem) ret;						      \
     __asm __volatile ("lock;" "cmpxchgb %b2, %1"			      \
		       : "=a" (ret), "=m" (*mem)			      \
		       : "q" (newval), "m" (*mem), "0" (oldval));	      \
     ret; })

#define __arch_compare_and_exchange_val_16_acq(mem, newval, oldval) \
  ({ __typeof (*mem) ret;						      \
     __asm __volatile ("lock;" "cmpxchgw %w2, %1"			      \
		       : "=a" (ret), "=m" (*mem)			      \
		       : "r" (newval), "m" (*mem), "0" (oldval));	      \
     ret; })

#define __arch_compare_and_exchange_val_32_acq(mem, newval, oldval) \
  ({ __typeof (*mem) ret;						      \
     __asm __volatile ("lock;" "cmpxchgl %2, %1"			      \
		       : "=a" (ret), "=m" (*mem)			      \
		       : "r" (newval), "m" (*mem), "0" (oldval));	      \
     ret; })

#define __arch_compare_and_exchange_val_64_acq(mem, newval, oldval) \
  ({ __typeof (*mem) ret;						      \
     __asm __volatile ("lock;" "cmpxchgq %q2, %1"			      \
		       : "=a" (ret), "=m" (*mem)			      \
		       : "r" ((long) (newval)), "m" (*mem),		      \
			 "0" ((long) (oldval)));			      \
     ret; })


/* Note that we need no lock prefix.  */
#define atomic_exchange_acq(mem, newvalue) \
  ({ __typeof (*mem) result;						      \
     if (sizeof (*mem) == 1)						      \
       __asm __volatile ("xchgb %b0, %1"				      \
			 : "=r" (result), "=m" (*mem)			      \
			 : "0" (newvalue), "m" (*mem));			      \
     else if (sizeof (*mem) == 2)					      \
       __asm __volatile ("xchgw %w0, %1"				      \
			 : "=r" (result), "=m" (*mem)			      \
			 : "0" (newvalue), "m" (*mem));			      \
     else if (sizeof (*mem) == 4)					      \
       __asm __volatile ("xchgl %0, %1"					      \
			 : "=r" (result), "=m" (*mem)			      \
			 : "0" (newvalue), "m" (*mem));			      \
     else								      \
       __asm __volatile ("xchgq %q0, %1"				      \
			 : "=r" (result), "=m" (*mem)			      \
			 : "0" ((long) (newvalue)), "m" (*mem));	      \
     result; })


#define atomic_exchange_and_add(mem, value) \
  ({ __typeof (*mem) result;						      \
     if (sizeof (*mem) == 1)						      \
       __asm __volatile ("lock;" "xaddb %b0, %1"			      \
			 : "=r" (result), "=m" (*mem)			      \
			 : "0" (value), "m" (*mem));			      \
     else if (sizeof (*mem) == 2)					      \
       __asm __volatile ("lock;" "xaddw %w0, %1"			      \
			 : "=r" (result), "=m" (*mem)			      \
			 : "0" (value), "m" (*mem));			      \
     else if (sizeof (*mem) == 4)					      \
       __asm __volatile ("lock;" "xaddl %0, %1"			      \
			 : "=r" (result), "=m" (*mem)			      \
			 : "0" (value), "m" (*mem));			      \
     else								      \
       __asm __volatile ("lock;" "xaddq %q0, %1"			      \
			 : "=r" (result), "=m" (*mem)			      \
			 : "0" ((long) (value)), "m" (*mem));		      \
     result; })


#define atomic_add(mem, value) \
  (void) ({ if (__builtin_constant_p (value) && (value) == 1)		      \
	      atomic_increment (mem);					      \
	    else if (__builtin_constant_p (value) && (value) == 1)	      \
	      atomic_decrement (mem);					      \
	    else if (sizeof (*mem) == 1)				      \
	      __asm __volatile ("lock;" "addb %b1, %0"  	    	      \
				: "=m" (*mem)				      \
				: "ir" (value), "m" (*mem));		      \
	    else if (sizeof (*mem) == 2)				      \
	      __asm __volatile ("lock;" "addw %w1, %0"  		      \
				: "=m" (*mem)				      \
				: "ir" (value), "m" (*mem));		      \
	    else if (sizeof (*mem) == 4)				      \
	      __asm __volatile ("lock;" "addl %1, %0"   		      \
				: "=m" (*mem)				      \
				: "ir" (value), "m" (*mem));		      \
	    else							      \
	      __asm __volatile ("lock;" "addq %q1, %0"		              \
				: "=m" (*mem)				      \
				: "ir" ((long) (value)), "m" (*mem));	      \
	    })


#define atomic_add_negative(mem, value) \
  ({ unsigned char __result;						      \
     if (sizeof (*mem) == 1)						      \
       __asm __volatile ("lock;" "addb %b2, %0; sets %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "ir" (value), "m" (*mem));			      \
     else if (sizeof (*mem) == 2)					      \
       __asm __volatile ("lock;" "addw %w2, %0; sets %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "ir" (value), "m" (*mem));			      \
     else if (sizeof (*mem) == 4)					      \
       __asm __volatile ("lock;" "addl %2, %0; sets %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "ir" (value), "m" (*mem));			      \
     else								      \
       __asm __volatile ("lock;" "addq %q2, %0; sets %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "ir" ((long) (value)), "m" (*mem));		      \
     __result; })


#define atomic_add_zero(mem, value) \
  ({ unsigned char __result;						      \
     if (sizeof (*mem) == 1)						      \
       __asm __volatile ("lock;" "addb %b2, %0; setz %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "ir" (value), "m" (*mem));			      \
     else if (sizeof (*mem) == 2)					      \
       __asm __volatile ("lock;" "addw %w2, %0; setz %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "ir" (value), "m" (*mem));			      \
     else if (sizeof (*mem) == 4)					      \
       __asm __volatile ("lock;" "addl %2, %0; setz %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "ir" (value), "m" (*mem));			      \
     else								      \
       __asm __volatile ("lock;" "addq %q2, %0; setz %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "ir" ((long) (value)), "m" (*mem));		      \
     __result; })


#define atomic_increment(mem) \
  (void) ({ if (sizeof (*mem) == 1)					      \
	      __asm __volatile ("lock;" "incb %b0"			      \
				: "=m" (*mem)				      \
				: "m" (*mem));				      \
	    else if (sizeof (*mem) == 2)				      \
	      __asm __volatile ("lock;" "incw %w0"			      \
				: "=m" (*mem)				      \
				: "m" (*mem));				      \
	    else if (sizeof (*mem) == 4)				      \
	      __asm __volatile ("lock;" "incl %0"			      \
				: "=m" (*mem)				      \
				: "m" (*mem));				      \
	    else							      \
	      __asm __volatile ("lock;" "incq %q0"			      \
				: "=m" (*mem)				      \
				: "m" (*mem));				      \
	    })


#define atomic_increment_and_test(mem) \
  ({ unsigned char __result;						      \
     if (sizeof (*mem) == 1)						      \
       __asm __volatile ("lock;" "incb %b0; sete %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "m" (*mem));					      \
     else if (sizeof (*mem) == 2)					      \
       __asm __volatile ("lock;" "incw %w0; sete %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "m" (*mem));					      \
     else if (sizeof (*mem) == 4)					      \
       __asm __volatile ("lock;" "incl %0; sete %1"			      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "m" (*mem));					      \
     else								      \
       __asm __volatile ("lock;" "incq %q0; sete %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "m" (*mem));					      \
     __result; })


#define atomic_decrement(mem) \
  (void) ({ if (sizeof (*mem) == 1)					      \
	      __asm __volatile ("lock;" "decb %b0"			      \
				: "=m" (*mem)				      \
				: "m" (*mem));				      \
	    else if (sizeof (*mem) == 2)				      \
	      __asm __volatile ("lock;" "decw %w0"			      \
				: "=m" (*mem)				      \
				: "m" (*mem));				      \
	    else if (sizeof (*mem) == 4)				      \
	      __asm __volatile ("lock;" "decl %0"			      \
				: "=m" (*mem)				      \
				: "m" (*mem));				      \
	    else							      \
	      __asm __volatile ("lock;" "decq %q0"			      \
				: "=m" (*mem)				      \
				: "m" (*mem));				      \
	    })


#define atomic_decrement_and_test(mem) \
  ({ unsigned char __result;						      \
     if (sizeof (*mem) == 1)						      \
       __asm __volatile ("lock;" "decb %b0; sete %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "m" (*mem));					      \
     else if (sizeof (*mem) == 2)					      \
       __asm __volatile ("lock;" "decw %w0; sete %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "m" (*mem));					      \
     else if (sizeof (*mem) == 4)					      \
       __asm __volatile ("lock;" "decl %0; sete %1"			      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "m" (*mem));					      \
     else								      \
       __asm __volatile ("lock;" "decq %q0; sete %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "m" (*mem));					      \
     __result; })


#define atomic_bit_set(mem, bit) \
  (void) ({ if (sizeof (*mem) == 1)					      \
	      __asm __volatile ("lock;" "orb %b2, %0"		      \
				: "=m" (*mem)				      \
				: "m" (*mem), "ir" (1L << (bit)));	      \
	    else if (sizeof (*mem) == 2)				      \
	      __asm __volatile ("lock;" "orw %w2, %0"		      \
				: "=m" (*mem)				      \
				: "m" (*mem), "ir" (1L << (bit)));	      \
	    else if (sizeof (*mem) == 4)				      \
	      __asm __volatile ("lock;" "orl %2, %0"		      \
				: "=m" (*mem)				      \
				: "m" (*mem), "ir" (1L << (bit)));	      \
	    else if (__builtin_constant_p (bit) && (bit) < 32)		      \
	      __asm __volatile ("lock;" "orq %2, %0"		      \
				: "=m" (*mem)				      \
				: "m" (*mem), "i" (1L << (bit)));	      \
	    else							      \
	      __asm __volatile ("lock;" "orq %q2, %0"		      \
				: "=m" (*mem)				      \
				: "m" (*mem), "r" (1UL << (bit)));	      \
	    })


#define atomic_bit_test_set(mem, bit) \
  ({ unsigned char __result;						      \
     if (sizeof (*mem) == 1)						      \
       __asm __volatile ("lock;" "btsb %3, %1; setc %0" 		      \
			 : "=q" (__result), "=m" (*mem)			      \
			 : "m" (*mem), "ir" (bit));			      \
     else if (sizeof (*mem) == 2)					      \
       __asm __volatile ("lock;" "btsw %3, %1; setc %0" 		      \
			 : "=q" (__result), "=m" (*mem)			      \
			 : "m" (*mem), "ir" (bit));			      \
     else if (sizeof (*mem) == 4)					      \
       __asm __volatile ("lock;" "btsl %3, %1; setc %0" 	   	      \
			 : "=q" (__result), "=m" (*mem)			      \
			 : "m" (*mem), "ir" (bit));			      \
     else							      	      \
       __asm __volatile ("lock;" "btsq %3, %1; setc %0" 		      \
			 : "=q" (__result), "=m" (*mem)			      \
			 : "m" (*mem), "ir" (bit));			      \
     __result; })


#define atomic_delay() ({ asm volatile ("rep; nop"); })

#else /* __x86_64__ */
#error "missing atomics implementation for your architecture"
#endif

#ifdef __cplusplus
}
#endif

#endif /* ATOMIC_H */

