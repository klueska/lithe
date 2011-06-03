/* Save current context and install the given one.
   Copyright (C) 2002, 2005 Free Software Foundation, Inc.
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


/* int __swapcontext (ucontext_t *oucp, const ucontext_t *ucp)

  Saves the machine context in oucp such that when it is activated,
  it appears as if __swapcontextt() returned again, restores the
  machine context in ucp and thereby resumes execution in that
  context.

  This implementation is intended to be used for *synchronous* context
  switches only.  Therefore, it does not have to save anything
  other than the PRESERVED state.  */

.text
	.align 4
.globl swapcontext
	.type swapcontext, @function
swapcontext:
	/* Load address of the context data structure we save in.  */
	movl	4(%esp), %eax

	/* Return value of swapcontext.  EAX is the only register whose
	   value is not preserved.  */
	movl	$0, oEAX(%eax)

	/* Save the 32-bit register values and the return address.  */
	movl	%ecx, oECX(%eax)
	movl	%edx, oEDX(%eax)
	movl	%edi, oEDI(%eax)
	movl	%esi, oESI(%eax)
	movl	%ebp, oEBP(%eax)
	movl	(%esp), %ecx
	movl	%ecx, oEIP(%eax)
	leal	4(%esp), %ecx
	movl	%ecx, oESP(%eax)
	movl	%ebx, oEBX(%eax)

	/* Save the FS segment register.  */
	xorl	%edx, %edx
	movw	%fs, %dx
	movl	%edx, oFS(%eax)

	/* We have separate floating-point register content memory on the
	   stack.  We use the __fpregs_mem block in the context.  Set the
	   links up correctly.  */
	leal	oFPREGSMEM(%eax), %ecx
	movl	%ecx, oFPREGS(%eax)
	/* Save the floating-point context.  */
	fnstenv	(%ecx)

	/* Load address of the context data structure we have to load.  */
	movl	8(%esp), %ecx

	/* Restore the floating-point context.  Not the registers, only the
	   rest.  */
	movl	oFPREGS(%eax), %ecx
	fldenv	(%ecx)

	/* Restore the FS segment register.  We don't touch the GS register
	   since it is used for threads.  */
	movl	oFS(%eax), %edx
	movw	%dx, %fs

	/* Fetch the address to return to.  */
	movl	oEIP(%eax), %ecx

	/* Load the new stack pointer.  */
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

	/* The following 'ret' will pop the address of the code and jump
	   to it.  */
	ret
