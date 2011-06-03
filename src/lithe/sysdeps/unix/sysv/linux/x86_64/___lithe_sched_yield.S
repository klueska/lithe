/*
 * When performing an enter or yield we need to move onto the
 * transition context and stack. This is analogous to a system call
 * in which we swap to the kernel stack.
*/

#ifndef __GNUC__
#error "expecting __GNUC__ to be defined (are you using gcc?)"
#endif

#ifndef __x86_64__
# error "expecting __x86_64__ be defined"
#endif

#ifndef __ELF__
# error "expecting ELF file format" /* i.e. for 'gottpoff', 'tpoff', etc. */
#endif
	
.globl ___lithe_sched_yield
	.type	___lithe_sched_yield, @function
___lithe_sched_yield:
#ifdef __PIC__
	movq	trampoline@GOTTPOFF(%rip), %rdx
	movq	task@GOTTPOFF(%rip), %rax
	movq	%rdx, %rcx
	addq	%fs:0, %rcx
	movq	%rcx, %fs:(%rax)
	movq	%fs:32(%rdx), %rax
	addq	%fs:16(%rdx), %rax
	subq	$16, %rax
	mov	%rax, %rsp
	call	__lithe_sched_yield@PLT
	hlt
#else
	movq	%fs:0, %rax
	addq	$trampoline@TPOFF, %rax
	movq	%rax, %fs:task@TPOFF
	movq	%fs:trampoline@TPOFF+32, %rax
	addq	%fs:trampoline@TPOFF+16, %rax
	subq	$16, %rax
	mov	%rax, %rsp
	call	__lithe_sched_yield
	hlt
#endif
