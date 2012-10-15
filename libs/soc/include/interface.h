/*
 * Copyright (c) 2007 Open Kernel Labs, Inc. (Copyright Holder).
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
/**
 * @file
 * @brief Functions provided by the general kernel that platform code may
 * call.
 *
 * Calling other functions from the platform code is unsupported.
 */

#ifndef __PLATFORM_SUPPORT_H__
#define __PLATFORM_SUPPORT_H__

/*
 * NOTE: platform code should not include any kernel headers directly.  All
 * the ones which are needed are included here.
 */
#include <soc/soc_types.h>
#include <kernel/types.h>

#include <soc/arch/interface.h>

#ifndef ASSEMBLY
#include <kernel/debug.h>
#include <kernel/bitmap.h>
#include <kernel/generic/lib.h>
#include <kernel/errors.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The version against which the "core" kernel was compiled.
 *
 * The value in this word should be taken from SOC_API_VERSION in
 * soc_types.h
 *
 * This will be used at construction of the final image to confirm that the
 * kernel and the SOC code agree on the API version.
 */
extern word_t kernel_api_version;

/**
 * @brief declaration of ref_t for soc usage.
 *
 * Within the "core" kernel tcb structures are "reference counted".  As
 * the SOC code needs to store tcb handles for interrupt delivery it needs
 * access to the reference counting framework.  This struct (and its
 * accessor functions) presents a "cut down" interface to the reference
 * objects, providing the subset of functionality which can be used from
 * SOC code.
 *
 * @note The members of the struct are not for direct access.  Any use of this
 *       type should be via the accessor functions following.
 */
typedef struct
{
    tcb_h  object;   /**< The stored tcb */
    word_t reserved; /**< NOT for SOC usage */
} soc_ref_t;

/**
 * @brief Value of handle indicating a non-existent tcb.
 */
#define NULL_TCB 0

INLINE tcb_h kernel_ref_get_tcb(soc_ref_t ref);
/**
 * @brief Accessor for tcb stored in the reference.
 *
 * @note The value may have been changed by "core" kernel code if the
 *       associated thread was deleted.  The caller needs to compare the
 *       returned value to #NULL_TCB.
 */
INLINE tcb_h kernel_ref_get_tcb(soc_ref_t ref)
{
    return ref.object;
}

/**
 * @brief Init the ref structure.
 *
 * @post The stored count is zero and there is no tcb referenced by the
 *       object.
 */
void kernel_ref_init(soc_ref_t *ref);


/**
 * @brief Set the thread object to be referenced and stored in the ref.
 *
 * @post  #object is set to #obj.
 */
void kernel_ref_set_referenced(tcb_h      obj,
                               soc_ref_t *ref);

/**
 * @brief Remove the object from the ref and cease reference management.
 *
 * @post #obj should no longer be used.
 */
void kernel_ref_remove_referenced(soc_ref_t *ref);

/**
 * @brief Convert an id into a thread handle
 *
 * @param cap The capability id for the thread.
 *
 * @return The corresponding thread handle. This may be #NULL_TCB.
 *
 * @post On return of a value other than #NULL_TCB the corresponsing
 *       thread object is locked.
 */
tcb_h kernel_lookup_tcb_locked(word_t cap);

/**
 * @brief Unlock a locked thread object.
 *
 * @param cap The capability id for the thread.
 *
 * @pre The thread object has previously been locked via
 *      kernel_lookup_tcb_locked().
 *
 * @post The thread object pointed to by #cap is unlocked.
 */
void kernel_unlock_tcb(tcb_h);

/**
 * @brief Get the utcb associated with the supplied thread handle.
 *
 * @param obj the thread to get the utcb for.
 *
 * @return A pointer to the utcb.
 *
 */
utcb_t *kernel_get_utcb(tcb_h obj);

/**
 * @brief Deliver an interrupt notification to a handler thread.
 *
 * @param handler    The handler thread to receive the notification
 * @param notifybits The set of notification bits to deliver to the handler
 *                   via the notify-bits.
 * @param cont       The continuation function to activate after processing
 *                   the interrupt.  If cont is NULL, deliver_notify() will
 *                   not immediately reschedule nad will return, in this case,
 *                   SoC code must check the return value.
 *
 * @return "true" if the SOC needs to manually invoke a re-schedule by calling
 *                kernel_schedule().
 */
bool kernel_deliver_notify(tcb_h          handler,
                           word_t         notifybits,
                           continuation_t cont);

/**
 * @brief Invoke the scheduler to determine thread to run next
 *
 * @param cont The continuation function to be stored for use when the
 *             current thread is reactivated.
 *
 * @pre #cont is not NULL.
 */
void kernel_schedule(continuation_t cont) NORETURN;

/**
 * @brief Inform the scheduler to handle a timer interrupt.
 *
 * @param wakeup   If true it means there is a pending thread to be
 *                 activated.  This may occcur if kernel_deliver_notify()
 *                 was called prior to this.
 * @param interval The time in microseconds since the last timer interrupt
 *                 or since reactivation of the timer (whichever is smaller).
 * @param cont     The continuation function to activate on function
 *                 completion.
 */
void kernel_scheduler_handle_timer_interrupt(int            wakeup,
                                             word_t         interval,
                                             continuation_t cont) NORETURN;

/**
 * @brief Allocate a block of memory
 *
 * @param size The size to allocate in bytes (1KB min).
 * @param zero Set to true to have the allocator guarentee that
 *             the returned memory is zeroed.
 */
void * kernel_mem_alloc(word_t size, int zero);

/**
 * @brief Free a block of memory.
 */
void kernel_mem_free(void * address, word_t size);


/**
 * @brief Add a new mapping to the kernel
 *
 * @param size       The size of the memory region in bytes.
 * @param phys       The physical starting address of the region.
 * @param cache_attr The caching attributes for the mapping.
 *
 * @note There is one predefined value for #cache_attr -
 *       SOC_CACHE_DEVICE  which gives the normal cacing attributes for a
 *       SoC-requested mapping. This is defined in <kernel/arch/interface.h>.
 *       Unless you really know what you are doing, pass this value.
 *
 * @post On success a kernel-only mapping is added with caching attributes
 *       as per the #cache_attr param and read+write+execute permissions
 *       (as relevant for the architecture).
 *
 * @return The virtual address the memory has been mapped at, or NULL
 *         on failure.
 */
addr_t kernel_add_mapping(word_t size,
                          addr_t phys,
                          word_t cache_attr);

/**
 * @brief Add a new mapping to the kernel and specify the virtual
 *        address at which it should be mapped.
 *
 * @deprecated SoC code does not know about the layout of virtual
 *             memory; this is managed by the core kernel.  As such
 *             there may be some "trial and error" in choosing a valid
 *             virtual address.  Further, this function will go away in a
 *             future release.  You have been warned.
 *
 * @param virt       The virtual address at which to map the memory.
 * @param size       The size of the memory region to map in bytes.
 * @param phys       The physical address of the memory region to map.
 * @param cache_attr The caching attributes for the mapping.
 *
 * @note There is one predefined value for #cache_attr -
 *       SOC_CACHE_DEVICE  which gives the normal cacing attributes for a
 *       SoC-requested mapping. This is defined in <kernel/arch/interface.h>.
 *       Unless you really know what you are doing, pass this value.
 *
 * @post On success a kernel-only mapping is added of #phys to #virt of
 *       size #size rounded up by page size.  The mapping will have
 *       caching attributes as per #cache_attr and read+write+execute
 *       permissions (as relevant for the CPU).
 *
 * @retval non-zero for success.
 * @retval 0        for failure.
 */
bool kernel_add_mapping_va(addr_t virt,
                           word_t size,
                           addr_t phys,
                           word_t cache_attr);

/**
 * @brief An unrecoverable error has been encountered while in SoC code.
 *
 * @param msg Message string to print if debugging support is enabled.
 *
 * @note If the kernel debugger is enabled this function will enter KDB.
 *       otherwise soc_panic() is called.
 */
void kernel_fatal_error(const char *msg) NORETURN;

void kernel_enter_kdebug(const char *msg);

/**
 * @brief Perform architecture initialisation and start the kernel.
 *
 * This is expected to be called from _start once the low-level
 * SOC-specific initialisation has been completed.
 *
 * @param param The contents of this is architecture-specific.
 */
void kernel_arch_init(void *param) NORETURN;

/*
 * Trace Macros
 *
 * The wrapper macros determine whether to build with debugger capability
 * or not.  Their definition should match their use in the kernel which the
 * SOC code will be combeined with.  The SDK provides the list of defines
 * used for each kernel.o provided.
 */
#if defined(CONFIG_DEBUG)
#if defined(CONFIG_KDB_CONS)
#if !defined(CONFIG_KDB_NO_ASSERTS)

/**
 * @brief Access to the "core" kernel output.
 *
 * @note: As this can be compiled out, it should NOT be used directly.  It
 *        should only be accessed by the debugging macros below.
 */
int kernel_printf(const char * format, ...);

#if defined(_lint)
#define SOC_ASSERT(level, x) do { if (! (x)) { __panic(); } } while(false)
#else
#define SOC_ASSERT(level, x)                                           \
    do {                                                                \
        if (EXPECT_FALSE(! (x)) && (level <= CONFIG_ASSERT_LEVEL)) {    \
            kernel_printf ("Assertion "#x" failed in file %s, line %d (fn=%p)\n", \
                                     __FILE__, __LINE__, __return_address()); \
            kernel_enter_kdebug ("assert");                   \
        }                                                               \
    } while(false)
#endif

#define SOC_TRACEF(f, x...)                                            \
    do {                                                                \
        (void)kernel_printf ("%s:%d: " f, __FUNCTION__, __LINE__ ,##x); \
    } while(false)

#define SOC_TRACE(x...)    soc_printf(x)

# else /* defined(CONFIG_KDB_NO_ASSERTS) */

#define SOC_ASSERT(level, x)   do { } while (false)
#define SOC_TRACE(x...)        do { } while (false)
#define SOC_TRACEF(x...)       do { } while (false)

#endif

#else /* ! defined(CONFIG_KDB_CONS) */

#define SOC_ASSERT(level, x)   do { } while (false)
#define SOC_TRACE(x...)        do { } while (false)
#define SOC_TRACEF(x...)       do { } while (false)

#endif /* CONFIG_KDB_CONS */

#else /* !defined(CONFIG_KDEBUG) */

#define SOC_ASSERT(level, x)   do { } while (false)
#define SOC_TRACE(x...)        do { } while (false)
#define SOC_TRACEF(x...)       do { } while (false)

#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#else /* So this is _for_ assembly */
#include <kernel/arch/asm.h>
#include <kernel/arch/continuation.h>

#endif /* ! ASSEMBLY */

#endif /*__PLATFORM_SUPPORT_H__*/
