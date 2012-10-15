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
 * ARMv5 Definitions
 */

#ifndef __ARCH_ARMV5_H__
#define __ARCH_ARMV5_H__

/* ARM CP15 Registers */
#define CP15_REG_CPU_INFO              c0        /* CPU information */
#define CP15_REG_CONTROL               c1        /* CPU setup */
#define CP15_REG_TRANS_TABLE_BASE      c2        /* Current page table. */
#define CP15_REG_DOMAIN_ACCESS_CTRL    c3        /* Domain access control. */
#define CP15_REG_FAULT_STATUS          c5        /* Fault status. */
#define CP15_REG_FAULT_ADDR            c6        /* Fault address. */
#define CP15_REG_CACHE_OPS             c7        /* Cache operations. */
#define CP15_REG_TLB_OPS               c8        /* TLB opereations. */
#define CP15_REG_CACHE_LOCKDOWN        c9        /* Cache lockdown settings. */
#define CP15_REG_TLB_LOCKDOWN          c10       /* TLB lockdown settings. */
#define CP15_REG_PROCESS_ID            c13       /* Processor PID. */
#define CP15_REG_BREAKPOINT            c14       /* Debug breakpoint. */
#define CP15_REG_COPROCESSOR_ACCESS    c15       /* Coprocessor access. */

/* ARM CP15/C1 CPU Setup Options. */
#define CP15_CNTL_DEFAULT             0x00000078 /* Set reserved bits as required. */
#define CP15_CNTL_MMU_ENABLE          (1 << 0)   /* Enable memory management. */
#define CP15_CNTL_ALIGNMENT_FAULTS    (1 << 1)   /* Enable alignment faults. */
#define CP15_CNTL_DCACHE_ENABLE       (1 << 2)   /* Enable data cache. */
#define CP15_CNTL_BIG_ENDIAN_ENABLE   (1 << 7)   /* Enable big endianness. */
#define CP15_CNTL_SYSTEM_PROTECTION   (1 << 8)   /* Enable system protection. */
#define CP15_CNTL_ROM_PROTECTION      (1 << 9)   /* Enable ROM protection. */
#define CP15_CNTL_BRANCH_BUFF_ENABLE  (1 << 11)  /* Enable branch target buffer. */
#define CP15_CNTL_ICACHE_ENABLE       (1 << 12)  /* Enable instruction cache. */
#define CP15_CNTL_VECTOR_RELOCATE     (1 << 13)  /* Relocate vectors to high address. */

/*
 * SPSR Constants
 */
#define SPSR_INTERRUPT_DISABLE   (1 << 7)     /* Disable interrupts. */
#define SPSR_FIQ_DISABLE         (1 << 6)     /* Disable fast interrupts. */
#define SPSR_THUMB_MODE          (1 << 5)     /* Enable thumb mode. */

#define SPSR_MODE_USER           0x10         /* User mode. */
#define SPSR_MODE_FIQ            0x11         /* FIQ mode. */
#define SPSR_MODE_IRQ            0x12         /* IRQ mode. */
#define SPSR_MODE_SUPERVISOR     0x13         /* Supervisor mode. */
#define SPSR_MODE_ABORT          0x17         /* Abort mode. */
#define SPSR_MODE_UNDEF          0x1b         /* Undefined instruction mode. */
#define SPSR_MODE_SYSTEM         0x1f         /* System mode. */

#define SPSR_FLAG_N              (1 << 31)    /* Negative flag. */
#define SPSR_FLAG_Z              (1 << 30)    /* Zero flag. */
#define SPSR_FLAG_C              (1 << 29)    /* Carry flag. */
#define SPSR_FLAG_V              (1 << 28)    /* Overflow flag. */
#define SPSR_FLAG_Q              (1 << 27)    /* Saturation flag. */

/*
 * ARM MMU Constants.
 */
#define PT_CACHEABLE            (1 << 3)
#define PT_BUFFERED             (1 << 2)

#define PT_L1_COARSE_PAGE_TABLE (1 << 0)
#define PT_L1_SECTION           (2 << 0)
#define PT_L1_FINE_PAGE_TABLE   (3 << 0)

#define PT_L2_LARGE_PAGE        (1 << 0)
#define PT_L2_SMALL_PAGE        (2 << 0)
#define PT_L2_TINY_PAGE         (3 << 0)

/* Page permission bits. */
#define PT_PERM_SUPERVISOR      (1)
#define PT_PERM_RO              (2)
#define PT_PERM_RW              (3)

/* Sets up the permissions for a given page entry. */
#define PT_PERMS_SECTION(x)     ((x) << 10)
#define PT_PERMS_LARGE_PAGE(x)  (((x) << 4) | ((x) << 6) | ((x) << 8) | ((x) << 10))
#define PT_PERMS_SMALL_PAGE(x)  (((x) << 4) | ((x) << 6) | ((x) << 8) | ((x) << 10))
#define PT_PERMS_TINY_PAGE(x)   ((x) << 4)

/*
 * For a number of these definitions, we use '+' instead of '|'. This is
 * because sometimes we want to have arbitrary symbols in these macros, and
 * patch them at link-time. Linkers tend to only be able to do simple
 * additions, however.
 *
 * Whenever these operations are being used correctly, '+' will behave
 * indentically to '|'.
 */

/* Define a section in the top-level page table. */
#define PT_L1_RAW_SECTION(paddr, flags) \
        ((paddr) + ((flags) | PT_L1_SECTION))

/* Define a second-level small page. */
#define PT_L2_RAW_SMALL_PAGE(paddr, flags) \
        ((paddr) + ((flags) | PT_L2_SMALL_PAGE))

/* Define a cached system section in the top-level page table. */
#define PT_L1_SYSTEM_SECTION(paddr) \
        PT_L1_RAW_SECTION(paddr, \
                PT_CACHEABLE | PT_BUFFERED \
                | PT_PERMS_SECTION(PT_PERM_SUPERVISOR))

/* Define a second-level course page table. */
#define PT_L1_RAW_COARSE_TABLE(paddr) \
        ((paddr) + PT_L1_COARSE_PAGE_TABLE)

/* Define an uncached system section in the top-level page table. */
#define PT_L1_UNCACHED_SYSTEM_SECTION(paddr) \
        PT_L1_RAW_SECTION(paddr, PT_PERMS_SECTION(PT_PERM_SUPERVISOR))

/* Define an uncached system section in the top-level page table. */
#define PT_L1_UNCACHED_USER_SECTION(paddr) \
        PT_L1_RAW_SECTION(paddr, PT_PERMS_SECTION(PT_PERM_RW))

/* Define a cached user section in the top-level page-table. */
#define PT_L1_USER_SECTION(paddr) \
        PT_L1_RAW_SECTION(paddr, \
                PT_CACHEABLE | PT_BUFFERED | PT_PERMS_SECTION(PT_PERM_RW))

#endif /* __ARCH_ARMV5_H__ */
