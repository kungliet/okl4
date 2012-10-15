
#include <asm.h>

PHDRS {
    kernel PT_LOAD;
    elfweaver PT_LOAD;
}

_entry = __kernel_main;
ENTRY (_entry)

SECTIONS
{
    /* Kernel memory region is in the range 0xf0000000 -- 0xffffffff. */
    . = VADDR_BASE;

    start_asm_text = .;

    /*
     * Vector Table.
     *
     * This vector table is mapped at 0xffff0000 and is jumped to by
     * the ARM processor when cetain events (interrupts/data aborts/syscalls)
     * occur.
     */
    _linker_start_vectors = .;
    .vectors . : {
        *(.vectors)
        *(.vectors.*)
    } : kernel


    /* Kernel assembly code. 
     * This includes uses of the LDR_DATA macro which requirese the data to be within 4k
     * of the usage, so the asmtext is split from the main text to keep it small
     * and the distance is checked in the linker script
     */

    .asmtext ALIGN(32) : { *(.asmtext .asmtext.*) } : kernel

    /*
     * Kernel data.
     */
    _start_data = .;
    .data ALIGN(32) : {
        *(.sdata .scommon .sbss)
        *(.data)
        *(.rodata)
        *(.rodata.*)
        *(.rodata)
        *(.rodata.*)
        *(.glue*)
        } : kernel

    .bss ALIGN(32) : {
        *(.bss)
        *(COMMON)
        } : kernel

    /* 
     * Check that all of the uses of the LDR_DATA macro are within acceptable range
     */
    ASSERT(. - start_asm_text < 4096, "Data and asmtext do not fit into range of LDR macro (see armv5_instructions.h)")

    /* Other kernel code. */
    .text ALIGN(32) : { *(.text .text.*) } : kernel

    /* Initialisation code. */
    .init ALIGN(32) : { *(.init) } : kernel

    /* Stack. */
    .stack ALIGN(32) : { *(.stack) } : kernel

    /* Trace buffer. */
    .trace_buffer ALIGN(32) : { *(.trace_buffer) } : kernel

    /*
     * User Helpers.
     *
     * These are a set of functions designed to help out the user to carry out
     * certain actions, such as atomic operations. The user can use these
     * functions by performing a long call to a magic address that we promise
     * to keep constant. The kernel is responsible for making them work.
     *
     * The user helpers are remapped user executable at vaddr 0xfffef000, which means we need to start
     * at the beginning of a page.
     */
    . = ALIGN(4K);
    arm_user_helpers_start_addr = .;
    .user_helpers . : {
        *(.user_helpers)
        *(.user_helpers.*)
    } : kernel

    /* Elfweaver info section */
    . = ALIGN(4K);
    .elfweaver_info . : {
        *(.elfweaver_info)
    } : elfweaver
    .elfweaver_extra . : {
        *(.elfweaver_extra)
    } : elfweaver

    .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_line     0 : { *(.debug_line) }
    .debug_frame    0 : { *(.debug_frame) }
    .debug_str      0 : { *(.debug_str) }
    .debug_loc      0 : { *(.debug_loc) }
    .debug_macinfo  0 : { *(.debug_macinfo) }

}
