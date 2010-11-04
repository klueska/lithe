/*
 * When yielding a pthread based hard thread we need to swap in a stack
 * in order to swap to our saved cleanup context. This is analogous to a
 * system call in which we swap to the kernel stack. Note that we require
 * at least __ht_stack and __ht_context to use the "initial-exec" TLS model.
*/

#ifndef __GNUC__
# error "expecting __GNUC__ to be defined (are you using gcc?)"
#endif

#ifndef __ELF__
# error "expecting ELF file format" /* i.e. for 'gottpoff', 'tpoff', etc. */
#endif


.globl __ht_yield
	.type	__ht_yield, @function
__ht_yield:
#ifdef __x86_64__
# ifdef __PIC__
	movq	__ht_stack@GOTTPOFF(%rip), %rax
	movq	%fs:(%rax), %rsp
	movq	__ht_context@GOTTPOFF(%rip), %rdi
	addq	%fs:0, %rdi
	jmp	setcontext@PLT
	hlt
# else
	movq	%fs:__ht_stack@TPOFF, %rsp
	movq	%fs:0, %rdi
	addq	$__ht_context@TPOFF, %rdi
	jmp	setcontext
	hlt
# endif
#elif __i386__
# ifdef __PIC__
	mov	__ht_stack@GOTTPOFF(%eip), %eax
	mov	%gs:(%eax), %esp
	mov	__ht_context@GOTTPOFF(%eip), %edi
	add	%gs:0, %edi
	jmp	setcontext@PLT
	hlt
# else
	mov	%gs:__ht_stack@TPOFF, %esp
	mov	%gs:0, %edi
	add	$__ht_context@TPOFF, %edi
	jmp	setcontext
	hlt
# endif
#else
# error "missing implementations for your architecture"
#endif





