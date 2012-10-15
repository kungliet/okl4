/*
 * Copyright (c) 2008 Open Kernel Labs, Inc. (Copyright Holder).
 * All rights reserved.
 *
 * 1. Redistribution and use of OKL4 (Software) in source and binary
 * forms, with or without modification, are permitted provided that the
 * following conditions are met:
 *
 *     (a) Redistributions of source code must retain this clause 1
 *         (including paragraphs (a), (b) and (c)), clause 2 and clause 3
 *         (Licence Terms) and the above copyright notice.
 *
 *     (b) Redistributions in binary form must reproduce the above
 *         copyright notice and the Licence Terms in the documentation and/or
 *         other materials provided with the distribution.
 *
 *     (c) Redistributions in any form must be accompanied by information on
 *         how to obtain complete source code for:
 *        (i) the Software; and
 *        (ii) all accompanying software that uses (or is intended to
 *        use) the Software whether directly or indirectly.  Such source
 *        code must:
 *        (iii) either be included in the distribution or be available
 *        for no more than the cost of distribution plus a nominal fee;
 *        and
 *        (iv) be licensed by each relevant holder of copyright under
 *        either the Licence Terms (with an appropriate copyright notice)
 *        or the terms of a licence which is approved by the Open Source
 *        Initative.  For an executable file, "complete source code"
 *        means the source code for all modules it contains and includes
 *        associated build and other files reasonably required to produce
 *        the executable.
 *
 * 2. THIS SOFTWARE IS PROVIDED ``AS IS'' AND, TO THE EXTENT PERMITTED BY
 * LAW, ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, OR NON-INFRINGEMENT, ARE DISCLAIMED.  WHERE ANY WARRANTY IS
 * IMPLIED AND IS PREVENTED BY LAW FROM BEING DISCLAIMED THEN TO THE
 * EXTENT PERMISSIBLE BY LAW: (A) THE WARRANTY IS READ DOWN IN FAVOUR OF
 * THE COPYRIGHT HOLDER (AND, IN THE CASE OF A PARTICIPANT, THAT
 * PARTICIPANT) AND (B) ANY LIMITATIONS PERMITTED BY LAW (INCLUDING AS TO
 * THE EXTENT OF THE WARRANTY AND THE REMEDIES AVAILABLE IN THE EVENT OF
 * BREACH) ARE DEEMED PART OF THIS LICENCE IN A FORM MOST FAVOURABLE TO
 * THE COPYRIGHT HOLDER (AND, IN THE CASE OF A PARTICIPANT, THAT
 * PARTICIPANT). IN THE LICENCE TERMS, "PARTICIPANT" INCLUDES EVERY
 * PERSON WHO HAS CONTRIBUTED TO THE SOFTWARE OR WHO HAS BEEN INVOLVED IN
 * THE DISTRIBUTION OR DISSEMINATION OF THE SOFTWARE.
 *
 * 3. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR ANY OTHER PARTICIPANT BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * ARMv5 Instruction Patching
 *
 * The GNU ARM Assembler does not support a particularly wide number of
 * relocations. In particular, it has almost no support for placing relocations
 * inside instructions. Thus, certain instruction sequences, such as:
 *
 *     ldr     r0, [pc, #current_tcb - . - 8]
 *
 * (the most optimal way to reference data)
 * Which, after link time, have well-defined meanings, become impossible to
 * represent. 
 *
 * One solution is to manually encode instructions, and get
 * the linker to add an offset:
 *
 *     .word   LOAD_INS_ENCODING + current_tcb - . - 8
 *
 * This requires that all uses of the macro and all of the data referred to by 
 * the macro be within 4K of each other (due to the range of the pc relative
 * load instruction).  This is (crudely) enforced by an assert in the linker script.
 * All uses of this macro must be in the vectors or asmtext section.
 */

#ifndef __ARCH_ARMV5_INSTRUCTIONS_H__
#define __ARCH_ARMV5_INSTRUCTIONS_H__

/* Registers. */
#define REG_R0      0x0
#define REG_R1      0x1
#define REG_R2      0x2
#define REG_R3      0x3
#define REG_R4      0x4
#define REG_R5      0x5
#define REG_R6      0x6
#define REG_R7      0x7
#define REG_R8      0x8
#define REG_R9      0x9
#define REG_R10     0xa
#define REG_R11     0xb
#define REG_R12     0xc
#define REG_R13     0xd
#define REG_R14     0xe
#define REG_R15     0xf
#define REG_SP      REG_R13
#define REG_LR      REG_R14
#define REG_PC      REG_R15

/* Condition codes. */
#define COND_EQ     0x0
#define COND_NE     0x1
#define COND_AL     0xe

/*
 * Instructions
 */

/* ldr     rDest, [rSrc + offset] */
#define LDR_OFF(cond, dest, offset) \
        .word (0x05900000 + (cond << 28) + (0xf << 16) + (dest << 12) + ((offset) - . - 8))

/* str     rSrc, [rDest + offset] */
#define STR_OFF(cond, src, offset) \
        .word (0x05800000 + (cond << 28) + (0xf << 16) + (src << 12) + ((offset) - . - 8))


#endif /* __ARCH_ARMV5_INSTRUCTIONS_H__ */
