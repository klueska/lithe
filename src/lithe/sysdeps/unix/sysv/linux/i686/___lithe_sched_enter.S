/*
 * When performing an enter or yield we need to move onto the
 * transition context and stack. This is analogous to a system call
 * in which we swap to the kernel stack.
*/

#ifndef __GNUC__
#error "expecting __GNUC__ to be defined (are you using gcc?)"
#endif

#ifndef __ELF__
# error "expecting ELF file format" /* i.e. for 'gottpoff', 'tpoff', etc. */
#endif

.globl ___lithe_sched_enter
	.type	___lithe_sched_enter, @function
___lithe_sched_enter:
#ifdef __x86_64__
# ifdef __PIC__
	movq	trampoline@GOTTPOFF(%rip), %rdx
	movq	task@GOTTPOFF(%rip), %rax
	movq	%rdx, %rcx
	addq	%fs:0, %rcx
	movq	%rcx, %fs:(%rax)
	movq	%fs:32(%rdx), %rax
	addq	%fs:16(%rdx), %rax
	subq	$16, %rax
	mov	%rax, %rsp
	call	__lithe_sched_enter@PLT
	hlt
# else
	movq	%fs:0, %rax
	addq	$trampoline@TPOFF, %rax
	movq	%rax, %fs:task@TPOFF
	movq	%fs:trampoline@TPOFF+32, %rax
	addq	%fs:trampoline@TPOFF+16, %rax
	subq	$16, %rax
	mov	%rax, %rsp
	call	__lithe_sched_enter
	hlt
# endif
#elif __i386__
# ifdef __PIC__
	mov	trampoline@GOTTPOFF(%eip), %edx
	mov	task@GOTTPOFF(%eip), %eax
	mov	%edx, %ecx
	add	%gs:0, %ecx
	mov	%ecx, %gs:(%eax)
	mov	%gs:32(%edx), %eax
	add	%gs:16(%edx), %eax
	sub	$16, %eax
	mov	%eax, %esp
	call	__lithe_sched_enter@PLT
	hlt
# else
	mov	%gs:0, %eax
	add	$trampoline@TPOFF, %eax
	mov	%eax, %gs:task@TPOFF
	mov	%gs:trampoline@TPOFF+32, %eax
	add	%gs:trampoline@TPOFF+16, %eax
	sub	$16, %eax
	mov	%eax, %esp
	call	__lithe_sched_enter
	hlt
# endif
#else
# error "missing implementations for your architecture"
#endif
