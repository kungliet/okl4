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
 * @brief Contains the interface for functionality provided on a per-platform
 *        basis.
 *
 * The types and functions declared here provide the platform specific code to
 * the wider kernel.
 */

#ifndef __L4__PLATFORM_H__
#define __L4__PLATFORM_H__

#include <soc/soc_types.h>

#if defined(CONFIG_KDB_CONS)
#include <kernel/kdb/console.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The version against which the platform code was compiled.
 *
 * The value in this word should be taken from SOC_API_VERSION in
 * soc_types.h
 *
 * This will be used at construction of the final image to confirm that the
 * kernel and the SOC code agree on the API version.
 */
extern word_t soc_api_version;

/**
 * @brief PlatformControl control word.
 *
 * As the operations available are determined by each platform, there is
 * very little detail at this level; the only specific information relates
 * to the number of message registers used.
 *
 * Note: The contents of the struct should not be accessed directly, but
 * via the accessor functions below.
 *
 * @var raw The control word as a "raw" value.
 * @var n   The number of message regisgers used.
 * @var req The platform-defined request value.
 */
typedef struct plat_control
{
    union {
        word_t raw;
        struct {
            BITFIELD2(word_t,
                    n    : 6,
                    req  : BITS_WORD - 6);
        } x;
    };
} plat_control_t;

/*
 * Accessor functions for plat_control_t
 */
INLINE word_t platcontrol_highest_item(plat_control_t p);
INLINE word_t platcontrol_request(plat_control_t p);
INLINE word_t platcontrol_get_raw(plat_control_t p);
INLINE void platcontrol_set_raw(plat_control_t *p, word_t val);

/**
 * @brief Indicates the highest item number stored in message registers.
 */
INLINE word_t platcontrol_highest_item(plat_control_t p)
{
    return p.x.n;
}

/**
 * @brief The request code.
 */
INLINE word_t platcontrol_request(plat_control_t p)
{
    return p.x.req;
}

/**
 * @brief The entire control word.
 */
INLINE word_t platcontrol_get_raw(plat_control_t p)
{
    return p.raw;
}

/**
 * @brief set the control word.
 */
INLINE void platcontrol_set_raw(plat_control_t *p,
                                word_t val)
{
    p->raw = val;
}

/**
 * @brief The entry point for the kernel.
 *
 * The platform must provide a symbol _start as the entry point for the
 * kernel.  This will be called with the system in "power-up" state - i.e.
 * the state of the system will be whatever default state it is when
 * it is first turned on.
 *
 * The function must carry out the "low-level" initialisation of the
 * CPU/SoC (see post condition for details).
 *
 * The startup code must end by calling kernel_arch_init() to continue
 * booting the kernel.
 *
 * @note This function may use NONE of the core kernel provided interfaces,
 *       beyond calling .
 *
 * @post At function exit (by calling kernel_arch_init()) the platform state
 *       is set as follows:
 *       @li All caching is disabled.
 *       @li Virtual memory is disabled (i.e. MMU turned off).
 *       @li Memory controllers are configured and running.
 *       @li The stack pointer is set.
 *       @li Other control registers required for further initialisation
 *           are set.
 *
 */
void _start(void);


/**
 * @brief Initialise the SOC.
 *
 * This function should perform any required platform
 * initialisation.  At a minimum this this includes:
 * @li Mapping IO areas into virtual memory.
 * @li Initialising timers.
 * @li Interrupt initialisation specific to the SOC.
 *
 * @note The last point needs some expansion.  Some CPUs will have a
 * "built-in" interrupt controller.  Some CPUs will have minimal "core"
 * interrupt capabilities.  Where the "core" architecture-specific code
 * can do interrupt initialisation/configuration it will happen _after_
 * this function has returned.  The SOC should do the "rest" of the (pre-)
 * configuration here.
 *
 * @pre Virtual memory is enabled and the memory subsystem initialised.  This
 *      means that the following "core" kernel-spplied functions may be used
 *      here:
 *      @li kernel_add_mapping()
 *      @li kernel_mem_alloc()
 *      @li kernel_mem_free()
 *      @li kernel_printf() (if a "debug" build is configured) - meaning
 *          all tracing macros may be used.
 *
 * @post All platform specific initialisation related to memory sections
 *       is complete.
 */
void soc_init(void);

/***************************************************************************
 * Interrupt handling
 **************************************************************************/

/**
 *  @brief The platform's description of an interrupt.
 *
 *  This is not used by "core" kernel code.  It is up to the SOC to
 *  define it as it needs.
 */
struct irq_desc;

/*
 * Note: soc_handle_interrupt() is declared in the architecture-specific
 * header.
 */

/**
 * @brief Configure access control for interrupts.
 *
 * This function provides the functionality to specify which space
 * is to be considered the owner of a given interrupt.
 *
 * @param desc    Pointer to the interrupt descriptor to be configured.
 *                This is opaque from the perspective of the core
 *                kernel.
 *
 * @param owner   The space to be associated with the interrupt descriptor.
 *                To be treated as opaque by platform code.
 *
 * @param control This describes the operation to carry out (associate,
 *                disassociate).  The value is stored in the top 8 bits of
 *                the word with 0 being for associate and 1 being for
 *                disassociate.
 *
 *
 * @post On association success, #owner is stored as the space to which any
 *       handler thread for #desc must belong.  (This will affect the checking
 *       in #soc_security_control_interrupt()).  Subsequent attempts to
 *       associate a space with this interrupt descriptor will fail.
 * @post On disassociation succes, there is no owner of #desc, and any call to
 *       #soc_security_control_interrupt() will fail.  Subsequent attempts
 *       to associate an owner with this interrupt descriptor may succeed.
 *
 *
 * @retval 0              for success.
 * @retval EINVALID_PARAM If the value of #desc is invalid (defined by the
 *                        SoC).
 * @retval EINVALID_PARAM for an attempt to disassociate when there is no owner.
 * @retval EINVALID_PARAM for an attempt to associate when there already is an
 *                        owner.
 */
word_t soc_security_control_interrupt(struct irq_desc *desc,
                                      space_h          owner,
                                      word_t           control);
/**
 * @brief The two actions defined for soc_configure_interrupt().
 */
typedef enum {
    soc_irq_register   = 0,
    soc_irq_unregister = 1
} soc_irq_action_e;

/**
 * @brief Register or unregister a specific thread to handle an interrupt.
 *
 * @param desc       A pointer to the interrupt descriptor.  This is opaque
 *                   from the perspective of the core kernel code.
 *
 * @param notify_bit The "Asynchronous Notification" bit to be used when
 *                   notifying handler thread of the interrupt.  The
 *                   value indicates the bit number in the notification mask
 *                   to set.
 *
 * @param handler    The thread to deal with this interrupt.  To be treated
 *                   as opaque by platform code.  This will be passed back
 *                   in notification calls.
 *
 * @param owner      The space in which the handling thread resides. To be
 *                   treated as opaque by SOC code.  This is used for
 *                   validation.
 *
 * @param action     This describes the operation to carry out
 *                   (register/deregister).
 *
 * @param utcb       The user data associated with the handling thread. It is
 *                   provided to allow access to the space for SoC reserved
 *                   words.
 *
 * @note The #utcb parameter should not be stored for later use.  This
 *       is because the #utcb associated with #handler can change (e.g.
 *       due to a space switch operation).
 *
 *
 * @post On mapping success #handler is stored (along with their #utcb and
 *       #notify_bit) to allow notification on occurrence of the interrupt
 *       described in #desc.  The interrupts in #desc are enabled.
 *       Calls to soc_ack_interrupt() for #desc must have the same #handler
 *       to succeed. Subsequent attempts to map a handler to this interrupt
 *       will fail.
 * @post On unmapping success there is no handler associated with the
 *       interrupt in #desc.  The interrupts in #desc are disabled.
 *       Subsequent attempts to map a handler to this interrupt may succeed.
 *
 * @retval 0              for success
 * @retval ENO_PRIVELEGE  if #owner does not match the registered owner, or
 *                        there is no registered owner.
 * @retval EINVALID_PARAM if #desc is invalid (as defined by SOC code).
 * @retval EINVALID_PARAM if #handler is not the registered handler when
 *                        performing an unregister request.
 */
word_t soc_config_interrupt(struct irq_desc *desc,
                            int              notify_bit,
                            tcb_h            handler,
                            space_h          owner,
                            soc_irq_action_e action,
                            utcb_t          *utcb);


/**
 * @brief Acknowledge processing of an interrupt.
 *
 * @param desc    A pointer to the interrupt descriptor.  This is
 *                opaque from the perspective of the generic kernel.
 *
 * @param handler The thread wishing to ack the interrupt.
 *
 * @post On success the interrupt is acknowledged, and any other pending
 *       interrupts for #handler have been delivered.  The interrupts in
 *       #desc have been reenabled.
 *
 * @retval 0             for success
 * @retval ENO_PRIVILEGE if #handler does not match the assigned handler.
 */
word_t soc_ack_interrupt(struct irq_desc *desc,
                         tcb_h            handler);


/* KDB console support. */
#if defined(CONFIG_KDB)
/**
 * @brief Carry out all initialisation related to KDB.
 *
 * The exact work required is dependent on which other KDB-related values
 * are defined.
 *
 * When debugging is enabled (via CONFIG_KDB_CONS) the work of this function
 * will include, as a minimum, initializing the console.
 */
void soc_kdb_init(void);

/*
 * The next two functions are only needed for the debug console.
 */
#if defined(CONFIG_KDB_CONS)

/**
 * @brief Return a character from the active console device.
 *
 * @param can_block  If true then the function may block while waiting
 *                   for a character to arrive.
 *
 * @return -1 If there is no character available, otherwise the current
 *            character.
 */
int soc_console_getc(bool can_block);

/**
 * @brief Output a character to the active console device.
 *
 * @param c The character to send to the console.
 *
 * This function may block while waiting for the device to accept the
 * character.
 */
void soc_console_putc(char c);
#endif

/**
 * @brief Reboot the platform.
 *
 * @note This function must not return under any circumstances.
 */
void NORETURN soc_reboot(void);
#endif

/**
 * @brief System panic.
 *
 * This is called when the system has generated an unrecoverable error. It is
 * up to the SOC code to decide what the appropriate action is.  Options
 * may include:
 * @li Just rebooting.
 * @li Some sort of trace dump.
 * @li Activating some form of custom debugging.
 * @li Executing the "halt and catch fire" instruction.
 * @li Other options as the SOC developer sees fit.
 *
 * @note This function must not return under any circumstances.
 *
 */
void NORETURN soc_panic(void);


/**
 * @brief Carry out a platform-specific operation
 *
 * @param current The thread making the request.
 * @param control The operation.
 * @param param1  SoC Defined.
 * @param param2  SoC Defined.
 * @param param3  SoC Defined.
 * @param cont    The continuation function to use for return.  If the SoC
 *                code decides that a reschedule or sleep is required, it
 *                should use this continuation in the call to
 *                kernel_schedule().
 *
 *
 * @return !0  For success.
 * @return 0 For failure.  Error values will be SoC specific, and the SoC
 *                         sets them in #current_utcb.
 *
 * @post On failure, #current_utcb has the error value set in it.
 */
word_t soc_do_platform_control(tcb_h           current,
                               plat_control_t  control,
                               word_t          param1,
                               word_t          param2,
                               word_t          param3,
                               continuation_t  cont);

/**
 * @brief The platform code should provide a boolean value to indicate
 *        whether ticks are currently enabled or disabled.
 *
 * The platform code may re-enable the timer in response to an interrupt
 * independently of the "core" kernel.  Hence the storage of the state within
 * plarform code.
 */
extern word_t soc_timer_disabled;

/**
 * @brief Enable timer.
 *
 * @post #soc_timer_disabled is set to false and timer interrupts are
 *       enabled.
 */
void soc_enable_timer(void);

#if defined(CONFIG_KDEBUG_TIMER)
/**
 * @brief Get platform specific length of the timer tick in microseconds.
 *
 * @return length of the timer tick in microseconds.
 */
word_t soc_get_timer_tick_length(void);
#endif


/**
 * @brief Disable timer.
 *
 * Mask out the timer interrupt.  This operation may be overridden by
 * the presence of interrupt activity.
 *
 * @post #soc_timer_disabled is set to true and timer interrupts are
 *       disabled.
 */
void soc_disable_timer(void);

#ifdef __cplusplus
}
#endif

#endif
