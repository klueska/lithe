/* Install given context.
   Copyright (C) 2002, 2004, 2005 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Andreas Jaeger <aj@suse.de>, 2002.

   The GNU C Library is free software ;  you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation	;  either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY ;  without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library	;  if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include "ucontext_i.h"

#  define cfi_startproc
#  define cfi_endproc
#  define cfi_def_cfa(reg, off)
#  define cfi_def_cfa_register(reg)
#  define cfi_def_cfa_offset(off)
#  define cfi_adjust_cfa_offset(off)
#  define cfi_offset(reg, off)
#  define cfi_rel_offset(reg, off)
#  define cfi_register(r1, r2)
#  define cfi_return_column(reg)
#  define cfi_restore(reg)
#  define cfi_same_value(reg)
#  define cfi_undefined(reg)
#  define cfi_remember_state
#  define cfi_restore_state
#  define cfi_window_save


/*  int __setcontext (const ucontext_t *ucp)

  Restores the machine context in UCP and thereby resumes execution
  in that context.

  This implementation is intended to be used for *synchronous* context
  switches only.  Therefore, it does not have to restore anything
  other than the PRESERVED state.  */

.text
	.align 4
.globl setcontext
	.type setcontext, @function
setcontext:
	/* Load address of the context data structure.  */
	movl	4(%esp), %eax

	/* Restore the floating-point context.  Not the registers, only the
	   rest.  */
	movl	oFPREGS(%eax), %ecx
	fldenv	(%ecx)

	/* Restore the FS segment register.  We don't touch the GS register
	   since it is used for threads.  */
	movl	oFS(%eax), %ecx
	movw	%cx, %fs

	/* Fetch the address to return to.  */
	movl	oEIP(%eax), %ecx

	/* Load the new stack pointer.  */
	cfi_def_cfa (eax, 0)
	cfi_offset (edi, oEDI)
	cfi_offset (esi, oESI)
	cfi_offset (ebp, oEBP)
	cfi_offset (ebx, oEBX)
	cfi_offset (edx, oEDX)
	cfi_offset (ecx, oECX)
	movl	oESP(%eax), %esp

	/* Push the return address on the new stack so we can return there.  */
	pushl	%ecx

	/* Load the values of all the 32-bit registers (except ESP).
	   Since we are loading from EAX, it must be last.  */
	movl	oEDI(%eax), %edi
	movl	oESI(%eax), %esi
	movl	oEBP(%eax), %ebp
	movl	oEBX(%eax), %ebx
	movl	oEDX(%eax), %edx
	movl	oECX(%eax), %ecx
	movl	oEAX(%eax), %eax

	/* End FDE here, we fall into another context.  */
	cfi_endproc
	cfi_startproc

	/* The following 'ret' will pop the address of the code and jump
	   to it.  */
	ret
