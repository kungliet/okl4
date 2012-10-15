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

#ifndef __OKL4__KTHREAD_H__
#define __OKL4__KTHREAD_H__

#include <compat/c.h>
#include <okl4/types.h>

#if defined(OKL4_KERNEL_MICRO)
#include <okl4/kspace.h>
#include <okl4/kclist.h>
#endif

/**
 *  @file
 *
 *  The kthread.h Header.
 *
 *  The kthread.h header provides the functionality required for
 *  using the kernel thread (@a kthread) interface.
 *
 *  The libokl4 kthread interface functionality may be categorized as:
 *
 *  @li Kthread attribute and its operations (Sections okl4_kthread_attr()
 *  to okl4_kthread_setprio())
 *
 *  @li Kthread and its operations (Sections okl4_kthread_kthread() to
 *  okl4_kthread_delete())
 */

/**
 *  The okl4_kthread_attr structure represents the kthread
 *  attribute used to create kthreads.
 *
 *  As the kthread is not a purely user-level construct, constructing a 
 *  new kthread using the OKL4 library always invokes
 *  a kernel syscall to create a thread within the kernel.
 *
 *  The following parameters are encoded in each kthread attribute: 
 *
 *  @li The @a kspace to which the kthread is attached. 
 *
 *  @li The @a kcap that is used as the thread identifier.
 *  
 *  @li The @a utcb area that it uses for its user thread control block.
 *
 *  @li The stack pointer and instruction pointer of the kthread.
 *
 *  @li The scheduling @a priority of the kthread.
 *
 *  @li The @a pager thread that handles the page faults of the
 *  newly-created kthread.
 *
 *  The kthread attribute initialization function encodes default values
 *  for some of these parameters.  An attribute must always be initialized
 *  before it may be used, even if the default values are overwritten.
 *  Failure to do so may produce undefined results.
 *
 */
struct okl4_kthread_attr {
#if !defined(NDEBUG)
    okl4_word_t magic;
#endif
    okl4_word_t sp;
    okl4_word_t ip;
    okl4_word_t priority;
    OKL4_MICRO(okl4_kcap_item_t *kcap_item;)
    OKL4_MICRO(okl4_utcb_item_t *utcb_item;)
    OKL4_MICRO(okl4_kspace_t *kspace;)
    OKL4_MICRO(okl4_kcap_t pager;)
};

/**
 *  The okl4_kthread structure is used to represent a kthread, forming 
 *  a wrapper around the kernel thread structure as well as storing the
 *  parameters used to create the kthread and maintaining a list of
 *  kthreads attached to the same kspace.
 *
 *  The following operations may be performed on a kthread:
 *
 *  @li Static memory allocation.
 *
 *  @li Dynamic memory allocation.
 *
 *  @li Creation.
 *
 *  @li Query.
 *
 *  @li Starting up.
 *
 *  @li Deletion.
 *
 */
struct okl4_kthread {
    /* The cap used to create this thread. */
    okl4_kcap_t cap;
    okl4_word_t sp;
    okl4_word_t ip;
    okl4_word_t priority;

    OKL4_MICRO(okl4_kspace_t *kspace;)
    OKL4_MICRO(okl4_utcb_t *utcb;)
    OKL4_MICRO(okl4_kcap_t pager;)

    okl4_kthread_t *next;
};

/* Static Memory Allocation */

/**
 *  The OKL4_KTHREAD_SIZE() macro is used to determine the amount of memory
 *  required to dynamically allocate a kthread, if the kthread is initialized
 *  with the @a attr attribute, in bytes.
 *
 */
#define OKL4_KTHREAD_SIZE          \
    (sizeof(struct okl4_kthread))

/**
 *  The OKL4_KTHREAD_DECLARE() macro is used to declare and statically
 *  allocate memory for a kthread. The resulting @a uninitialized kthread
 *  with an appropriate amount of memory is returned using the @a name argument.
 *
 *  This macro only handles memory allocation. The initialization function
 *  must be called separately.
 *
 */
#define OKL4_KTHREAD_DECLARE(name)                       \
   okl4_word_t __##name[OKL4_KTHREAD_SIZE];           \
   static okl4_kthread_t *(name) = (okl4_kthread_t *)(void *)__##name

/* Dynamic Memory Allocation */

/**
 *  The OKL4_KTHREAD_SIZE_ATTR() macro is used to determine the amount of 
 *  memory required to dynamically allocate a kthread, if the kthread is 
 *  initialized with the attribute specified using the @a attr argument,
 *  in bytes.
 *
 */
#define OKL4_KTHREAD_SIZE_ATTR(attr)  sizeof(struct okl4_kthread)

/* API Interface */

/**
 *  This function is used to create a new @a kthread with the
 *  the parameters encoded in the attribute specified by the @a attr argument.
 *
 *  This function expects the @a attr attribute to encode a valid cap (to use
 *  as its kthread id), a valid kspace for the kthread to attach to, a valid
 *  utcb, sp, ip, and thread priority. This function will allocate kernel
 *  resources for the thread, and requires that the given stack pointer is
 *  valid and contains at least 32 bytes of storage space.
 *
 *  This function invokes the L4_ThreadControl() system call to
 *  create a kernel thread with parameters encoded in the @a attr
 *  attribute.
 *
 *  No instructions at the given instruction pointer will be executed until
 *  okl4_kthread_start() is called.
 *
 *  This function requires the following arguments:
 *
 *  @param kthread
 *    kthread to be created. This argument is used to return 
 *    the newly created kthread. 
 *
 *  @param attr
 *    Attribute containing the parameter of the kthread.
 *
 *  This function returns the following error codes:
 *
 *  @retval OKL4_OK
 *    The creation was successful.
 *
 *  @retval others
 *    The return value from the kernel.
 *
 *  Please refer to the @a OKL4 @a Reference @a Manual for a more detailed explanation
 *  of the L4_ThreadControl() system call.
 *
 */
int okl4_kthread_create(okl4_kthread_t *kthread,
        okl4_kthread_attr_t *attr);

/**
 *  This function is used to start the execution of an already created @a
 *  kthread.
 *
 *  This function requires the following arguments:
 *
 *  @param kthread
 *    The kthread to start.
 *
 */
void okl4_kthread_start(okl4_kthread_t *kthread);

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_kthread_delete() function is used to delete a kthread specified 
 *  by the @a kthread argument. This function invokes the L4_ThreadControl() 
 *  system call to delete the specified kernel thread.
 *
 *  This function returns the following error conditions:
 *
 *  @retval OKL4_OK
 *    The deletion was successful.
 *
 *  @retval others
 *    The return value from L4_ThreadControl().
 *
 *  Please refer to the @a OKL4 @a Reference @a Manual for a more detailed explanation
 *  of the L4_ThreadControl() system call.
 *
 */
void okl4_kthread_delete(okl4_kthread_t *kthread);
#endif

#if defined(OKL4_KERNEL_NANO)
/**
 *  This function causes the current thread to block until the given thread
 *  exits.
 *
 *  This function requires the following arguments:
 *
 *  @param kthread
 *    The kthread to wait for.
 *
 *  This function returns the following error conditions:
 *
 *  @retval OKL4_OK
 *    The given thread has successfully exited.
 *
 *  @retval OKL4_INVALID_THREAD
 *    The given thread does not exist.
 */
int okl4_kthread_join(okl4_kthread_t *kthread);
#endif

#if defined(OKL4_KERNEL_NANO)
/**
 *  This function causes the current thread to halt execution.
 *
 *  This function does not return.
 */
NORETURN void okl4_kthread_exit(void);
#endif

/* Attribute accessor functions. */

/**
 *  The okl4_kthread_attr_init() function is used to initialize the kthread
 *  attribute, @a attr.  This function must be invoked on the attribute before it is
 *  used for kthread creation.
 *
 */
INLINE void okl4_kthread_attr_init(okl4_kthread_attr_t *attr);

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_kthread_attr_setspace() function is used to encode a kthread
 *  attribute @a attr with an associated @a kspace.  Creating a kthread
 *  with this attribute will cause it to belong to the kspace specified 
 *  by the @a kspace arguement.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    Attribute to be encoded.
 *
 *  @param kspace
 *    kspace to be encoded.
 *
 */
INLINE void okl4_kthread_attr_setspace(okl4_kthread_attr_t *attr,
        okl4_kspace_t *kspace);
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_kthread_attr_setcapitem() function is used to encode a kthread
 *  attribute specified by the @a attr argument with a capability that 
 *  serves as the identifier for the newly created kthread. 
 *  The capability specified by the @a cap argument must be managed by a clist owned by the
 *  current calling cell.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    Attribute to be encoded.
 *
 *  @param cap
 *    Capability to be encoded.
 *
 */
INLINE void okl4_kthread_attr_setcapitem(okl4_kthread_attr_t *attr,
        okl4_kcap_item_t *cap);
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_kthread_attr_setutcbitem() function is used to encode a
 *  kthread attribute @a attr with a utcb.  This specifies the utcb
 *  that the kthread will use as its user thread control block. The utcb
 *  specified by the @a utcb argument  must be a valid utcb residing within 
 *  the utcb area of the kspace to which the newly created kthread will belong.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    Attribute to be encoded.
 *
 *  @param utcb
 *    utcb to be encoded.
 *
 */
INLINE void okl4_kthread_attr_setutcbitem(okl4_kthread_attr_t *attr,
        okl4_utcb_item_t *utcb);
#endif /* OKL4_KERNEL_MICRO */

/**
 *  The okl4_kthread_attr_setspip() function is used to encode a kthread
 *  attribute @a attr with the stack pointer, @a sp, and the instruction pointer,
 *  @a ip.  
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    Attribute to be encoded.
 *
 *  @param sp
 *    The initial stack pointer of the newly created kthread to be encoded.
 *
 *  @param ip
 *    The initial instruction pointer of the newly created kthread to be encoded.
 *
 */
INLINE void okl4_kthread_attr_setspip(okl4_kthread_attr_t *attr,
        okl4_word_t sp, okl4_word_t ip);

/**
 *  The okl4_kthread_attr_setpriority() function is used to encode a
 *  kthread attribute @a attr with a scheduling priority specified by the 
 *  @a priority argument.   
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    Attribute to be encoded.
 *
 *  @param priority
 *    Priority to be encoded.
 *
 */
INLINE void okl4_kthread_attr_setpriority(okl4_kthread_attr_t *attr,
        okl4_word_t priority);

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_kthread_attr_setpager() function is used to encode a kthread
 *  attribute @a attr with a pager thread specified by the @a pager argument. 
 *  This specifies that when the newly created kthread page faults, 
 *  the specified pager thread will be responsible for handling the page fault.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    Attribute to be encoded.
 *
 *  @param pager
 *    Pager thread to be encoded in the attribute.
 *
 */
INLINE void okl4_kthread_attr_setpager(okl4_kthread_attr_t *attr, okl4_kcap_t pager);
#endif /* OKL4_KERNEL_MICRO */

/**
 *  The okl4_kthread_getkcap() function is used to retrieve the kcap item
 *  of the kthread specified by the @a kthread argument.  
 *
 */
INLINE okl4_kcap_t okl4_kthread_getkcap(okl4_kthread_t *kthread);

#if defined(OKL4_KERNEL_MICRO)
/**
 *  The okl4_kthread_getpager() function is used to retrieve the kcap item
 *  of a kthread.  This kcap item is the identifier for the pager of the 
 *  specified kthread.
 *
 */
INLINE okl4_kcap_t okl4_kthread_getpager(okl4_kthread_t *kthread);
#endif /* OKL4_KERNEL_MICRO */

/* Inlined Methods */

INLINE void
okl4_kthread_attr_init(okl4_kthread_attr_t *attr)
{
    assert(attr != NULL);

    OKL4_SETUP_MAGIC(attr, OKL4_MAGIC_KTHREAD_ATTR);
    attr->sp = ~0UL;
    attr->ip = ~0UL;
    attr->priority = 0;

    OKL4_MICRO(attr->kcap_item = NULL;)
    OKL4_MICRO(attr->utcb_item = NULL;)
    OKL4_MICRO(attr->kspace = NULL;)
    OKL4_MICRO(attr->pager.raw = L4_nilthread.raw;)
}

#if defined(OKL4_KERNEL_MICRO)
INLINE void
okl4_kthread_attr_setspace(okl4_kthread_attr_t *attr,
        okl4_kspace_t *kspace)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_KTHREAD_ATTR);

    attr->kspace = kspace;
}
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
INLINE void
okl4_kthread_attr_setcapitem(okl4_kthread_attr_t *attr,
        okl4_kcap_item_t *kcap_item)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_KTHREAD_ATTR);

    attr->kcap_item = kcap_item;
}
#endif /* OKL4_KERNEL_MICRO */

#if defined(OKL4_KERNEL_MICRO)
INLINE void
okl4_kthread_attr_setutcbitem(okl4_kthread_attr_t *attr,
        okl4_utcb_item_t *utcb_item)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_KTHREAD_ATTR);

    attr->utcb_item = utcb_item;
}
#endif /* OKL4_KERNEL_MICRO */

INLINE void okl4_kthread_attr_setspip(okl4_kthread_attr_t *attr,
        okl4_word_t sp, okl4_word_t ip)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_KTHREAD_ATTR);

    attr->sp = sp;
    attr->ip = ip;
}

INLINE void okl4_kthread_attr_setpriority(okl4_kthread_attr_t *attr,
        okl4_word_t priority)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_KTHREAD_ATTR);

    attr->priority = priority;
}

#if defined(OKL4_KERNEL_MICRO)
INLINE void
okl4_kthread_attr_setpager(okl4_kthread_attr_t *attr, okl4_kcap_t pager)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_KTHREAD_ATTR);

    attr->pager = pager;
}
#endif /* OKL4_KERNEL_MICRO */

INLINE okl4_kcap_t
okl4_kthread_getkcap(okl4_kthread_t *kthread)
{
    assert(kthread != NULL);

    return kthread->cap;
}

#if defined(OKL4_KERNEL_MICRO)
INLINE okl4_kcap_t
okl4_kthread_getpager(okl4_kthread_t *kthread)
{
    assert(kthread != NULL);

    return kthread->pager;
}
#endif /* OKL4_KERNEL_MICRO */

#endif /* !__OKL4_KTHREAD_H__ */
