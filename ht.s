	.file	"ht.c"
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.text
.Ltext0:
	.p2align 4,,15
.globl ht_id
	.type	ht_id, @function
ht_id:
.LFB40:
	.file 1 "ht.c"
	.loc 1 277 0
	.loc 1 277 0
	movl	%fs:__ht_id@TPOFF, %eax
	.loc 1 279 0
	ret
.LFE40:
	.size	ht_id, .-ht_id
	.p2align 4,,15
.globl ht_num_hard_threads
	.type	ht_num_hard_threads, @function
ht_num_hard_threads:
.LFB41:
	.loc 1 283 0
	.loc 1 283 0
	movl	__ht_num_hard_threads(%rip), %eax
	.loc 1 285 0
	ret
.LFE41:
	.size	ht_num_hard_threads, .-ht_num_hard_threads
	.p2align 4,,15
.globl ht_max_hard_threads
	.type	ht_max_hard_threads, @function
ht_max_hard_threads:
.LFB42:
	.loc 1 289 0
	.loc 1 289 0
	movl	__ht_max_hard_threads(%rip), %eax
	.loc 1 291 0
	ret
.LFE42:
	.size	ht_max_hard_threads, .-ht_max_hard_threads
	.p2align 4,,15
.globl ht_limit_hard_threads
	.type	ht_limit_hard_threads, @function
ht_limit_hard_threads:
.LFB43:
	.loc 1 295 0
	.loc 1 295 0
	movl	__ht_limit_hard_threads(%rip), %eax
	.loc 1 297 0
	ret
.LFE43:
	.size	ht_limit_hard_threads, .-ht_limit_hard_threads
	.p2align 4,,15
.globl ht_request_async
	.type	ht_request_async, @function
ht_request_async:
.LFB39:
	.loc 1 246 0
.LVL0:
	pushq	%rbx
.LCFI0:
	.loc 1 246 0
	movl	%edi, %ebx
	.loc 1 247 0
	movl	$__ht_mutex, %edi
.LVL1:
	call	pthread_mutex_lock
	.loc 1 250 0
	testl	%ebx, %ebx
	jle	.L18
.LBB2:
	.loc 1 257 0
	movl	__ht_max_hard_threads(%rip), %eax
	.loc 1 261 0
	movl	__ht_limit_hard_threads(%rip), %edx
	.loc 1 264 0
	movl	$__ht_condition, %edi
	.loc 1 261 0
	subl	%eax, %edx
	cmpl	%edx, %ebx
	cmovle	%ebx, %edx
	addl	%edx, %eax
	movl	%eax, __ht_max_hard_threads(%rip)
	.loc 1 264 0
	call	pthread_cond_signal
	testl	%eax, %eax
	jne	.L19
.L12:
.LBE2:
	.loc 1 270 0
	movl	$__ht_mutex, %edi
	call	pthread_mutex_unlock
	.loc 1 273 0
	popq	%rbx
.LVL2:
	.loc 1 272 0
	movl	__ht_max_hard_threads(%rip), %eax
	.loc 1 273 0
	ret
.LVL3:
	.p2align 4,,7
.L18:
	.loc 1 251 0
	movl	__ht_max_hard_threads(%rip), %eax
	.loc 1 252 0
	movl	__ht_num_hard_threads(%rip), %edx
	.loc 1 251 0
	subl	%ebx, %eax
	.loc 1 252 0
	cmpl	%edx, %eax
	.loc 1 251 0
	movl	%eax, __ht_max_hard_threads(%rip)
	.loc 1 252 0
	jge	.L12
	.loc 1 253 0
	movl	%edx, __ht_max_hard_threads(%rip)
	jmp	.L12
.L19:
.LBB3:
	.loc 1 265 0
	movl	$__ht_mutex, %edi
	call	pthread_mutex_unlock
.LBE3:
	.loc 1 273 0
	popq	%rbx
.LVL4:
.LBB4:
	.loc 1 265 0
	movl	$-1, %eax
.LBE4:
	.loc 1 273 0
	ret
.LFE39:
	.size	ht_request_async, .-ht_request_async
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC0:
	.string	"ht: futex_wakeup_all failed"
	.text
	.p2align 4,,15
.globl futex_wakeup_all
	.type	futex_wakeup_all, @function
futex_wakeup_all:
.LFB34:
	.file 2 "futex.h"
	.loc 2 34 0
.LVL5:
	subq	$8, %rsp
.LCFI1:
	.loc 2 35 0
	xorl	%r9d, %r9d
	xorl	%r8d, %r8d
	movq	%rdi, %rsi
	xorl	%eax, %eax
	movl	$2147483647, %ecx
	movl	$1, %edx
	movl	$202, %edi
.LVL6:
	movl	$0, (%rsp)
	call	syscall
.LVL7:
	.loc 2 36 0
	testl	%eax, %eax
	js	.L24
	.loc 2 40 0
	addq	$8, %rsp
	ret
.L24:
	.loc 2 37 0
	movq	stderr(%rip), %rcx
	movl	$.LC0, %edi
	movl	$27, %edx
	movl	$1, %esi
	call	fwrite
	.loc 2 38 0
	movl	$1, %edi
	call	exit
.LFE34:
	.size	futex_wakeup_all, .-futex_wakeup_all
	.section	.rodata.str1.1
.LC1:
	.string	"ht: futex_wakeup_on failed"
	.text
	.p2align 4,,15
.globl futex_wakeup_one
	.type	futex_wakeup_one, @function
futex_wakeup_one:
.LFB33:
	.loc 2 24 0
.LVL8:
	subq	$8, %rsp
.LCFI2:
	.loc 2 25 0
	xorl	%r9d, %r9d
	xorl	%r8d, %r8d
	movq	%rdi, %rsi
	xorl	%eax, %eax
	movl	$1, %ecx
	movl	$1, %edx
	movl	$202, %edi
.LVL9:
	movl	$0, (%rsp)
	call	syscall
.LVL10:
	.loc 2 26 0
	cmpl	$1, %eax
	ja	.L29
	.loc 2 30 0
	addq	$8, %rsp
	ret
.L29:
	.loc 2 27 0
	movq	stderr(%rip), %rcx
	movl	$.LC1, %edi
	movl	$26, %edx
	movl	$1, %esi
	call	fwrite
	.loc 2 28 0
	movl	$1, %edi
	call	exit
.LFE33:
	.size	futex_wakeup_one, .-futex_wakeup_one
	.section	.rodata.str1.1
.LC2:
	.string	"ht: futex_wait failed"
	.text
	.p2align 4,,15
.globl futex_wait
	.type	futex_wait, @function
futex_wait:
.LFB32:
	.loc 2 13 0
.LVL11:
	subq	$8, %rsp
.LCFI3:
	.loc 2 14 0
	movl	%esi, %ecx
	xorl	%edx, %edx
	xorl	%r9d, %r9d
	xorl	%r8d, %r8d
	movq	%rdi, %rsi
.LVL12:
	xorl	%eax, %eax
	movl	$202, %edi
.LVL13:
	movl	$0, (%rsp)
	call	syscall
.LVL14:
.LVL15:
	.loc 2 15 0
	testl	%eax, %eax
.LVL16:
	.loc 2 14 0
	movl	%eax, %ecx
.LVL17:
	.loc 2 15 0
	setne	%dl
	cmpl	$11, %eax
	setne	%al
	testb	%al, %dl
	je	.L35
	incl	%ecx
.LVL18:
	jne	.L33
	call	__errno_location
	movl	(%rax), %eax
	cmpl	$11, %eax
	setne	%dl
	cmpl	$4, %eax
	setne	%al
	testb	%al, %dl
	je	.L35
.L33:
	.loc 2 17 0
	movq	stderr(%rip), %rcx
	movl	$.LC2, %edi
	movl	$21, %edx
	movl	$1, %esi
	call	fwrite
	.loc 2 18 0
	movl	$1, %edi
	call	exit
.LVL19:
	.p2align 4,,7
.L35:
	.loc 2 20 0
	addq	$8, %rsp
	ret
.LFE32:
	.size	futex_wait, .-futex_wait
	.section	.rodata.str1.8,"aMS",@progbits,1
	.align 8
.LC3:
	.string	"ht: could not set affinity of underlying pthread\n"
	.align 8
.LC4:
	.string	"ht: could not allocate underlying pthread\n"
	.text
	.p2align 4,,15
.globl __ht_allocate
	.type	__ht_allocate, @function
__ht_allocate:
.LFB37:
	.loc 1 184 0
.LVL20:
	pushq	%r15
.LCFI4:
	movl	%edi, %r15d
	pushq	%r14
.LCFI5:
.LBB11:
	.loc 1 185 0
	movl	%edi, %r14d
.LVL21:
.LBE11:
	.loc 1 184 0
	pushq	%r13
.LCFI6:
	pushq	%r12
.LCFI7:
	pushq	%rbp
.LCFI8:
	pushq	%rbx
.LCFI9:
	subq	$216, %rsp
.LCFI10:
.LBB12:
	.loc 1 185 0
	testl	%edi, %edi
	jle	.L37
.LVL22:
	.p2align 4,,7
.L39:
.LBB13:
	.loc 1 186 0
	movl	__ht_limit_hard_threads(%rip), %edx
	testl	%edx, %edx
	jle	.L49
	.loc 1 187 0
	movq	__ht_threads(%rip), %rsi
	xorl	%ebp, %ebp
	xorl	%ebx, %ebx
	xorl	%r12d, %r12d
	cmpb	$0, 4(%rsi)
	je	.L42
.LVL23:
	.loc 1 212 0
	cmpb	$0, 5(%rsi)
	leaq	16(%rsi), %rax
	movb	$1, %bl
	jne	.L50
	jmp	.L51
	.p2align 4,,7
.L40:
.LBE13:
	.loc 1 185 0
	movq	%rbx, %r12
	movq	%rax, %rsi
	salq	$4, %r12
.LBB14:
	.loc 1 187 0
	cmpb	$0, 4(%rax)
	je	.L42
.LBB15:
	.loc 1 211 0
	addq	$16, %rax
	incq	%rbx
.LBE15:
	.loc 1 212 0
	cmpb	$0, 5(%rsi)
	je	.L51
.L50:
	.loc 1 186 0
	incl	%ebp
	cmpl	%edx, %ebp
	jne	.L40
.LVL24:
.L49:
.LBE14:
	.loc 1 185 0
	decl	%r14d
	jne	.L39
.L37:
.LBE12:
	.loc 1 221 0
	addq	$216, %rsp
	movl	%r15d, %eax
	popq	%rbx
	popq	%rbp
.LVL25:
	popq	%r12
	popq	%r13
	popq	%r14
.LVL26:
	popq	%r15
.LVL27:
	ret
.LVL28:
.L42:
.LBB16:
.LBB17:
.LBB18:
	.loc 1 189 0
	leaq	144(%rsp), %rdi
.LVL29:
	leaq	16(%rsp), %r13
	call	pthread_attr_init
	xorl	%eax, %eax
	.p2align 4,,7
.L43:
.LBB19:
	.loc 1 191 0
	movq	$0, (%r13,%rax,8)
	incq	%rax
	cmpq	$16, %rax
	jne	.L43
.LBE19:
	.loc 1 192 0
	movl	%ebp, %ecx
	shrq	$6, %rbx
	movb	$1, %al
	andl	$63, %ecx
	salq	%cl, %rax
	orq	%rax, 16(%rsp,%rbx,8)
	.loc 1 193 0
	call	__errno_location
	leaq	144(%rsp), %rdi
	movq	%r13, %rdx
	movl	$128, %esi
	movq	%rax, %rbx
	call	pthread_attr_setaffinity_np
	testl	%eax, %eax
	movl	%eax, (%rbx)
	jne	.L62
	.loc 1 200 0
	movq	%r12, %rdi
	addq	__ht_threads(%rip), %rdi
	leaq	144(%rsp), %rsi
	movslq	%ebp,%rcx
	movl	$__ht_entry_trampoline, %edx
	addq	$8, %rdi
	call	pthread_create
	testl	%eax, %eax
	movl	%eax, (%rbx)
	jne	.L63
	.loc 1 208 0
	movq	%r12, %rax
	addq	__ht_threads(%rip), %rax
	.loc 1 210 0
	decl	%r15d
.LBE18:
.LBE17:
	.loc 1 185 0
	decl	%r14d
.LBB20:
.LBB21:
	.loc 1 208 0
	movb	$1, 4(%rax)
	.loc 1 209 0
	movb	$1, 5(%rax)
.LBE21:
.LBE20:
	.loc 1 185 0
	jne	.L39
	jmp	.L37
.LVL30:
	.p2align 4,,7
.L51:
.LBB22:
	.loc 1 213 0
	movb	$1, 5(%rsi)
.LBB23:
	.loc 2 25 0
	xorl	%r9d, %r9d
	xorl	%r8d, %r8d
	xorl	%eax, %eax
	movl	$1, %ecx
	movl	$1, %edx
	movl	$202, %edi
.LVL31:
	movl	$0, (%rsp)
	call	syscall
.LBB24:
	.loc 2 26 0
	cmpl	$1, %eax
	ja	.L64
.LBE24:
.LBE23:
	.loc 1 215 0
	decl	%r15d
.LBE22:
	.loc 1 185 0
	decl	%r14d
	jne	.L39
	.p2align 4,,5
	jmp	.L37
.L62:
.LBB25:
.LBB26:
	.loc 1 196 0
	movq	stderr(%rip), %rcx
	movl	$.LC3, %edi
	movl	$49, %edx
	movl	$1, %esi
	call	fwrite
	.loc 1 197 0
	movl	$1, %edi
	call	exit
.L63:
	.loc 1 204 0
	movq	stderr(%rip), %rcx
	movl	$.LC4, %edi
	movl	$42, %edx
	movl	$1, %esi
	call	fwrite
	.loc 1 205 0
	movl	$1, %edi
	call	exit
.L64:
.LBE26:
.LBB27:
.LBB28:
	.loc 2 27 0
	movq	stderr(%rip), %rcx
	movl	$.LC1, %edi
	movl	$26, %edx
	movl	$1, %esi
	call	fwrite
	.loc 2 28 0
	movl	$1, %edi
	call	exit
.LBE28:
.LBE27:
.LBE25:
.LBE16:
.LFE37:
	.size	__ht_allocate, .-__ht_allocate
	.section	.rodata.str1.8
	.align 8
.LC5:
	.string	"ht: failed to initialize hard threads\n"
	.text
	.p2align 4,,15
.globl main
	.type	main, @function
main:
.LFB44:
	.loc 1 301 0
.LVL32:
	pushq	%rbx
.LCFI11:
	subq	$224, %rsp
.LCFI12:
	.loc 1 301 0
	movl	%edi, 12(%rsp)
	movq	%rsi, (%rsp)
	.loc 1 304 0
	call	get_nprocs
.LVL33:
	.loc 1 306 0
	movslq	%eax,%rdi
	.loc 1 304 0
	movl	%eax, __ht_limit_hard_threads(%rip)
	.loc 1 306 0
	salq	$4, %rdi
	call	malloc
	.loc 1 308 0
	testq	%rax, %rax
	.loc 1 306 0
	movq	%rax, __ht_threads(%rip)
	.loc 1 308 0
	je	.L66
.LBB29:
	.loc 1 314 0
	movl	__ht_limit_hard_threads(%rip), %esi
.LBE29:
	.loc 1 310 0
	xorl	%ecx, %ecx
.LVL34:
	xorl	%edx, %edx
.LBB30:
	.loc 1 314 0
	testl	%esi, %esi
	jle	.L69
.L70:
	incl	%ecx
	.loc 1 315 0
	movl	$0, (%rdx,%rax)
	.loc 1 316 0
	movb	$0, 4(%rdx,%rax)
	.loc 1 317 0
	movb	$0, 5(%rdx,%rax)
	.loc 1 314 0
	addq	$16, %rdx
	cmpl	%esi, %ecx
	jne	.L70
.L69:
.LBE30:
	.loc 1 321 0
	leaq	12(%rsp), %rax
.LBB31:
	.loc 1 327 0
	leaq	144(%rsp), %rbx
.LBE31:
	.loc 1 324 0
	movl	$__ht_mutex, %edi
	.loc 1 321 0
	movq	%rsp, 216(%rsp)
	movq	%rax, 208(%rsp)
	.loc 1 324 0
	call	pthread_mutex_lock
.LVL35:
.LBB32:
	.loc 1 327 0
	movq	%rbx, %rdi
	call	pthread_attr_init
	leaq	16(%rsp), %rdx
	xorl	%eax, %eax
.L71:
.LBB33:
	.loc 1 329 0
	movq	$0, (%rdx,%rax,8)
	incq	%rax
	cmpq	$16, %rax
	jne	.L71
.LBE33:
	.loc 1 332 0
	movl	$128, %esi
	movq	%rbx, %rdi
	.loc 1 330 0
	orq	$1, 16(%rsp)
	.loc 1 332 0
	call	pthread_attr_setaffinity_np
	testl	%eax, %eax
	jne	.L73
	movq	__ht_threads(%rip), %rdi
	leaq	208(%rsp), %rcx
	movl	$__ht_main_trampoline, %edx
	movq	%rbx, %rsi
	addq	$8, %rdi
	call	pthread_create
	testl	%eax, %eax
	jne	.L73
	.loc 1 337 0
	movq	__ht_threads(%rip), %rax
	.loc 1 345 0
	movl	$1, __ht_num_hard_threads(%rip)
	.loc 1 346 0
	movl	$1, __ht_max_hard_threads(%rip)
	.loc 1 337 0
	movb	$1, 4(%rax)
	.loc 1 338 0
	movb	$1, 5(%rax)
	.p2align 4,,7
.L76:
.LBB34:
	.loc 1 350 0
	movl	$__ht_mutex, %esi
	movl	$__ht_condition, %edi
	call	pthread_cond_wait
	.loc 1 353 0
	movl	__ht_max_hard_threads(%rip), %edi
	subl	__ht_num_hard_threads(%rip), %edi
	.loc 1 354 0
	call	__ht_allocate
.LVL36:
	.loc 1 357 0
	addl	%eax, __ht_num_hard_threads(%rip)
	jmp	.L76
.LVL37:
.L73:
.LBE34:
	.loc 1 340 0
	movq	stderr(%rip), %rcx
	movl	$.LC4, %edi
	movl	$42, %edx
	movl	$1, %esi
	call	fwrite
	.loc 1 341 0
	movl	$1, %edi
	call	exit
.L66:
.LBE32:
	.loc 1 309 0
	movq	stderr(%rip), %rcx
	movl	$.LC5, %edi
	movl	$38, %edx
	movl	$1, %esi
	call	fwrite
	.loc 1 310 0
	movl	$1, %edi
	call	exit
.LFE44:
	.size	main, .-main
	.p2align 4,,15
.globl ht_request
	.type	ht_request, @function
ht_request:
.LFB38:
	.loc 1 225 0
.LVL38:
	pushq	%rbx
.LCFI13:
	.loc 1 225 0
	movl	%edi, %ebx
	.loc 1 226 0
	movl	$__ht_mutex, %edi
.LVL39:
	call	pthread_mutex_lock
.LBB35:
	.loc 1 230 0
	movl	__ht_limit_hard_threads(%rip), %edi
.LVL40:
	subl	__ht_max_hard_threads(%rip), %edi
	cmpl	%ebx, %edi
	cmovg	%ebx, %edi
	.loc 1 233 0
	call	__ht_allocate
.LVL41:
.LBE35:
	.loc 1 239 0
	movl	$__ht_mutex, %edi
.LBB36:
	.loc 1 233 0
	movl	%eax, %ebx
.LVL42:
	.loc 1 236 0
	addl	%eax, __ht_max_hard_threads(%rip)
	.loc 1 237 0
	addl	%eax, __ht_num_hard_threads(%rip)
.LBE36:
	.loc 1 239 0
	call	pthread_mutex_unlock
	.loc 1 242 0
	movl	%ebx, %eax
	popq	%rbx
.LVL43:
	ret
.LFE38:
	.size	ht_request, .-ht_request
	.section	.rodata.str1.8
	.align 8
.LC6:
	.string	"ht: could not save context for yielding\n"
	.text
	.p2align 4,,15
.globl __ht_main_trampoline
	.type	__ht_main_trampoline, @function
__ht_main_trampoline:
.LFB36:
	.loc 1 132 0
.LVL44:
	pushq	%rbp
.LCFI14:
	movq	%rsp, %rbp
.LCFI15:
	subq	$32, %rsp
.LCFI16:
	.loc 1 134 0
	movl	$0, %fs:__ht_id@TPOFF
	.loc 1 132 0
	movq	%rdi, -24(%rbp)
	.loc 1 142 0
	call	getpagesize
.LVL45:
	cltq
	addq	$30, %rax
	andq	$-16, %rax
	subq	%rax, %rsp
	leaq	23(%rsp), %rax
	andq	$-16, %rax
	movq	%rax, %fs:__ht_stack@TPOFF
.LVL46:
	.p2align 4,,7
.L98:
.LBB40:
	.loc 1 153 0
	movq	%fs:0, %rdi
	.loc 1 151 0
	movb	$0, -1(%rbp)
	.loc 1 153 0
	addq	$__ht_context@TPOFF, %rdi
	call	getcontext
.LVL47:
	testl	%eax, %eax
	js	.L99
	.loc 1 158 0
	movzbl	-1(%rbp), %eax
	testb	%al, %al
	je	.L100
	.loc 1 164 0
	movl	$__ht_mutex, %edi
	call	pthread_mutex_lock
	.loc 1 167 0
	movslq	%fs:__ht_id@TPOFF,%rax
	movq	__ht_threads(%rip), %rdx
	.loc 1 173 0
	movl	$__ht_mutex, %edi
	.loc 1 170 0
	decl	__ht_num_hard_threads(%rip)
	.loc 1 171 0
	decl	__ht_max_hard_threads(%rip)
	.loc 1 167 0
	salq	$4, %rax
	movb	$0, 5(%rax,%rdx)
	.loc 1 173 0
	call	pthread_mutex_unlock
.LBB41:
	.loc 2 14 0
	movslq	%fs:__ht_id@TPOFF,%rsi
	xorl	%ecx, %ecx
	xorl	%edx, %edx
	xorl	%r9d, %r9d
	xorl	%r8d, %r8d
	xorl	%eax, %eax
	movl	$202, %edi
	movl	$0, (%rsp)
	salq	$4, %rsi
	addq	__ht_threads(%rip), %rsi
	call	syscall
.LVL48:
.LBB42:
	.loc 2 15 0
	testl	%eax, %eax
.LVL49:
	.loc 2 14 0
	movl	%eax, %ecx
.LVL50:
	.loc 2 15 0
	setne	%dl
	cmpl	$11, %eax
	setne	%al
	testb	%al, %dl
	je	.L98
	incl	%ecx
.LVL51:
	jne	.L90
.LBE42:
	call	__errno_location
.LBB43:
	movl	(%rax), %eax
	cmpl	$11, %eax
	setne	%dl
	cmpl	$4, %eax
	setne	%al
	testb	%al, %dl
	je	.L98
.L90:
	.loc 2 17 0
	movq	stderr(%rip), %rcx
	movl	$.LC2, %edi
	movl	$21, %edx
	movl	$1, %esi
	call	fwrite
	.loc 2 18 0
	movl	$1, %edi
	call	exit
.L99:
.LBE43:
.LBE41:
	.loc 1 154 0
	movq	stderr(%rip), %rcx
	movl	$.LC6, %edi
	movl	$40, %edx
	movl	$1, %esi
	call	fwrite
	.loc 1 155 0
	movl	$1, %edi
	call	exit
.L100:
	.loc 1 161 0
	movq	-24(%rbp), %rdx
	.loc 1 159 0
	movb	$1, -1(%rbp)
	.loc 1 161 0
	movq	8(%rdx), %rax
	movq	(%rdx), %rdx
	movq	(%rax), %rsi
	movl	(%rdx), %edi
	call	__main
	movl	%eax, %edi
	call	exit
.LBE40:
.LFE36:
	.size	__ht_main_trampoline, .-__ht_main_trampoline
	.section	.rodata.str1.1
.LC7:
	.string	"ht: pthread_detach failed\n"
	.text
	.p2align 4,,15
.globl __ht_entry_trampoline
	.type	__ht_entry_trampoline, @function
__ht_entry_trampoline:
.LFB35:
	.loc 1 69 0
.LVL52:
	pushq	%rbp
.LCFI17:
	movq	%rsp, %rbp
.LCFI18:
	subq	$32, %rsp
.LCFI19:
	.loc 1 71 0
	movl	%edi, %fs:__ht_id@TPOFF
	.loc 1 82 0
	call	__errno_location
.LVL53:
	movq	%rax, -24(%rbp)
	call	pthread_self
	movq	%rax, %rdi
	call	pthread_detach
	movq	-24(%rbp), %rdx
	testl	%eax, %eax
	movl	%eax, (%rdx)
	jne	.L119
	.loc 1 91 0
	call	getpagesize
	cltq
	addq	$30, %rax
	andq	$-16, %rax
	subq	%rax, %rsp
	leaq	23(%rsp), %rax
	andq	$-16, %rax
	movq	%rax, %fs:__ht_stack@TPOFF
.LVL54:
	.p2align 4,,7
.L118:
.LBB47:
	.loc 1 102 0
	movq	%fs:0, %rdi
	.loc 1 100 0
	movb	$0, -1(%rbp)
	.loc 1 102 0
	addq	$__ht_context@TPOFF, %rdi
	call	getcontext
.LVL55:
	testl	%eax, %eax
	js	.L120
	.loc 1 107 0
	movzbl	-1(%rbp), %eax
	testb	%al, %al
	je	.L121
.L107:
	.loc 1 112 0
	movl	$__ht_mutex, %edi
	call	pthread_mutex_lock
	.loc 1 115 0
	movslq	%fs:__ht_id@TPOFF,%rax
	movq	__ht_threads(%rip), %rdx
	.loc 1 121 0
	movl	$__ht_mutex, %edi
	.loc 1 118 0
	decl	__ht_num_hard_threads(%rip)
	.loc 1 119 0
	decl	__ht_max_hard_threads(%rip)
	.loc 1 115 0
	salq	$4, %rax
	movb	$0, 5(%rax,%rdx)
	.loc 1 121 0
	call	pthread_mutex_unlock
.LBB48:
	.loc 2 14 0
	movslq	%fs:__ht_id@TPOFF,%rsi
	xorl	%ecx, %ecx
	xorl	%edx, %edx
	xorl	%r9d, %r9d
	xorl	%r8d, %r8d
	xorl	%eax, %eax
	movl	$202, %edi
	movl	$0, (%rsp)
	salq	$4, %rsi
	addq	__ht_threads(%rip), %rsi
	call	syscall
.LVL56:
.LBB49:
	.loc 2 15 0
	testl	%eax, %eax
.LVL57:
	.loc 2 14 0
	movl	%eax, %ecx
.LVL58:
	.loc 2 15 0
	setne	%dl
	cmpl	$11, %eax
	setne	%al
	testb	%al, %dl
	je	.L118
	incl	%ecx
.LVL59:
	jne	.L110
	movq	-24(%rbp), %rdx
	movl	(%rdx), %eax
	cmpl	$11, %eax
	setne	%dl
	cmpl	$4, %eax
	setne	%al
	testb	%al, %dl
	je	.L118
.L110:
	.loc 2 17 0
	movq	stderr(%rip), %rcx
	movl	$.LC2, %edi
	movl	$21, %edx
	movl	$1, %esi
	call	fwrite
	.loc 2 18 0
	movl	$1, %edi
	call	exit
	.p2align 4,,7
.L121:
.LBE49:
.LBE48:
	.loc 1 108 0
	movb	$1, -1(%rbp)
	.loc 1 109 0
	call	entry
	jmp	.L107
.L120:
	.loc 1 103 0
	movq	stderr(%rip), %rcx
	movl	$.LC6, %edi
	movl	$40, %edx
	movl	$1, %esi
	call	fwrite
	.loc 1 104 0
	movl	$1, %edi
	call	exit
.LVL60:
.L119:
.LBE47:
	.loc 1 83 0
	movq	stderr(%rip), %rcx
	movl	$.LC7, %edi
	movl	$26, %edx
	movl	$1, %esi
	call	fwrite
	.loc 1 84 0
	movl	$1, %edi
	call	exit
.LFE35:
	.size	__ht_entry_trampoline, .-__ht_entry_trampoline
.globl __ht_id
	.section	.tdata,"awT",@progbits
	.align 4
	.type	__ht_id, @object
	.size	__ht_id, 4
__ht_id:
	.long	-1
.globl __ht_context
	.section	.tbss,"awT",@nobits
	.align 32
	.type	__ht_context, @object
	.size	__ht_context, 936
__ht_context:
	.zero	936
.globl __ht_stack
	.align 8
	.type	__ht_stack, @object
	.size	__ht_stack, 8
__ht_stack:
	.zero	8
	.local	__ht_limit_hard_threads
	.comm	__ht_limit_hard_threads,4,4
	.local	__ht_threads
	.comm	__ht_threads,8,8
	.local	__ht_mutex
	.comm	__ht_mutex,40,32
	.local	__ht_num_hard_threads
	.comm	__ht_num_hard_threads,4,4
	.local	__ht_max_hard_threads
	.comm	__ht_max_hard_threads,4,4
	.local	__ht_condition
	.comm	__ht_condition,48,32
	.section	.debug_frame,"",@progbits
.Lframe0:
	.long	.LECIE0-.LSCIE0
.LSCIE0:
	.long	0xffffffff
	.byte	0x1
	.string	""
	.uleb128 0x1
	.sleb128 -8
	.byte	0x10
	.byte	0xc
	.uleb128 0x7
	.uleb128 0x8
	.byte	0x90
	.uleb128 0x1
	.align 8
.LECIE0:
.LSFDE0:
	.long	.LEFDE0-.LASFDE0
.LASFDE0:
	.long	.Lframe0
	.quad	.LFB40
	.quad	.LFE40-.LFB40
	.align 8
.LEFDE0:
.LSFDE2:
	.long	.LEFDE2-.LASFDE2
.LASFDE2:
	.long	.Lframe0
	.quad	.LFB41
	.quad	.LFE41-.LFB41
	.align 8
.LEFDE2:
.LSFDE4:
	.long	.LEFDE4-.LASFDE4
.LASFDE4:
	.long	.Lframe0
	.quad	.LFB42
	.quad	.LFE42-.LFB42
	.align 8
.LEFDE4:
.LSFDE6:
	.long	.LEFDE6-.LASFDE6
.LASFDE6:
	.long	.Lframe0
	.quad	.LFB43
	.quad	.LFE43-.LFB43
	.align 8
.LEFDE6:
.LSFDE8:
	.long	.LEFDE8-.LASFDE8
.LASFDE8:
	.long	.Lframe0
	.quad	.LFB39
	.quad	.LFE39-.LFB39
	.byte	0x4
	.long	.LCFI0-.LFB39
	.byte	0xe
	.uleb128 0x10
	.byte	0x83
	.uleb128 0x2
	.align 8
.LEFDE8:
.LSFDE10:
	.long	.LEFDE10-.LASFDE10
.LASFDE10:
	.long	.Lframe0
	.quad	.LFB34
	.quad	.LFE34-.LFB34
	.byte	0x4
	.long	.LCFI1-.LFB34
	.byte	0xe
	.uleb128 0x10
	.align 8
.LEFDE10:
.LSFDE12:
	.long	.LEFDE12-.LASFDE12
.LASFDE12:
	.long	.Lframe0
	.quad	.LFB33
	.quad	.LFE33-.LFB33
	.byte	0x4
	.long	.LCFI2-.LFB33
	.byte	0xe
	.uleb128 0x10
	.align 8
.LEFDE12:
.LSFDE14:
	.long	.LEFDE14-.LASFDE14
.LASFDE14:
	.long	.Lframe0
	.quad	.LFB32
	.quad	.LFE32-.LFB32
	.byte	0x4
	.long	.LCFI3-.LFB32
	.byte	0xe
	.uleb128 0x10
	.align 8
.LEFDE14:
.LSFDE16:
	.long	.LEFDE16-.LASFDE16
.LASFDE16:
	.long	.Lframe0
	.quad	.LFB37
	.quad	.LFE37-.LFB37
	.byte	0x4
	.long	.LCFI4-.LFB37
	.byte	0xe
	.uleb128 0x10
	.byte	0x8f
	.uleb128 0x2
	.byte	0x4
	.long	.LCFI5-.LCFI4
	.byte	0xe
	.uleb128 0x18
	.byte	0x8e
	.uleb128 0x3
	.byte	0x4
	.long	.LCFI6-.LCFI5
	.byte	0xe
	.uleb128 0x20
	.byte	0x4
	.long	.LCFI7-.LCFI6
	.byte	0xe
	.uleb128 0x28
	.byte	0x4
	.long	.LCFI8-.LCFI7
	.byte	0xe
	.uleb128 0x30
	.byte	0x4
	.long	.LCFI9-.LCFI8
	.byte	0xe
	.uleb128 0x38
	.byte	0x4
	.long	.LCFI10-.LCFI9
	.byte	0xe
	.uleb128 0x110
	.byte	0x83
	.uleb128 0x7
	.byte	0x86
	.uleb128 0x6
	.byte	0x8c
	.uleb128 0x5
	.byte	0x8d
	.uleb128 0x4
	.align 8
.LEFDE16:
.LSFDE18:
	.long	.LEFDE18-.LASFDE18
.LASFDE18:
	.long	.Lframe0
	.quad	.LFB44
	.quad	.LFE44-.LFB44
	.byte	0x4
	.long	.LCFI11-.LFB44
	.byte	0xe
	.uleb128 0x10
	.byte	0x4
	.long	.LCFI12-.LCFI11
	.byte	0xe
	.uleb128 0xf0
	.byte	0x83
	.uleb128 0x2
	.align 8
.LEFDE18:
.LSFDE20:
	.long	.LEFDE20-.LASFDE20
.LASFDE20:
	.long	.Lframe0
	.quad	.LFB38
	.quad	.LFE38-.LFB38
	.byte	0x4
	.long	.LCFI13-.LFB38
	.byte	0xe
	.uleb128 0x10
	.byte	0x83
	.uleb128 0x2
	.align 8
.LEFDE20:
.LSFDE22:
	.long	.LEFDE22-.LASFDE22
.LASFDE22:
	.long	.Lframe0
	.quad	.LFB36
	.quad	.LFE36-.LFB36
	.byte	0x4
	.long	.LCFI14-.LFB36
	.byte	0xe
	.uleb128 0x10
	.byte	0x86
	.uleb128 0x2
	.byte	0x4
	.long	.LCFI15-.LCFI14
	.byte	0xd
	.uleb128 0x6
	.align 8
.LEFDE22:
.LSFDE24:
	.long	.LEFDE24-.LASFDE24
.LASFDE24:
	.long	.Lframe0
	.quad	.LFB35
	.quad	.LFE35-.LFB35
	.byte	0x4
	.long	.LCFI17-.LFB35
	.byte	0xe
	.uleb128 0x10
	.byte	0x86
	.uleb128 0x2
	.byte	0x4
	.long	.LCFI18-.LCFI17
	.byte	0xd
	.uleb128 0x6
	.align 8
.LEFDE24:
	.section	.eh_frame,"a",@progbits
.Lframe1:
	.long	.LECIE1-.LSCIE1
.LSCIE1:
	.long	0x0
	.byte	0x1
	.string	"zR"
	.uleb128 0x1
	.sleb128 -8
	.byte	0x10
	.uleb128 0x1
	.byte	0x3
	.byte	0xc
	.uleb128 0x7
	.uleb128 0x8
	.byte	0x90
	.uleb128 0x1
	.align 8
.LECIE1:
.LSFDE1:
	.long	.LEFDE1-.LASFDE1
.LASFDE1:
	.long	.LASFDE1-.Lframe1
	.long	.LFB40
	.long	.LFE40-.LFB40
	.uleb128 0x0
	.align 8
.LEFDE1:
.LSFDE3:
	.long	.LEFDE3-.LASFDE3
.LASFDE3:
	.long	.LASFDE3-.Lframe1
	.long	.LFB41
	.long	.LFE41-.LFB41
	.uleb128 0x0
	.align 8
.LEFDE3:
.LSFDE5:
	.long	.LEFDE5-.LASFDE5
.LASFDE5:
	.long	.LASFDE5-.Lframe1
	.long	.LFB42
	.long	.LFE42-.LFB42
	.uleb128 0x0
	.align 8
.LEFDE5:
.LSFDE7:
	.long	.LEFDE7-.LASFDE7
.LASFDE7:
	.long	.LASFDE7-.Lframe1
	.long	.LFB43
	.long	.LFE43-.LFB43
	.uleb128 0x0
	.align 8
.LEFDE7:
.LSFDE9:
	.long	.LEFDE9-.LASFDE9
.LASFDE9:
	.long	.LASFDE9-.Lframe1
	.long	.LFB39
	.long	.LFE39-.LFB39
	.uleb128 0x0
	.byte	0x4
	.long	.LCFI0-.LFB39
	.byte	0xe
	.uleb128 0x10
	.byte	0x83
	.uleb128 0x2
	.align 8
.LEFDE9:
.LSFDE11:
	.long	.LEFDE11-.LASFDE11
.LASFDE11:
	.long	.LASFDE11-.Lframe1
	.long	.LFB34
	.long	.LFE34-.LFB34
	.uleb128 0x0
	.byte	0x4
	.long	.LCFI1-.LFB34
	.byte	0xe
	.uleb128 0x10
	.align 8
.LEFDE11:
.LSFDE13:
	.long	.LEFDE13-.LASFDE13
.LASFDE13:
	.long	.LASFDE13-.Lframe1
	.long	.LFB33
	.long	.LFE33-.LFB33
	.uleb128 0x0
	.byte	0x4
	.long	.LCFI2-.LFB33
	.byte	0xe
	.uleb128 0x10
	.align 8
.LEFDE13:
.LSFDE15:
	.long	.LEFDE15-.LASFDE15
.LASFDE15:
	.long	.LASFDE15-.Lframe1
	.long	.LFB32
	.long	.LFE32-.LFB32
	.uleb128 0x0
	.byte	0x4
	.long	.LCFI3-.LFB32
	.byte	0xe
	.uleb128 0x10
	.align 8
.LEFDE15:
.LSFDE17:
	.long	.LEFDE17-.LASFDE17
.LASFDE17:
	.long	.LASFDE17-.Lframe1
	.long	.LFB37
	.long	.LFE37-.LFB37
	.uleb128 0x0
	.byte	0x4
	.long	.LCFI4-.LFB37
	.byte	0xe
	.uleb128 0x10
	.byte	0x8f
	.uleb128 0x2
	.byte	0x4
	.long	.LCFI5-.LCFI4
	.byte	0xe
	.uleb128 0x18
	.byte	0x8e
	.uleb128 0x3
	.byte	0x4
	.long	.LCFI6-.LCFI5
	.byte	0xe
	.uleb128 0x20
	.byte	0x4
	.long	.LCFI7-.LCFI6
	.byte	0xe
	.uleb128 0x28
	.byte	0x4
	.long	.LCFI8-.LCFI7
	.byte	0xe
	.uleb128 0x30
	.byte	0x4
	.long	.LCFI9-.LCFI8
	.byte	0xe
	.uleb128 0x38
	.byte	0x4
	.long	.LCFI10-.LCFI9
	.byte	0xe
	.uleb128 0x110
	.byte	0x83
	.uleb128 0x7
	.byte	0x86
	.uleb128 0x6
	.byte	0x8c
	.uleb128 0x5
	.byte	0x8d
	.uleb128 0x4
	.align 8
.LEFDE17:
.LSFDE19:
	.long	.LEFDE19-.LASFDE19
.LASFDE19:
	.long	.LASFDE19-.Lframe1
	.long	.LFB44
	.long	.LFE44-.LFB44
	.uleb128 0x0
	.byte	0x4
	.long	.LCFI11-.LFB44
	.byte	0xe
	.uleb128 0x10
	.byte	0x4
	.long	.LCFI12-.LCFI11
	.byte	0xe
	.uleb128 0xf0
	.byte	0x83
	.uleb128 0x2
	.align 8
.LEFDE19:
.LSFDE21:
	.long	.LEFDE21-.LASFDE21
.LASFDE21:
	.long	.LASFDE21-.Lframe1
	.long	.LFB38
	.long	.LFE38-.LFB38
	.uleb128 0x0
	.byte	0x4
	.long	.LCFI13-.LFB38
	.byte	0xe
	.uleb128 0x10
	.byte	0x83
	.uleb128 0x2
	.align 8
.LEFDE21:
.LSFDE23:
	.long	.LEFDE23-.LASFDE23
.LASFDE23:
	.long	.LASFDE23-.Lframe1
	.long	.LFB36
	.long	.LFE36-.LFB36
	.uleb128 0x0
	.byte	0x4
	.long	.LCFI14-.LFB36
	.byte	0xe
	.uleb128 0x10
	.byte	0x86
	.uleb128 0x2
	.byte	0x4
	.long	.LCFI15-.LCFI14
	.byte	0xd
	.uleb128 0x6
	.align 8
.LEFDE23:
.LSFDE25:
	.long	.LEFDE25-.LASFDE25
.LASFDE25:
	.long	.LASFDE25-.Lframe1
	.long	.LFB35
	.long	.LFE35-.LFB35
	.uleb128 0x0
	.byte	0x4
	.long	.LCFI17-.LFB35
	.byte	0xe
	.uleb128 0x10
	.byte	0x86
	.uleb128 0x2
	.byte	0x4
	.long	.LCFI18-.LCFI17
	.byte	0xd
	.uleb128 0x6
	.align 8
.LEFDE25:
	.file 3 "/usr/include/bits/pthreadtypes.h"
	.file 4 "/usr/include/bits/sched.h"
	.file 5 "/usr/include/stdio.h"
	.file 6 "/usr/include/libio.h"
	.file 7 "/usr/include/bits/types.h"
	.file 8 "/usr/include/sys/ucontext.h"
	.file 9 "/usr/include/bits/sigstack.h"
	.file 10 "/usr/lib/gcc/x86_64-linux-gnu/4.1.2/include/stddef.h"
	.file 11 "/usr/include/bits/sigset.h"
	.text
.Letext0:
	.section	.debug_loc,"",@progbits
.Ldebug_loc0:
.LLST4:
	.quad	.LFB39-.Ltext0
	.quad	.LCFI0-.Ltext0
	.value	0x2
	.byte	0x77
	.sleb128 8
	.quad	.LCFI0-.Ltext0
	.quad	.LFE39-.Ltext0
	.value	0x2
	.byte	0x77
	.sleb128 16
	.quad	0x0
	.quad	0x0
.LLST5:
	.quad	.LVL0-.Ltext0
	.quad	.LVL1-.Ltext0
	.value	0x1
	.byte	0x55
	.quad	.LVL1-.Ltext0
	.quad	.LVL2-.Ltext0
	.value	0x1
	.byte	0x53
	.quad	.LVL3-.Ltext0
	.quad	.LVL4-.Ltext0
	.value	0x1
	.byte	0x53
	.quad	0x0
	.quad	0x0
.LLST6:
	.quad	.LFB34-.Ltext0
	.quad	.LCFI1-.Ltext0
	.value	0x2
	.byte	0x77
	.sleb128 8
	.quad	.LCFI1-.Ltext0
	.quad	.LFE34-.Ltext0
	.value	0x2
	.byte	0x77
	.sleb128 16
	.quad	0x0
	.quad	0x0
.LLST7:
	.quad	.LVL5-.Ltext0
	.quad	.LVL6-.Ltext0
	.value	0x1
	.byte	0x55
	.quad	.LVL6-.Ltext0
	.quad	.LVL7-.Ltext0
	.value	0x1
	.byte	0x54
	.quad	0x0
	.quad	0x0
.LLST8:
	.quad	.LFB33-.Ltext0
	.quad	.LCFI2-.Ltext0
	.value	0x2
	.byte	0x77
	.sleb128 8
	.quad	.LCFI2-.Ltext0
	.quad	.LFE33-.Ltext0
	.value	0x2
	.byte	0x77
	.sleb128 16
	.quad	0x0
	.quad	0x0
.LLST9:
	.quad	.LVL8-.Ltext0
	.quad	.LVL9-.Ltext0
	.value	0x1
	.byte	0x55
	.quad	.LVL9-.Ltext0
	.quad	.LVL10-.Ltext0
	.value	0x1
	.byte	0x54
	.quad	0x0
	.quad	0x0
.LLST10:
	.quad	.LFB32-.Ltext0
	.quad	.LCFI3-.Ltext0
	.value	0x2
	.byte	0x77
	.sleb128 8
	.quad	.LCFI3-.Ltext0
	.quad	.LFE32-.Ltext0
	.value	0x2
	.byte	0x77
	.sleb128 16
	.quad	0x0
	.quad	0x0
.LLST11:
	.quad	.LVL11-.Ltext0
	.quad	.LVL13-.Ltext0
	.value	0x1
	.byte	0x55
	.quad	.LVL13-.Ltext0
	.quad	.LVL14-.Ltext0
	.value	0x1
	.byte	0x54
	.quad	0x0
	.quad	0x0
.LLST12:
	.quad	.LVL11-.Ltext0
	.quad	.LVL12-.Ltext0
	.value	0x1
	.byte	0x54
	.quad	.LVL12-.Ltext0
	.quad	.LVL14-.Ltext0
	.value	0x1
	.byte	0x52
	.quad	0x0
	.quad	0x0
.LLST13:
	.quad	.LVL15-.Ltext0
	.quad	.LVL16-.Ltext0
	.value	0x1
	.byte	0x50
	.quad	.LVL17-.Ltext0
	.quad	.LVL18-.Ltext0
	.value	0x1
	.byte	0x52
	.quad	.LVL19-.Ltext0
	.quad	.LFE32-.Ltext0
	.value	0x1
	.byte	0x52
	.quad	0x0
	.quad	0x0
.LLST14:
	.quad	.LFB37-.Ltext0
	.quad	.LCFI4-.Ltext0
	.value	0x2
	.byte	0x77
	.sleb128 8
	.quad	.LCFI4-.Ltext0
	.quad	.LCFI5-.Ltext0
	.value	0x2
	.byte	0x77
	.sleb128 16
	.quad	.LCFI5-.Ltext0
	.quad	.LCFI6-.Ltext0
	.value	0x2
	.byte	0x77
	.sleb128 24
	.quad	.LCFI6-.Ltext0
	.quad	.LCFI7-.Ltext0
	.value	0x2
	.byte	0x77
	.sleb128 32
	.quad	.LCFI7-.Ltext0
	.quad	.LCFI8-.Ltext0
	.value	0x2
	.byte	0x77
	.sleb128 40
	.quad	.LCFI8-.Ltext0
	.quad	.LCFI9-.Ltext0
	.value	0x2
	.byte	0x77
	.sleb128 48
	.quad	.LCFI9-.Ltext0
	.quad	.LCFI10-.Ltext0
	.value	0x2
	.byte	0x77
	.sleb128 56
	.quad	.LCFI10-.Ltext0
	.quad	.LFE37-.Ltext0
	.value	0x3
	.byte	0x77
	.sleb128 272
	.quad	0x0
	.quad	0x0
.LLST15:
	.quad	.LVL20-.Ltext0
	.quad	.LVL22-.Ltext0
	.value	0x1
	.byte	0x55
	.quad	.LVL22-.Ltext0
	.quad	.LVL23-.Ltext0
	.value	0x1
	.byte	0x5f
	.quad	.LVL23-.Ltext0
	.quad	.LVL24-.Ltext0
	.value	0x1
	.byte	0x55
	.quad	.LVL24-.Ltext0
	.quad	.LVL27-.Ltext0
	.value	0x1
	.byte	0x5f
	.quad	.LVL27-.Ltext0
	.quad	.LVL29-.Ltext0
	.value	0x1
	.byte	0x55
	.quad	.LVL29-.Ltext0
	.quad	.LVL30-.Ltext0
	.value	0x1
	.byte	0x5f
	.quad	.LVL30-.Ltext0
	.quad	.LVL31-.Ltext0
	.value	0x1
	.byte	0x55
	.quad	.LVL31-.Ltext0
	.quad	.LFE37-.Ltext0
	.value	0x1
	.byte	0x5f
	.quad	0x0
	.quad	0x0
.LLST16:
	.quad	.LVL21-.Ltext0
	.quad	.LVL26-.Ltext0
	.value	0x1
	.byte	0x5e
	.quad	.LVL28-.Ltext0
	.quad	.LFE37-.Ltext0
	.value	0x1
	.byte	0x5e
	.quad	0x0
	.quad	0x0
.LLST17:
	.quad	.LVL22-.Ltext0
	.quad	.LVL25-.Ltext0
	.value	0x1
	.byte	0x56
	.quad	.LVL28-.Ltext0
	.quad	.LFE37-.Ltext0
	.value	0x1
	.byte	0x56
	.quad	0x0
	.quad	0x0
.LLST18:
	.quad	.LFB44-.Ltext0
	.quad	.LCFI11-.Ltext0
	.value	0x2
	.byte	0x77
	.sleb128 8
	.quad	.LCFI11-.Ltext0
	.quad	.LCFI12-.Ltext0
	.value	0x2
	.byte	0x77
	.sleb128 16
	.quad	.LCFI12-.Ltext0
	.quad	.LFE44-.Ltext0
	.value	0x3
	.byte	0x77
	.sleb128 240
	.quad	0x0
	.quad	0x0
.LLST19:
	.quad	.LVL32-.Ltext0
	.quad	.LVL33-.Ltext0
	.value	0x1
	.byte	0x55
	.quad	.LVL33-.Ltext0
	.quad	.LFE44-.Ltext0
	.value	0x3
	.byte	0x91
	.sleb128 -228
	.quad	0x0
	.quad	0x0
.LLST20:
	.quad	.LVL32-.Ltext0
	.quad	.LVL33-.Ltext0
	.value	0x1
	.byte	0x54
	.quad	.LVL33-.Ltext0
	.quad	.LFE44-.Ltext0
	.value	0x3
	.byte	0x91
	.sleb128 -240
	.quad	0x0
	.quad	0x0
.LLST21:
	.quad	.LVL34-.Ltext0
	.quad	.LVL35-.Ltext0
	.value	0x1
	.byte	0x52
	.quad	0x0
	.quad	0x0
.LLST22:
	.quad	.LVL36-.Ltext0
	.quad	.LVL37-.Ltext0
	.value	0x1
	.byte	0x50
	.quad	0x0
	.quad	0x0
.LLST23:
	.quad	.LFB38-.Ltext0
	.quad	.LCFI13-.Ltext0
	.value	0x2
	.byte	0x77
	.sleb128 8
	.quad	.LCFI13-.Ltext0
	.quad	.LFE38-.Ltext0
	.value	0x2
	.byte	0x77
	.sleb128 16
	.quad	0x0
	.quad	0x0
.LLST24:
	.quad	.LVL38-.Ltext0
	.quad	.LVL39-.Ltext0
	.value	0x1
	.byte	0x55
	.quad	.LVL39-.Ltext0
	.quad	.LVL40-.Ltext0
	.value	0x1
	.byte	0x53
	.quad	.LVL40-.Ltext0
	.quad	.LVL41-.Ltext0
	.value	0x1
	.byte	0x55
	.quad	.LVL42-.Ltext0
	.quad	.LVL42-.Ltext0
	.value	0x1
	.byte	0x53
	.quad	0x0
	.quad	0x0
.LLST25:
	.quad	.LFB36-.Ltext0
	.quad	.LCFI14-.Ltext0
	.value	0x2
	.byte	0x77
	.sleb128 8
	.quad	.LCFI14-.Ltext0
	.quad	.LCFI15-.Ltext0
	.value	0x2
	.byte	0x77
	.sleb128 16
	.quad	.LCFI15-.Ltext0
	.quad	.LFE36-.Ltext0
	.value	0x2
	.byte	0x76
	.sleb128 16
	.quad	0x0
	.quad	0x0
.LLST26:
	.quad	.LVL44-.Ltext0
	.quad	.LVL45-.Ltext0
	.value	0x1
	.byte	0x55
	.quad	.LVL45-.Ltext0
	.quad	.LFE36-.Ltext0
	.value	0x2
	.byte	0x76
	.sleb128 -24
	.quad	0x0
	.quad	0x0
.LLST27:
	.quad	.LVL46-.Ltext0
	.quad	.LVL47-.Ltext0
	.value	0x1
	.byte	0x52
	.quad	.LVL48-.Ltext0
	.quad	.LVL49-.Ltext0
	.value	0x1
	.byte	0x50
	.quad	.LVL50-.Ltext0
	.quad	.LVL51-.Ltext0
	.value	0x1
	.byte	0x52
	.quad	0x0
	.quad	0x0
.LLST28:
	.quad	.LFB35-.Ltext0
	.quad	.LCFI17-.Ltext0
	.value	0x2
	.byte	0x77
	.sleb128 8
	.quad	.LCFI17-.Ltext0
	.quad	.LCFI18-.Ltext0
	.value	0x2
	.byte	0x77
	.sleb128 16
	.quad	.LCFI18-.Ltext0
	.quad	.LFE35-.Ltext0
	.value	0x2
	.byte	0x76
	.sleb128 16
	.quad	0x0
	.quad	0x0
.LLST29:
	.quad	.LVL52-.Ltext0
	.quad	.LVL53-.Ltext0
	.value	0x1
	.byte	0x55
	.quad	0x0
	.quad	0x0
.LLST30:
	.quad	.LVL54-.Ltext0
	.quad	.LVL60-.Ltext0
	.value	0x2
	.byte	0x76
	.sleb128 -1
	.quad	0x0
	.quad	0x0
.LLST31:
	.quad	.LVL54-.Ltext0
	.quad	.LVL55-.Ltext0
	.value	0x1
	.byte	0x52
	.quad	.LVL56-.Ltext0
	.quad	.LVL57-.Ltext0
	.value	0x1
	.byte	0x50
	.quad	.LVL58-.Ltext0
	.quad	.LVL59-.Ltext0
	.value	0x1
	.byte	0x52
	.quad	0x0
	.quad	0x0
	.section	.debug_info
	.long	0xdb6
	.value	0x2
	.long	.Ldebug_abbrev0
	.byte	0x8
	.uleb128 0x1
	.long	.Ldebug_line0
	.quad	.Letext0
	.quad	.Ltext0
	.long	.LASF142
	.byte	0x1
	.long	.LASF143
	.long	.LASF144
	.uleb128 0x2
	.string	"int"
	.byte	0x4
	.byte	0x5
	.uleb128 0x3
	.long	.LASF6
	.byte	0xa
	.byte	0xd6
	.long	0x3f
	.uleb128 0x4
	.long	.LASF0
	.byte	0x8
	.byte	0x7
	.uleb128 0x4
	.long	.LASF1
	.byte	0x1
	.byte	0x8
	.uleb128 0x4
	.long	.LASF2
	.byte	0x2
	.byte	0x7
	.uleb128 0x4
	.long	.LASF3
	.byte	0x4
	.byte	0x7
	.uleb128 0x4
	.long	.LASF4
	.byte	0x1
	.byte	0x6
	.uleb128 0x4
	.long	.LASF5
	.byte	0x2
	.byte	0x5
	.uleb128 0x3
	.long	.LASF7
	.byte	0x7
	.byte	0x2b
	.long	0x4d
	.uleb128 0x3
	.long	.LASF8
	.byte	0x7
	.byte	0x2d
	.long	0x54
	.uleb128 0x4
	.long	.LASF9
	.byte	0x8
	.byte	0x5
	.uleb128 0x3
	.long	.LASF10
	.byte	0x7
	.byte	0x30
	.long	0x3f
	.uleb128 0x3
	.long	.LASF11
	.byte	0x7
	.byte	0x8f
	.long	0x7f
	.uleb128 0x3
	.long	.LASF12
	.byte	0x7
	.byte	0x90
	.long	0x7f
	.uleb128 0x4
	.long	.LASF0
	.byte	0x8
	.byte	0x7
	.uleb128 0x5
	.byte	0x8
	.uleb128 0x6
	.byte	0x8
	.long	0xb6
	.uleb128 0x4
	.long	.LASF13
	.byte	0x1
	.byte	0x6
	.uleb128 0x3
	.long	.LASF14
	.byte	0x4
	.byte	0x62
	.long	0x3f
	.uleb128 0x7
	.long	0xdf
	.byte	0x80
	.byte	0x4
	.byte	0x6a
	.uleb128 0x8
	.long	.LASF16
	.byte	0x4
	.byte	0x6b
	.long	0xdf
	.byte	0x2
	.byte	0x23
	.uleb128 0x0
	.byte	0x0
	.uleb128 0x9
	.long	0xef
	.long	0xbd
	.uleb128 0xa
	.long	0xa7
	.byte	0xf
	.byte	0x0
	.uleb128 0x3
	.long	.LASF15
	.byte	0x4
	.byte	0x6c
	.long	0xc8
	.uleb128 0x7
	.long	0x111
	.byte	0x80
	.byte	0xb
	.byte	0x1d
	.uleb128 0x8
	.long	.LASF17
	.byte	0xb
	.byte	0x1e
	.long	0x111
	.byte	0x2
	.byte	0x23
	.uleb128 0x0
	.byte	0x0
	.uleb128 0x9
	.long	0x121
	.long	0x3f
	.uleb128 0xa
	.long	0xa7
	.byte	0xf
	.byte	0x0
	.uleb128 0x3
	.long	.LASF18
	.byte	0xb
	.byte	0x1f
	.long	0xfa
	.uleb128 0x3
	.long	.LASF19
	.byte	0x3
	.byte	0x32
	.long	0x3f
	.uleb128 0xb
	.long	0x156
	.byte	0x38
	.byte	0x3
	.byte	0x36
	.uleb128 0xc
	.long	.LASF20
	.byte	0x3
	.byte	0x37
	.long	0x156
	.uleb128 0xc
	.long	.LASF21
	.byte	0x3
	.byte	0x38
	.long	0x7f
	.byte	0x0
	.uleb128 0x9
	.long	0x166
	.long	0xb6
	.uleb128 0xa
	.long	0xa7
	.byte	0x37
	.byte	0x0
	.uleb128 0x3
	.long	.LASF22
	.byte	0x3
	.byte	0x39
	.long	0x137
	.uleb128 0x7
	.long	0x1ce
	.byte	0x18
	.byte	0x3
	.byte	0x41
	.uleb128 0x8
	.long	.LASF23
	.byte	0x3
	.byte	0x42
	.long	0x2d
	.byte	0x2
	.byte	0x23
	.uleb128 0x0
	.uleb128 0x8
	.long	.LASF24
	.byte	0x3
	.byte	0x43
	.long	0x54
	.byte	0x2
	.byte	0x23
	.uleb128 0x4
	.uleb128 0x8
	.long	.LASF25
	.byte	0x3
	.byte	0x44
	.long	0x2d
	.byte	0x2
	.byte	0x23
	.uleb128 0x8
	.uleb128 0x8
	.long	.LASF26
	.byte	0x3
	.byte	0x46
	.long	0x54
	.byte	0x2
	.byte	0x23
	.uleb128 0xc
	.uleb128 0x8
	.long	.LASF27
	.byte	0x3
	.byte	0x4a
	.long	0x2d
	.byte	0x2
	.byte	0x23
	.uleb128 0x10
	.uleb128 0x8
	.long	.LASF28
	.byte	0x3
	.byte	0x4e
	.long	0x2d
	.byte	0x2
	.byte	0x23
	.uleb128 0x14
	.byte	0x0
	.uleb128 0xb
	.long	0x1f8
	.byte	0x28
	.byte	0x3
	.byte	0x3f
	.uleb128 0xc
	.long	.LASF29
	.byte	0x3
	.byte	0x4f
	.long	0x171
	.uleb128 0xc
	.long	.LASF20
	.byte	0x3
	.byte	0x50
	.long	0x1f8
	.uleb128 0xc
	.long	.LASF21
	.byte	0x3
	.byte	0x51
	.long	0x7f
	.byte	0x0
	.uleb128 0x9
	.long	0x208
	.long	0xb6
	.uleb128 0xa
	.long	0xa7
	.byte	0x27
	.byte	0x0
	.uleb128 0x3
	.long	.LASF30
	.byte	0x3
	.byte	0x52
	.long	0x1ce
	.uleb128 0x7
	.long	0x28c
	.byte	0x30
	.byte	0x3
	.byte	0x60
	.uleb128 0x8
	.long	.LASF23
	.byte	0x3
	.byte	0x61
	.long	0x2d
	.byte	0x2
	.byte	0x23
	.uleb128 0x0
	.uleb128 0x8
	.long	.LASF31
	.byte	0x3
	.byte	0x62
	.long	0x54
	.byte	0x2
	.byte	0x23
	.uleb128 0x4
	.uleb128 0x8
	.long	.LASF32
	.byte	0x3
	.byte	0x63
	.long	0x28c
	.byte	0x2
	.byte	0x23
	.uleb128 0x8
	.uleb128 0x8
	.long	.LASF33
	.byte	0x3
	.byte	0x64
	.long	0x28c
	.byte	0x2
	.byte	0x23
	.uleb128 0x10
	.uleb128 0x8
	.long	.LASF34
	.byte	0x3
	.byte	0x65
	.long	0x28c
	.byte	0x2
	.byte	0x23
	.uleb128 0x18
	.uleb128 0x8
	.long	.LASF35
	.byte	0x3
	.byte	0x66
	.long	0xae
	.byte	0x2
	.byte	0x23
	.uleb128 0x20
	.uleb128 0x8
	.long	.LASF36
	.byte	0x3
	.byte	0x67
	.long	0x54
	.byte	0x2
	.byte	0x23
	.uleb128 0x28
	.uleb128 0x8
	.long	.LASF37
	.byte	0x3
	.byte	0x68
	.long	0x54
	.byte	0x2
	.byte	0x23
	.uleb128 0x2c
	.byte	0x0
	.uleb128 0x4
	.long	.LASF38
	.byte	0x8
	.byte	0x7
	.uleb128 0xb
	.long	0x2bd
	.byte	0x30
	.byte	0x3
	.byte	0x5e
	.uleb128 0xc
	.long	.LASF29
	.byte	0x3
	.byte	0x69
	.long	0x213
	.uleb128 0xc
	.long	.LASF20
	.byte	0x3
	.byte	0x6a
	.long	0x2bd
	.uleb128 0xc
	.long	.LASF21
	.byte	0x3
	.byte	0x6b
	.long	0x2cd
	.byte	0x0
	.uleb128 0x9
	.long	0x2cd
	.long	0xb6
	.uleb128 0xa
	.long	0xa7
	.byte	0x2f
	.byte	0x0
	.uleb128 0x4
	.long	.LASF39
	.byte	0x8
	.byte	0x5
	.uleb128 0x3
	.long	.LASF40
	.byte	0x3
	.byte	0x6c
	.long	0x293
	.uleb128 0xd
	.long	0x47b
	.long	.LASF68
	.byte	0xd8
	.byte	0x5
	.byte	0x2e
	.uleb128 0xe
	.long	.LASF41
	.byte	0x6
	.value	0x10c
	.long	0x2d
	.byte	0x2
	.byte	0x23
	.uleb128 0x0
	.uleb128 0xe
	.long	.LASF42
	.byte	0x6
	.value	0x111
	.long	0xb0
	.byte	0x2
	.byte	0x23
	.uleb128 0x8
	.uleb128 0xe
	.long	.LASF43
	.byte	0x6
	.value	0x112
	.long	0xb0
	.byte	0x2
	.byte	0x23
	.uleb128 0x10
	.uleb128 0xe
	.long	.LASF44
	.byte	0x6
	.value	0x113
	.long	0xb0
	.byte	0x2
	.byte	0x23
	.uleb128 0x18
	.uleb128 0xe
	.long	.LASF45
	.byte	0x6
	.value	0x114
	.long	0xb0
	.byte	0x2
	.byte	0x23
	.uleb128 0x20
	.uleb128 0xe
	.long	.LASF46
	.byte	0x6
	.value	0x115
	.long	0xb0
	.byte	0x2
	.byte	0x23
	.uleb128 0x28
	.uleb128 0xe
	.long	.LASF47
	.byte	0x6
	.value	0x116
	.long	0xb0
	.byte	0x2
	.byte	0x23
	.uleb128 0x30
	.uleb128 0xe
	.long	.LASF48
	.byte	0x6
	.value	0x117
	.long	0xb0
	.byte	0x2
	.byte	0x23
	.uleb128 0x38
	.uleb128 0xe
	.long	.LASF49
	.byte	0x6
	.value	0x118
	.long	0xb0
	.byte	0x2
	.byte	0x23
	.uleb128 0x40
	.uleb128 0xe
	.long	.LASF50
	.byte	0x6
	.value	0x11a
	.long	0xb0
	.byte	0x2
	.byte	0x23
	.uleb128 0x48
	.uleb128 0xe
	.long	.LASF51
	.byte	0x6
	.value	0x11b
	.long	0xb0
	.byte	0x2
	.byte	0x23
	.uleb128 0x50
	.uleb128 0xe
	.long	.LASF52
	.byte	0x6
	.value	0x11c
	.long	0xb0
	.byte	0x2
	.byte	0x23
	.uleb128 0x58
	.uleb128 0xe
	.long	.LASF53
	.byte	0x6
	.value	0x11e
	.long	0x4bf
	.byte	0x2
	.byte	0x23
	.uleb128 0x60
	.uleb128 0xe
	.long	.LASF54
	.byte	0x6
	.value	0x120
	.long	0x4c5
	.byte	0x2
	.byte	0x23
	.uleb128 0x68
	.uleb128 0xe
	.long	.LASF55
	.byte	0x6
	.value	0x122
	.long	0x2d
	.byte	0x2
	.byte	0x23
	.uleb128 0x70
	.uleb128 0xe
	.long	.LASF56
	.byte	0x6
	.value	0x126
	.long	0x2d
	.byte	0x2
	.byte	0x23
	.uleb128 0x74
	.uleb128 0xe
	.long	.LASF57
	.byte	0x6
	.value	0x128
	.long	0x91
	.byte	0x2
	.byte	0x23
	.uleb128 0x78
	.uleb128 0xe
	.long	.LASF58
	.byte	0x6
	.value	0x12c
	.long	0x4d
	.byte	0x3
	.byte	0x23
	.uleb128 0x80
	.uleb128 0xe
	.long	.LASF59
	.byte	0x6
	.value	0x12d
	.long	0x5b
	.byte	0x3
	.byte	0x23
	.uleb128 0x82
	.uleb128 0xe
	.long	.LASF60
	.byte	0x6
	.value	0x12e
	.long	0x4cb
	.byte	0x3
	.byte	0x23
	.uleb128 0x83
	.uleb128 0xe
	.long	.LASF61
	.byte	0x6
	.value	0x132
	.long	0x4db
	.byte	0x3
	.byte	0x23
	.uleb128 0x88
	.uleb128 0xe
	.long	.LASF62
	.byte	0x6
	.value	0x13b
	.long	0x9c
	.byte	0x3
	.byte	0x23
	.uleb128 0x90
	.uleb128 0xe
	.long	.LASF63
	.byte	0x6
	.value	0x141
	.long	0xae
	.byte	0x3
	.byte	0x23
	.uleb128 0x98
	.uleb128 0xe
	.long	.LASF64
	.byte	0x6
	.value	0x142
	.long	0xae
	.byte	0x3
	.byte	0x23
	.uleb128 0xa0
	.uleb128 0xe
	.long	.LASF65
	.byte	0x6
	.value	0x144
	.long	0x2d
	.byte	0x3
	.byte	0x23
	.uleb128 0xa8
	.uleb128 0xe
	.long	.LASF66
	.byte	0x6
	.value	0x146
	.long	0x4e1
	.byte	0x3
	.byte	0x23
	.uleb128 0xac
	.byte	0x0
	.uleb128 0x6
	.byte	0x8
	.long	0xae
	.uleb128 0xf
	.long	.LASF67
	.byte	0x6
	.byte	0xb0
	.uleb128 0xd
	.long	0x4bf
	.long	.LASF69
	.byte	0x18
	.byte	0x6
	.byte	0xb6
	.uleb128 0x8
	.long	.LASF70
	.byte	0x6
	.byte	0xb7
	.long	0x4bf
	.byte	0x2
	.byte	0x23
	.uleb128 0x0
	.uleb128 0x8
	.long	.LASF71
	.byte	0x6
	.byte	0xb8
	.long	0x4c5
	.byte	0x2
	.byte	0x23
	.uleb128 0x8
	.uleb128 0x8
	.long	.LASF72
	.byte	0x6
	.byte	0xbc
	.long	0x2d
	.byte	0x2
	.byte	0x23
	.uleb128 0x10
	.byte	0x0
	.uleb128 0x6
	.byte	0x8
	.long	0x488
	.uleb128 0x6
	.byte	0x8
	.long	0x2df
	.uleb128 0x9
	.long	0x4db
	.long	0xb6
	.uleb128 0xa
	.long	0xa7
	.byte	0x0
	.byte	0x0
	.uleb128 0x6
	.byte	0x8
	.long	0x481
	.uleb128 0x9
	.long	0x4f1
	.long	0xb6
	.uleb128 0xa
	.long	0xa7
	.byte	0x2b
	.byte	0x0
	.uleb128 0x9
	.long	0x501
	.long	0x4d
	.uleb128 0xa
	.long	0xa7
	.byte	0x2
	.byte	0x0
	.uleb128 0x9
	.long	0x511
	.long	0x4d
	.uleb128 0xa
	.long	0xa7
	.byte	0x3
	.byte	0x0
	.uleb128 0x9
	.long	0x521
	.long	0x74
	.uleb128 0xa
	.long	0xa7
	.byte	0x3
	.byte	0x0
	.uleb128 0x9
	.long	0x531
	.long	0x74
	.uleb128 0xa
	.long	0xa7
	.byte	0x17
	.byte	0x0
	.uleb128 0x9
	.long	0x541
	.long	0x3f
	.uleb128 0xa
	.long	0xa7
	.byte	0x7
	.byte	0x0
	.uleb128 0xd
	.long	0x578
	.long	.LASF73
	.byte	0x18
	.byte	0x9
	.byte	0x33
	.uleb128 0x8
	.long	.LASF74
	.byte	0x9
	.byte	0x34
	.long	0xae
	.byte	0x2
	.byte	0x23
	.uleb128 0x0
	.uleb128 0x8
	.long	.LASF75
	.byte	0x9
	.byte	0x35
	.long	0x2d
	.byte	0x2
	.byte	0x23
	.uleb128 0x8
	.uleb128 0x8
	.long	.LASF76
	.byte	0x9
	.byte	0x36
	.long	0x34
	.byte	0x2
	.byte	0x23
	.uleb128 0x10
	.byte	0x0
	.uleb128 0x3
	.long	.LASF77
	.byte	0x9
	.byte	0x37
	.long	0x541
	.uleb128 0x3
	.long	.LASF78
	.byte	0x8
	.byte	0x21
	.long	0x7f
	.uleb128 0x3
	.long	.LASF79
	.byte	0x8
	.byte	0x27
	.long	0x599
	.uleb128 0x9
	.long	0x5a9
	.long	0x583
	.uleb128 0xa
	.long	0xa7
	.byte	0x16
	.byte	0x0
	.uleb128 0xd
	.long	0x5e0
	.long	.LASF80
	.byte	0x10
	.byte	0x8
	.byte	0x5f
	.uleb128 0x8
	.long	.LASF81
	.byte	0x8
	.byte	0x60
	.long	0x501
	.byte	0x2
	.byte	0x23
	.uleb128 0x0
	.uleb128 0x8
	.long	.LASF82
	.byte	0x8
	.byte	0x61
	.long	0x4d
	.byte	0x2
	.byte	0x23
	.uleb128 0x8
	.uleb128 0x8
	.long	.LASF83
	.byte	0x8
	.byte	0x62
	.long	0x4f1
	.byte	0x2
	.byte	0x23
	.uleb128 0xa
	.byte	0x0
	.uleb128 0xd
	.long	0x5fb
	.long	.LASF84
	.byte	0x10
	.byte	0x8
	.byte	0x66
	.uleb128 0x8
	.long	.LASF85
	.byte	0x8
	.byte	0x67
	.long	0x511
	.byte	0x2
	.byte	0x23
	.uleb128 0x0
	.byte	0x0
	.uleb128 0x10
	.long	0x6a5
	.long	.LASF86
	.value	0x200
	.byte	0x8
	.byte	0x6b
	.uleb128 0x11
	.string	"cwd"
	.byte	0x8
	.byte	0x6d
	.long	0x69
	.byte	0x2
	.byte	0x23
	.uleb128 0x0
	.uleb128 0x11
	.string	"swd"
	.byte	0x8
	.byte	0x6e
	.long	0x69
	.byte	0x2
	.byte	0x23
	.uleb128 0x2
	.uleb128 0x11
	.string	"ftw"
	.byte	0x8
	.byte	0x6f
	.long	0x69
	.byte	0x2
	.byte	0x23
	.uleb128 0x4
	.uleb128 0x11
	.string	"fop"
	.byte	0x8
	.byte	0x70
	.long	0x69
	.byte	0x2
	.byte	0x23
	.uleb128 0x6
	.uleb128 0x11
	.string	"rip"
	.byte	0x8
	.byte	0x71
	.long	0x86
	.byte	0x2
	.byte	0x23
	.uleb128 0x8
	.uleb128 0x11
	.string	"rdp"
	.byte	0x8
	.byte	0x72
	.long	0x86
	.byte	0x2
	.byte	0x23
	.uleb128 0x10
	.uleb128 0x8
	.long	.LASF87
	.byte	0x8
	.byte	0x73
	.long	0x74
	.byte	0x2
	.byte	0x23
	.uleb128 0x18
	.uleb128 0x8
	.long	.LASF88
	.byte	0x8
	.byte	0x74
	.long	0x74
	.byte	0x2
	.byte	0x23
	.uleb128 0x1c
	.uleb128 0x11
	.string	"_st"
	.byte	0x8
	.byte	0x75
	.long	0x6a5
	.byte	0x2
	.byte	0x23
	.uleb128 0x20
	.uleb128 0x8
	.long	.LASF89
	.byte	0x8
	.byte	0x76
	.long	0x6b5
	.byte	0x3
	.byte	0x23
	.uleb128 0xa0
	.uleb128 0x8
	.long	.LASF83
	.byte	0x8
	.byte	0x77
	.long	0x521
	.byte	0x3
	.byte	0x23
	.uleb128 0x1a0
	.byte	0x0
	.uleb128 0x9
	.long	0x6b5
	.long	0x5a9
	.uleb128 0xa
	.long	0xa7
	.byte	0x7
	.byte	0x0
	.uleb128 0x9
	.long	0x6c5
	.long	0x5e0
	.uleb128 0xa
	.long	0xa7
	.byte	0xf
	.byte	0x0
	.uleb128 0x3
	.long	.LASF90
	.byte	0x8
	.byte	0x7b
	.long	0x6d0
	.uleb128 0x6
	.byte	0x8
	.long	0x5fb
	.uleb128 0x12
	.long	0x70c
	.value	0x100
	.byte	0x8
	.byte	0x7f
	.uleb128 0x8
	.long	.LASF91
	.byte	0x8
	.byte	0x80
	.long	0x58e
	.byte	0x2
	.byte	0x23
	.uleb128 0x0
	.uleb128 0x8
	.long	.LASF92
	.byte	0x8
	.byte	0x82
	.long	0x6c5
	.byte	0x3
	.byte	0x23
	.uleb128 0xb8
	.uleb128 0x8
	.long	.LASF93
	.byte	0x8
	.byte	0x83
	.long	0x531
	.byte	0x3
	.byte	0x23
	.uleb128 0xc0
	.byte	0x0
	.uleb128 0x3
	.long	.LASF94
	.byte	0x8
	.byte	0x84
	.long	0x6d6
	.uleb128 0x10
	.long	0x77b
	.long	.LASF95
	.value	0x3a8
	.byte	0x8
	.byte	0x88
	.uleb128 0x8
	.long	.LASF96
	.byte	0x8
	.byte	0x89
	.long	0x3f
	.byte	0x2
	.byte	0x23
	.uleb128 0x0
	.uleb128 0x8
	.long	.LASF97
	.byte	0x8
	.byte	0x8a
	.long	0x77b
	.byte	0x2
	.byte	0x23
	.uleb128 0x8
	.uleb128 0x8
	.long	.LASF98
	.byte	0x8
	.byte	0x8b
	.long	0x578
	.byte	0x2
	.byte	0x23
	.uleb128 0x10
	.uleb128 0x8
	.long	.LASF99
	.byte	0x8
	.byte	0x8c
	.long	0x70c
	.byte	0x2
	.byte	0x23
	.uleb128 0x28
	.uleb128 0x8
	.long	.LASF100
	.byte	0x8
	.byte	0x8d
	.long	0x121
	.byte	0x3
	.byte	0x23
	.uleb128 0x128
	.uleb128 0x8
	.long	.LASF101
	.byte	0x8
	.byte	0x8e
	.long	0x5fb
	.byte	0x3
	.byte	0x23
	.uleb128 0x1a8
	.byte	0x0
	.uleb128 0x6
	.byte	0x8
	.long	0x717
	.uleb128 0x3
	.long	.LASF102
	.byte	0x8
	.byte	0x8f
	.long	0x717
	.uleb128 0xd
	.long	0x7d1
	.long	.LASF103
	.byte	0x10
	.byte	0x1
	.byte	0x2a
	.uleb128 0x8
	.long	.LASF104
	.byte	0x1
	.byte	0x2b
	.long	0x2d
	.byte	0x2
	.byte	0x23
	.uleb128 0x0
	.uleb128 0x8
	.long	.LASF105
	.byte	0x1
	.byte	0x2c
	.long	0x7d1
	.byte	0x2
	.byte	0x23
	.uleb128 0x4
	.uleb128 0x8
	.long	.LASF106
	.byte	0x1
	.byte	0x2d
	.long	0x7d1
	.byte	0x2
	.byte	0x23
	.uleb128 0x5
	.uleb128 0x8
	.long	.LASF107
	.byte	0x1
	.byte	0x2e
	.long	0x12c
	.byte	0x2
	.byte	0x23
	.uleb128 0x8
	.byte	0x0
	.uleb128 0x4
	.long	.LASF108
	.byte	0x1
	.byte	0x2
	.uleb128 0x13
	.byte	0x1
	.long	.LASF109
	.byte	0x1
	.value	0x115
	.long	0x2d
	.quad	.LFB40
	.quad	.LFE40
	.byte	0x2
	.byte	0x77
	.sleb128 8
	.uleb128 0x13
	.byte	0x1
	.long	.LASF110
	.byte	0x1
	.value	0x11b
	.long	0x2d
	.quad	.LFB41
	.quad	.LFE41
	.byte	0x2
	.byte	0x77
	.sleb128 8
	.uleb128 0x13
	.byte	0x1
	.long	.LASF111
	.byte	0x1
	.value	0x121
	.long	0x2d
	.quad	.LFB42
	.quad	.LFE42
	.byte	0x2
	.byte	0x77
	.sleb128 8
	.uleb128 0x13
	.byte	0x1
	.long	.LASF112
	.byte	0x1
	.value	0x127
	.long	0x2d
	.quad	.LFB43
	.quad	.LFE43
	.byte	0x2
	.byte	0x77
	.sleb128 8
	.uleb128 0x14
	.long	0x89d
	.byte	0x1
	.long	.LASF113
	.byte	0x1
	.byte	0xf6
	.byte	0x1
	.long	0x2d
	.quad	.LFB39
	.quad	.LFE39
	.long	.LLST4
	.uleb128 0x15
	.string	"k"
	.byte	0x1
	.byte	0xf5
	.long	0x2d
	.long	.LLST5
	.uleb128 0x16
	.long	.Ldebug_ranges0+0x0
	.uleb128 0x17
	.long	.LASF115
	.byte	0x1
	.value	0x101
	.long	0x2d
	.byte	0x0
	.byte	0x0
	.uleb128 0x18
	.long	0x8d7
	.byte	0x1
	.long	.LASF114
	.byte	0x2
	.byte	0x22
	.byte	0x1
	.quad	.LFB34
	.quad	.LFE34
	.long	.LLST6
	.uleb128 0x19
	.long	.LASF104
	.byte	0x2
	.byte	0x21
	.long	0xae
	.long	.LLST7
	.uleb128 0x1a
	.string	"r"
	.byte	0x2
	.byte	0x23
	.long	0x2d
	.byte	0x0
	.uleb128 0x1b
	.long	0x8fa
	.byte	0x1
	.long	.LASF116
	.byte	0x2
	.byte	0x18
	.byte	0x1
	.byte	0x3
	.uleb128 0x1c
	.long	.LASF104
	.byte	0x2
	.byte	0x17
	.long	0xae
	.uleb128 0x1a
	.string	"r"
	.byte	0x2
	.byte	0x19
	.long	0x2d
	.byte	0x0
	.uleb128 0x1d
	.long	0x926
	.long	0x8d7
	.quad	.LFB33
	.quad	.LFE33
	.long	.LLST8
	.uleb128 0x1e
	.long	0x8e5
	.long	.LLST9
	.uleb128 0x1f
	.long	0x8f0
	.byte	0x0
	.uleb128 0x1b
	.long	0x954
	.byte	0x1
	.long	.LASF117
	.byte	0x2
	.byte	0xd
	.byte	0x1
	.byte	0x3
	.uleb128 0x1c
	.long	.LASF104
	.byte	0x2
	.byte	0xc
	.long	0xae
	.uleb128 0x1c
	.long	.LASF118
	.byte	0x2
	.byte	0xc
	.long	0x2d
	.uleb128 0x1a
	.string	"r"
	.byte	0x2
	.byte	0xe
	.long	0x2d
	.byte	0x0
	.uleb128 0x1d
	.long	0x98d
	.long	0x926
	.quad	.LFB32
	.quad	.LFE32
	.long	.LLST10
	.uleb128 0x1e
	.long	0x934
	.long	.LLST11
	.uleb128 0x1e
	.long	0x93f
	.long	.LLST12
	.uleb128 0x20
	.long	0x94a
	.long	.LLST13
	.byte	0x0
	.uleb128 0x14
	.long	0xa5c
	.byte	0x1
	.long	.LASF119
	.byte	0x1
	.byte	0xb8
	.byte	0x1
	.long	0x2d
	.quad	.LFB37
	.quad	.LFE37
	.long	.LLST14
	.uleb128 0x15
	.string	"k"
	.byte	0x1
	.byte	0xb7
	.long	0x2d
	.long	.LLST15
	.uleb128 0x21
	.long	0x9d6
	.long	.Ldebug_ranges0+0x40
	.uleb128 0x22
	.string	"j"
	.byte	0x1
	.byte	0xb9
	.long	0x2d
	.long	.LLST16
	.byte	0x0
	.uleb128 0x21
	.long	0x9ed
	.long	.Ldebug_ranges0+0x80
	.uleb128 0x22
	.string	"i"
	.byte	0x1
	.byte	0xba
	.long	0x2d
	.long	.LLST17
	.byte	0x0
	.uleb128 0x21
	.long	0xa13
	.long	.Ldebug_ranges0+0xf0
	.uleb128 0x23
	.long	.LASF120
	.byte	0x1
	.byte	0xbc
	.long	0x166
	.byte	0x3
	.byte	0x91
	.sleb128 -128
	.uleb128 0x24
	.string	"c"
	.byte	0x1
	.byte	0xbe
	.long	0xef
	.byte	0x3
	.byte	0x91
	.sleb128 -256
	.byte	0x0
	.uleb128 0x25
	.long	0xa3f
	.quad	.LBB19
	.quad	.LBE19
	.uleb128 0x1a
	.string	"__i"
	.byte	0x1
	.byte	0xbf
	.long	0x54
	.uleb128 0x26
	.long	.LASF121
	.byte	0x1
	.byte	0xbf
	.long	0xa5c
	.byte	0x0
	.uleb128 0x27
	.long	0x8d7
	.long	.Ldebug_ranges0+0x140
	.byte	0x1
	.byte	0xd6
	.uleb128 0x28
	.long	0x917
	.uleb128 0x16
	.long	.Ldebug_ranges0+0x170
	.uleb128 0x1f
	.long	0x8f0
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.uleb128 0x6
	.byte	0x8
	.long	0xef
	.uleb128 0x29
	.long	0xb46
	.byte	0x1
	.long	.LASF122
	.byte	0x1
	.value	0x12d
	.byte	0x1
	.long	0x2d
	.quad	.LFB44
	.quad	.LFE44
	.long	.LLST18
	.uleb128 0x2a
	.long	.LASF123
	.byte	0x1
	.value	0x12c
	.long	0x2d
	.long	.LLST19
	.uleb128 0x2a
	.long	.LASF124
	.byte	0x1
	.value	0x12c
	.long	0xb46
	.long	.LLST20
	.uleb128 0x2b
	.long	.LASF125
	.byte	0x1
	.value	0x141
	.long	0xb4c
	.byte	0x2
	.byte	0x91
	.sleb128 -32
	.uleb128 0x21
	.long	0xacf
	.long	.Ldebug_ranges0+0x1a0
	.uleb128 0x2c
	.string	"i"
	.byte	0x1
	.value	0x13a
	.long	0x2d
	.long	.LLST21
	.byte	0x0
	.uleb128 0x21
	.long	0xaf7
	.long	.Ldebug_ranges0+0x1d0
	.uleb128 0x2b
	.long	.LASF120
	.byte	0x1
	.value	0x146
	.long	0x166
	.byte	0x3
	.byte	0x91
	.sleb128 -96
	.uleb128 0x2d
	.string	"c"
	.byte	0x1
	.value	0x148
	.long	0xef
	.byte	0x3
	.byte	0x91
	.sleb128 -224
	.byte	0x0
	.uleb128 0x25
	.long	0xb25
	.quad	.LBB33
	.quad	.LBE33
	.uleb128 0x2e
	.string	"__i"
	.byte	0x1
	.value	0x149
	.long	0x54
	.uleb128 0x17
	.long	.LASF121
	.byte	0x1
	.value	0x149
	.long	0xa5c
	.byte	0x0
	.uleb128 0x2f
	.quad	.LBB34
	.quad	.LBE34
	.uleb128 0x2c
	.string	"k"
	.byte	0x1
	.value	0x161
	.long	0x2d
	.long	.LLST22
	.byte	0x0
	.byte	0x0
	.uleb128 0x6
	.byte	0x8
	.long	0xb0
	.uleb128 0x9
	.long	0xb5c
	.long	0xae
	.uleb128 0xa
	.long	0xa7
	.byte	0x1
	.byte	0x0
	.uleb128 0x14
	.long	0xba0
	.byte	0x1
	.long	.LASF126
	.byte	0x1
	.byte	0xe1
	.byte	0x1
	.long	0x2d
	.quad	.LFB38
	.quad	.LFE38
	.long	.LLST23
	.uleb128 0x15
	.string	"k"
	.byte	0x1
	.byte	0xe0
	.long	0x2d
	.long	.LLST24
	.uleb128 0x16
	.long	.Ldebug_ranges0+0x200
	.uleb128 0x26
	.long	.LASF115
	.byte	0x1
	.byte	0xe5
	.long	0x2d
	.byte	0x0
	.byte	0x0
	.uleb128 0x14
	.long	0xc31
	.byte	0x1
	.long	.LASF127
	.byte	0x1
	.byte	0x84
	.byte	0x1
	.long	0xae
	.quad	.LFB36
	.quad	.LFE36
	.long	.LLST25
	.uleb128 0x15
	.string	"arg"
	.byte	0x1
	.byte	0x83
	.long	0xae
	.long	.LLST26
	.uleb128 0x26
	.long	.LASF125
	.byte	0x1
	.byte	0x85
	.long	0x47b
	.uleb128 0x2f
	.quad	.LBB40
	.quad	.LBE40
	.uleb128 0x23
	.long	.LASF128
	.byte	0x1
	.byte	0x97
	.long	0xc31
	.byte	0x2
	.byte	0x76
	.sleb128 -1
	.uleb128 0x30
	.long	0x926
	.quad	.LBB41
	.quad	.LBE41
	.byte	0x1
	.byte	0xaf
	.uleb128 0x28
	.long	0x97a
	.uleb128 0x28
	.long	0x971
	.uleb128 0x16
	.long	.Ldebug_ranges0+0x230
	.uleb128 0x20
	.long	0x94a
	.long	.LLST27
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.uleb128 0x31
	.long	0x7d1
	.uleb128 0x14
	.long	0xcc9
	.byte	0x1
	.long	.LASF129
	.byte	0x1
	.byte	0x45
	.byte	0x1
	.long	0xae
	.quad	.LFB35
	.quad	.LFE35
	.long	.LLST28
	.uleb128 0x15
	.string	"arg"
	.byte	0x1
	.byte	0x44
	.long	0xae
	.long	.LLST29
	.uleb128 0x2f
	.quad	.LBB47
	.quad	.LBE47
	.uleb128 0x32
	.long	.LASF128
	.byte	0x1
	.byte	0x64
	.long	0xc31
	.long	.LLST30
	.uleb128 0x30
	.long	0x926
	.quad	.LBB48
	.quad	.LBE48
	.byte	0x1
	.byte	0x7b
	.uleb128 0x28
	.long	0x97a
	.uleb128 0x28
	.long	0x971
	.uleb128 0x2f
	.quad	.LBB49
	.quad	.LBE49
	.uleb128 0x20
	.long	0x94a
	.long	.LLST31
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.uleb128 0x23
	.long	.LASF130
	.byte	0x1
	.byte	0x24
	.long	0x208
	.byte	0x9
	.byte	0x3
	.quad	__ht_mutex
	.uleb128 0x23
	.long	.LASF131
	.byte	0x1
	.byte	0x27
	.long	0x2d4
	.byte	0x9
	.byte	0x3
	.quad	__ht_condition
	.uleb128 0x23
	.long	.LASF132
	.byte	0x1
	.byte	0x2f
	.long	0xd08
	.byte	0x9
	.byte	0x3
	.quad	__ht_threads
	.uleb128 0x6
	.byte	0x8
	.long	0x78c
	.uleb128 0x23
	.long	.LASF133
	.byte	0x1
	.byte	0x32
	.long	0x2d
	.byte	0x9
	.byte	0x3
	.quad	__ht_num_hard_threads
	.uleb128 0x23
	.long	.LASF134
	.byte	0x1
	.byte	0x35
	.long	0x2d
	.byte	0x9
	.byte	0x3
	.quad	__ht_max_hard_threads
	.uleb128 0x23
	.long	.LASF135
	.byte	0x1
	.byte	0x38
	.long	0x2d
	.byte	0x9
	.byte	0x3
	.quad	__ht_limit_hard_threads
	.uleb128 0x33
	.long	.LASF136
	.byte	0x5
	.byte	0x8e
	.long	0x4c5
	.byte	0x1
	.byte	0x1
	.uleb128 0x33
	.long	.LASF137
	.byte	0x5
	.byte	0x8f
	.long	0x4c5
	.byte	0x1
	.byte	0x1
	.uleb128 0x33
	.long	.LASF138
	.byte	0x5
	.byte	0x90
	.long	0x4c5
	.byte	0x1
	.byte	0x1
	.uleb128 0x34
	.long	.LASF139
	.byte	0x1
	.byte	0x3b
	.long	0x2d
	.byte	0x1
	.byte	0xa
	.byte	0x3
	.long	__ht_id@DTPOFF, 0
	.byte	0xe0
	.uleb128 0x34
	.long	.LASF140
	.byte	0x1
	.byte	0x3e
	.long	0x781
	.byte	0x1
	.byte	0xa
	.byte	0x3
	.long	__ht_context@DTPOFF, 0
	.byte	0xe0
	.uleb128 0x34
	.long	.LASF141
	.byte	0x1
	.byte	0x41
	.long	0xae
	.byte	0x1
	.byte	0xa
	.byte	0x3
	.long	__ht_stack@DTPOFF, 0
	.byte	0xe0
	.byte	0x0
	.section	.debug_abbrev
	.uleb128 0x1
	.uleb128 0x11
	.byte	0x1
	.uleb128 0x10
	.uleb128 0x6
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x25
	.uleb128 0xe
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x1b
	.uleb128 0xe
	.byte	0x0
	.byte	0x0
	.uleb128 0x2
	.uleb128 0x24
	.byte	0x0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.byte	0x0
	.byte	0x0
	.uleb128 0x3
	.uleb128 0x16
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x4
	.uleb128 0x24
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.byte	0x0
	.byte	0x0
	.uleb128 0x5
	.uleb128 0xf
	.byte	0x0
	.uleb128 0xb
	.uleb128 0xb
	.byte	0x0
	.byte	0x0
	.uleb128 0x6
	.uleb128 0xf
	.byte	0x0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x7
	.uleb128 0x13
	.byte	0x1
	.uleb128 0x1
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.byte	0x0
	.byte	0x0
	.uleb128 0x8
	.uleb128 0xd
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x38
	.uleb128 0xa
	.byte	0x0
	.byte	0x0
	.uleb128 0x9
	.uleb128 0x1
	.byte	0x1
	.uleb128 0x1
	.uleb128 0x13
	.uleb128 0x49
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0xa
	.uleb128 0x21
	.byte	0x0
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2f
	.uleb128 0xb
	.byte	0x0
	.byte	0x0
	.uleb128 0xb
	.uleb128 0x17
	.byte	0x1
	.uleb128 0x1
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.byte	0x0
	.byte	0x0
	.uleb128 0xc
	.uleb128 0xd
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0xd
	.uleb128 0x13
	.byte	0x1
	.uleb128 0x1
	.uleb128 0x13
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.byte	0x0
	.byte	0x0
	.uleb128 0xe
	.uleb128 0xd
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x38
	.uleb128 0xa
	.byte	0x0
	.byte	0x0
	.uleb128 0xf
	.uleb128 0x16
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.byte	0x0
	.byte	0x0
	.uleb128 0x10
	.uleb128 0x13
	.byte	0x1
	.uleb128 0x1
	.uleb128 0x13
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0xb
	.uleb128 0x5
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.byte	0x0
	.byte	0x0
	.uleb128 0x11
	.uleb128 0xd
	.byte	0x0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x38
	.uleb128 0xa
	.byte	0x0
	.byte	0x0
	.uleb128 0x12
	.uleb128 0x13
	.byte	0x1
	.uleb128 0x1
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0x5
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.byte	0x0
	.byte	0x0
	.uleb128 0x13
	.uleb128 0x2e
	.byte	0x0
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x40
	.uleb128 0xa
	.byte	0x0
	.byte	0x0
	.uleb128 0x14
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x1
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0xc
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x40
	.uleb128 0x6
	.byte	0x0
	.byte	0x0
	.uleb128 0x15
	.uleb128 0x5
	.byte	0x0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x6
	.byte	0x0
	.byte	0x0
	.uleb128 0x16
	.uleb128 0xb
	.byte	0x1
	.uleb128 0x55
	.uleb128 0x6
	.byte	0x0
	.byte	0x0
	.uleb128 0x17
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x18
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x1
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0xc
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x40
	.uleb128 0x6
	.byte	0x0
	.byte	0x0
	.uleb128 0x19
	.uleb128 0x5
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x6
	.byte	0x0
	.byte	0x0
	.uleb128 0x1a
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x1b
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x1
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0xc
	.uleb128 0x20
	.uleb128 0xb
	.byte	0x0
	.byte	0x0
	.uleb128 0x1c
	.uleb128 0x5
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x1d
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x1
	.uleb128 0x13
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x40
	.uleb128 0x6
	.byte	0x0
	.byte	0x0
	.uleb128 0x1e
	.uleb128 0x5
	.byte	0x0
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x6
	.byte	0x0
	.byte	0x0
	.uleb128 0x1f
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x31
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x20
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x6
	.byte	0x0
	.byte	0x0
	.uleb128 0x21
	.uleb128 0xb
	.byte	0x1
	.uleb128 0x1
	.uleb128 0x13
	.uleb128 0x55
	.uleb128 0x6
	.byte	0x0
	.byte	0x0
	.uleb128 0x22
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x6
	.byte	0x0
	.byte	0x0
	.uleb128 0x23
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0xa
	.byte	0x0
	.byte	0x0
	.uleb128 0x24
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0xa
	.byte	0x0
	.byte	0x0
	.uleb128 0x25
	.uleb128 0xb
	.byte	0x1
	.uleb128 0x1
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.byte	0x0
	.byte	0x0
	.uleb128 0x26
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x27
	.uleb128 0x1d
	.byte	0x1
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x55
	.uleb128 0x6
	.uleb128 0x58
	.uleb128 0xb
	.uleb128 0x59
	.uleb128 0xb
	.byte	0x0
	.byte	0x0
	.uleb128 0x28
	.uleb128 0x5
	.byte	0x0
	.uleb128 0x31
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x29
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x1
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x27
	.uleb128 0xc
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x40
	.uleb128 0x6
	.byte	0x0
	.byte	0x0
	.uleb128 0x2a
	.uleb128 0x5
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x6
	.byte	0x0
	.byte	0x0
	.uleb128 0x2b
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0xa
	.byte	0x0
	.byte	0x0
	.uleb128 0x2c
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x6
	.byte	0x0
	.byte	0x0
	.uleb128 0x2d
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0xa
	.byte	0x0
	.byte	0x0
	.uleb128 0x2e
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x2f
	.uleb128 0xb
	.byte	0x1
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.byte	0x0
	.byte	0x0
	.uleb128 0x30
	.uleb128 0x1d
	.byte	0x1
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x58
	.uleb128 0xb
	.uleb128 0x59
	.uleb128 0xb
	.byte	0x0
	.byte	0x0
	.uleb128 0x31
	.uleb128 0x35
	.byte	0x0
	.uleb128 0x49
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x32
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x6
	.byte	0x0
	.byte	0x0
	.uleb128 0x33
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3c
	.uleb128 0xc
	.byte	0x0
	.byte	0x0
	.uleb128 0x34
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x2
	.uleb128 0xa
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.section	.debug_pubnames,"",@progbits
	.long	0x139
	.value	0x2
	.long	.Ldebug_info0
	.long	0xdba
	.long	0x7d8
	.string	"ht_id"
	.long	0x7f8
	.string	"ht_num_hard_threads"
	.long	0x818
	.string	"ht_max_hard_threads"
	.long	0x838
	.string	"ht_limit_hard_threads"
	.long	0x858
	.string	"ht_request_async"
	.long	0x89d
	.string	"futex_wakeup_all"
	.long	0x8fa
	.string	"futex_wakeup_one"
	.long	0x954
	.string	"futex_wait"
	.long	0x98d
	.string	"__ht_allocate"
	.long	0xa62
	.string	"main"
	.long	0xb5c
	.string	"ht_request"
	.long	0xba0
	.string	"__ht_main_trampoline"
	.long	0xc36
	.string	"__ht_entry_trampoline"
	.long	0xd74
	.string	"__ht_id"
	.long	0xd8b
	.string	"__ht_context"
	.long	0xda2
	.string	"__ht_stack"
	.long	0x0
	.section	.debug_aranges,"",@progbits
	.long	0x2c
	.value	0x2
	.long	.Ldebug_info0
	.byte	0x8
	.byte	0x0
	.value	0x0
	.value	0x0
	.quad	.Ltext0
	.quad	.Letext0-.Ltext0
	.quad	0x0
	.quad	0x0
	.section	.debug_ranges,"",@progbits
.Ldebug_ranges0:
	.quad	.LBB2-.Ltext0
	.quad	.LBE2-.Ltext0
	.quad	.LBB4-.Ltext0
	.quad	.LBE4-.Ltext0
	.quad	.LBB3-.Ltext0
	.quad	.LBE3-.Ltext0
	.quad	0x0
	.quad	0x0
	.quad	.LBB11-.Ltext0
	.quad	.LBE11-.Ltext0
	.quad	.LBB16-.Ltext0
	.quad	.LBE16-.Ltext0
	.quad	.LBB12-.Ltext0
	.quad	.LBE12-.Ltext0
	.quad	0x0
	.quad	0x0
	.quad	.LBB13-.Ltext0
	.quad	.LBE13-.Ltext0
	.quad	.LBB25-.Ltext0
	.quad	.LBE25-.Ltext0
	.quad	.LBB22-.Ltext0
	.quad	.LBE22-.Ltext0
	.quad	.LBB20-.Ltext0
	.quad	.LBE20-.Ltext0
	.quad	.LBB17-.Ltext0
	.quad	.LBE17-.Ltext0
	.quad	.LBB14-.Ltext0
	.quad	.LBE14-.Ltext0
	.quad	0x0
	.quad	0x0
	.quad	.LBB15-.Ltext0
	.quad	.LBE15-.Ltext0
	.quad	.LBB26-.Ltext0
	.quad	.LBE26-.Ltext0
	.quad	.LBB21-.Ltext0
	.quad	.LBE21-.Ltext0
	.quad	.LBB18-.Ltext0
	.quad	.LBE18-.Ltext0
	.quad	0x0
	.quad	0x0
	.quad	.LBB23-.Ltext0
	.quad	.LBE23-.Ltext0
	.quad	.LBB27-.Ltext0
	.quad	.LBE27-.Ltext0
	.quad	0x0
	.quad	0x0
	.quad	.LBB24-.Ltext0
	.quad	.LBE24-.Ltext0
	.quad	.LBB28-.Ltext0
	.quad	.LBE28-.Ltext0
	.quad	0x0
	.quad	0x0
	.quad	.LBB29-.Ltext0
	.quad	.LBE29-.Ltext0
	.quad	.LBB30-.Ltext0
	.quad	.LBE30-.Ltext0
	.quad	0x0
	.quad	0x0
	.quad	.LBB31-.Ltext0
	.quad	.LBE31-.Ltext0
	.quad	.LBB32-.Ltext0
	.quad	.LBE32-.Ltext0
	.quad	0x0
	.quad	0x0
	.quad	.LBB35-.Ltext0
	.quad	.LBE35-.Ltext0
	.quad	.LBB36-.Ltext0
	.quad	.LBE36-.Ltext0
	.quad	0x0
	.quad	0x0
	.quad	.LBB42-.Ltext0
	.quad	.LBE42-.Ltext0
	.quad	.LBB43-.Ltext0
	.quad	.LBE43-.Ltext0
	.quad	0x0
	.quad	0x0
	.section	.debug_str,"MS",@progbits,1
.LASF84:
	.string	"_libc_xmmreg"
.LASF101:
	.string	"__fpregs_mem"
.LASF6:
	.string	"size_t"
.LASF27:
	.string	"__kind"
.LASF79:
	.string	"gregset_t"
.LASF98:
	.string	"uc_stack"
.LASF144:
	.string	"/home/eecs/benh/projects/ht"
.LASF52:
	.string	"_IO_save_end"
.LASF32:
	.string	"__total_seq"
.LASF45:
	.string	"_IO_write_base"
.LASF133:
	.string	"__ht_num_hard_threads"
.LASF61:
	.string	"_lock"
.LASF138:
	.string	"stderr"
.LASF124:
	.string	"argv"
.LASF15:
	.string	"cpu_set_t"
.LASF88:
	.string	"mxcr_mask"
.LASF113:
	.string	"ht_request_async"
.LASF54:
	.string	"_chain"
.LASF58:
	.string	"_cur_column"
.LASF73:
	.string	"sigaltstack"
.LASF65:
	.string	"_mode"
.LASF74:
	.string	"ss_sp"
.LASF118:
	.string	"comparand"
.LASF9:
	.string	"long int"
.LASF69:
	.string	"_IO_marker"
.LASF93:
	.string	"__reserved1"
.LASF122:
	.string	"main"
.LASF121:
	.string	"__arr"
.LASF26:
	.string	"__nusers"
.LASF33:
	.string	"__wakeup_seq"
.LASF4:
	.string	"signed char"
.LASF80:
	.string	"_libc_fpxreg"
.LASF91:
	.string	"gregs"
.LASF68:
	.string	"_IO_FILE"
.LASF29:
	.string	"__data"
.LASF1:
	.string	"unsigned char"
.LASF28:
	.string	"__spins"
.LASF71:
	.string	"_sbuf"
.LASF142:
	.string	"GNU C 4.1.2 20061115 (prerelease) (Debian 4.1.1-21)"
.LASF108:
	.string	"_Bool"
.LASF97:
	.string	"uc_link"
.LASF13:
	.string	"char"
.LASF105:
	.string	"created"
.LASF67:
	.string	"_IO_lock_t"
.LASF7:
	.string	"__uint16_t"
.LASF120:
	.string	"attr"
.LASF42:
	.string	"_IO_read_ptr"
.LASF90:
	.string	"fpregset_t"
.LASF72:
	.string	"_pos"
.LASF136:
	.string	"stdin"
.LASF37:
	.string	"__broadcast_seq"
.LASF53:
	.string	"_markers"
.LASF110:
	.string	"ht_num_hard_threads"
.LASF103:
	.string	"hard_thread"
.LASF23:
	.string	"__lock"
.LASF14:
	.string	"__cpu_mask"
.LASF10:
	.string	"__uint64_t"
.LASF0:
	.string	"long unsigned int"
.LASF19:
	.string	"pthread_t"
.LASF56:
	.string	"_flags2"
.LASF50:
	.string	"_IO_save_base"
.LASF128:
	.string	"resume"
.LASF44:
	.string	"_IO_read_base"
.LASF66:
	.string	"_unused2"
.LASF106:
	.string	"allocated"
.LASF115:
	.string	"available"
.LASF34:
	.string	"__woken_seq"
.LASF20:
	.string	"__size"
.LASF94:
	.string	"mcontext_t"
.LASF57:
	.string	"_old_offset"
.LASF125:
	.string	"args"
.LASF117:
	.string	"futex_wait"
.LASF87:
	.string	"mxcsr"
.LASF8:
	.string	"__uint32_t"
.LASF39:
	.string	"long long int"
.LASF141:
	.string	"__ht_stack"
.LASF130:
	.string	"__ht_mutex"
.LASF131:
	.string	"__ht_condition"
.LASF47:
	.string	"_IO_write_end"
.LASF114:
	.string	"futex_wakeup_all"
.LASF143:
	.string	"ht.c"
.LASF21:
	.string	"__align"
.LASF22:
	.string	"pthread_attr_t"
.LASF46:
	.string	"_IO_write_ptr"
.LASF48:
	.string	"_IO_buf_base"
.LASF77:
	.string	"stack_t"
.LASF70:
	.string	"_next"
.LASF3:
	.string	"unsigned int"
.LASF30:
	.string	"pthread_mutex_t"
.LASF63:
	.string	"__pad1"
.LASF64:
	.string	"__pad2"
.LASF40:
	.string	"pthread_cond_t"
.LASF41:
	.string	"_flags"
.LASF35:
	.string	"__mutex"
.LASF116:
	.string	"futex_wakeup_one"
.LASF24:
	.string	"__count"
.LASF132:
	.string	"__ht_threads"
.LASF25:
	.string	"__owner"
.LASF100:
	.string	"uc_sigmask"
.LASF81:
	.string	"significand"
.LASF140:
	.string	"__ht_context"
.LASF127:
	.string	"__ht_main_trampoline"
.LASF62:
	.string	"_offset"
.LASF16:
	.string	"__bits"
.LASF112:
	.string	"ht_limit_hard_threads"
.LASF85:
	.string	"element"
.LASF38:
	.string	"long long unsigned int"
.LASF109:
	.string	"ht_id"
.LASF31:
	.string	"__futex"
.LASF11:
	.string	"__off_t"
.LASF119:
	.string	"__ht_allocate"
.LASF78:
	.string	"greg_t"
.LASF82:
	.string	"exponent"
.LASF126:
	.string	"ht_request"
.LASF51:
	.string	"_IO_backup_base"
.LASF60:
	.string	"_shortbuf"
.LASF107:
	.string	"thread"
.LASF123:
	.string	"argc"
.LASF111:
	.string	"ht_max_hard_threads"
.LASF12:
	.string	"__off64_t"
.LASF139:
	.string	"__ht_id"
.LASF49:
	.string	"_IO_buf_end"
.LASF134:
	.string	"__ht_max_hard_threads"
.LASF129:
	.string	"__ht_entry_trampoline"
.LASF76:
	.string	"ss_size"
.LASF5:
	.string	"short int"
.LASF135:
	.string	"__ht_limit_hard_threads"
.LASF86:
	.string	"_libc_fpstate"
.LASF95:
	.string	"ucontext"
.LASF59:
	.string	"_vtable_offset"
.LASF104:
	.string	"futex"
.LASF92:
	.string	"fpregs"
.LASF17:
	.string	"__val"
.LASF89:
	.string	"_xmm"
.LASF96:
	.string	"uc_flags"
.LASF36:
	.string	"__nwaiters"
.LASF43:
	.string	"_IO_read_end"
.LASF83:
	.string	"padding"
.LASF102:
	.string	"ucontext_t"
.LASF55:
	.string	"_fileno"
.LASF2:
	.string	"short unsigned int"
.LASF137:
	.string	"stdout"
.LASF99:
	.string	"uc_mcontext"
.LASF75:
	.string	"ss_flags"
.LASF18:
	.string	"__sigset_t"
	.ident	"GCC: (GNU) 4.1.2 20061115 (prerelease) (Debian 4.1.1-21)"
	.section	.note.GNU-stack,"",@progbits
