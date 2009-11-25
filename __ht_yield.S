/*
 * When yielding a pthread based hard thread we need to swap in a stack
 * in order to swap to our saved cleanup context. This is analogous to a
 * system call in which we swap to the kernel stack. Note that we require
 * at least __ht_stack and __ht_context to use the "initial-exec" TLS model.
*/

#ifndef __GNUC__
# error "expecting __GNUC__ to be defined (are you using gcc?)"
#endif

#ifndef __x86_64__
# error "expecting __x86_64__ be defined"
#endif

#ifndef __ELF__
# error "expecting ELF file format" /* i.e. for 'gottpoff', 'tpoff', etc. */
#endif

.globl __ht_yield
	.type	__ht_yield, @function
__ht_yield:
#ifdef __PIC__
	movq	__ht_stack@GOTTPOFF(%rip), %rax
	movq	%fs:(%rax), %rsp
	movq	__ht_context@GOTTPOFF(%rip), %rdi
	addq	%fs:0, %rdi
	jmp	setcontext@PLT
	hlt
#else
	movq	%fs:__ht_stack@TPOFF, %rsp
	movq	%fs:0, %rdi
	addq	$__ht_context@TPOFF, %rdi
	jmp	setcontext
	hlt
#endif
