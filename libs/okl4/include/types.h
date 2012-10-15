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

#ifndef __OKL4__TYPES_H__
#define __OKL4__TYPES_H__

#include <okl4/arch/config.h>
#include <okl4/kernel.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>

#if defined(OKL4_KERNEL_MICRO)
#include <l4/types.h>
#include <l4/config.h>
#include <l4/utcb.h>
#include <l4/cache_attribs.h>
#elif defined(OKL4_KERNEL_NANO)
#include <nano/nano.h>
#else
#error "Unknown kernel configuration."
#endif

/**
 * Define INLINE macro.
 */
#ifndef INLINE
#define INLINE static inline
#endif

/**
 *  @file
 *
 *  The types.h Header.
 *
 *  The types.h header provides the definitions for basic data types
 *  that is used throughout libokl4.  These include basic integer types,
 *  native unsigned word types, and pointer types. This header also
 *  contains some common magic numbers.
 *
 */

/**
 *  The macro OKL4_WORD_T_BIT defines the number of bits in a
 *  machine-native unsigned word.
 *
 */
#if defined(OKL4_KERNEL_MICRO)
#define OKL4_WORD_T_BIT WORD_T_BIT
#else
#define OKL4_WORD_T_BIT (sizeof(okl4_word_t) * 8)
#endif

/**
 *  The macro OKL4_POISON defines a magic value to represent uninitialized
 *  memory.  This is used to assist in identifying bugs caused due to using
 *  uninitialized memory.  The value of this macro is currently defined to
 *  be 0xdeadbeef.
 *
 */
#define OKL4_POISON 0xdeadbeefUL

/**
 * Default page size to use when none is specified by the user.
 */
#if defined(OKL4_KERNEL_MICRO)
#define OKL4_DEFAULT_PAGESIZE   (1 << __L4_MIN_PAGE_BITS)
#else
#define OKL4_DEFAULT_PAGESIZE   (1 << 12) /* FIXME */
#endif

/**
 * Magic checking is used to ensure that users of the library initialise
 * attributes before attempting to use them. They are compiled away when
 * debugging is disabled.
 */
#if !defined(NDEBUG)
#define OKL4_SETUP_MAGIC(attr, val)                                    \
    do {                                                               \
        (attr)->magic = (val);                                         \
    } while (0)
#define OKL4_CHECK_MAGIC(attr, val)                                    \
    do {                                                               \
        assert((attr)->magic == (val));                                \
    } while (0)
#else /* NDEBUG */
#define OKL4_SETUP_MAGIC(attr, val)  do { } while (0)
#define OKL4_CHECK_MAGIC(attr, val)  do { } while (0)
#endif /* NDEBUG */

/*
 * Magic values to indicate that an object has been initialised.
 */
#define OKL4_MAGIC_ALLOCATOR_ATTR         0x778d3577UL
#define OKL4_MAGIC_BARRIER_ATTR           0x77dfe277UL
#define OKL4_MAGIC_EXTENSION_ATTR         0x77632677UL
#define OKL4_MAGIC_INTSET_ATTR            0x77f56177UL
#define OKL4_MAGIC_INTSET_DEREGISTER_ATTR 0x77876677UL
#define OKL4_MAGIC_INTSET_REGISTER_ATTR   0x77b26677UL
#define OKL4_MAGIC_KCLIST_ATTR            0x77f6fc77UL
#define OKL4_MAGIC_KSPACE_ATTR            0x778c1a77UL
#define OKL4_MAGIC_KSPACE_MAP_ATTR        0x77831077UL
#define OKL4_MAGIC_KSPACE_UNMAP_ATTR      0x77eb5877UL
#define OKL4_MAGIC_KTHREAD_ATTR           0x771a5977UL
#define OKL4_MAGIC_MUTEX_ATTR             0x77ad3577UL
#define OKL4_MAGIC_MEMSEC_ATTR            0x77ef0b77UL
#define OKL4_MAGIC_PD_ATTACH_ATTR         0x77a99777UL
#define OKL4_MAGIC_PD_ATTR                0x77f90877UL
#define OKL4_MAGIC_PD_DICT_ATTR           0x77898a77UL
#define OKL4_MAGIC_PD_THREAD_CREATE_ATTR  0x77710177UL
#define OKL4_MAGIC_PD_ZONE_ATTACH_ATTR    0x77a3dd77UL
#define OKL4_MAGIC_PHYSMEM_POOL_ATTR      0x77fac577UL
#define OKL4_MAGIC_SEMAPHORE_ATTR         0x7794ba77UL
#define OKL4_MAGIC_STATIC_MEMSEC_ATTR     0x771d8377UL
#define OKL4_MAGIC_UTCB_AREA_ATTR         0x779b7777UL
#define OKL4_MAGIC_VIRTMEM_POOL_ATTR      0x77663777UL
#define OKL4_MAGIC_ZONE_ATTR              0x779e4077UL

/**
 *  The macro OKL4_WEAVED_OBJECT defines a magic value to represent an
 *  object constructed by Elfweaver. This is used in libokl4 object
 *  attributes to help determine whether the object has already been
 *  created and initialized by Elfweaver, in which case no further setup is
 *  neccessary during cell initialization.
 *
 */
#define OKL4_WEAVED_OBJECT ((void *)-1)

/**
 * Memory permissions.
 */
#if defined(OKL4_KERNEL_MICRO)
#define OKL4_PERMS_R                 L4_Readable
#define OKL4_PERMS_W                 L4_Writable
#define OKL4_PERMS_X                 L4_eXecutable
#define OKL4_PERMS_RW                L4_ReadWriteOnly
#define OKL4_PERMS_RX                L4_ReadeXecOnly
#define OKL4_PERMS_RWX               L4_FullyAccessible
#define OKL4_PERMS_FULL              L4_FullyAccessible
#define OKL4_DEFAULT_PERMS           L4_FullyAccessible

#define OKL4_DEFAULT_MEM_ATTRIBUTES  L4_DefaultMemory

#else
#define OKL4_PERMS_R                 4
#define OKL4_PERMS_W                 2
#define OKL4_PERMS_X                 1
#define OKL4_PERMS_RW                6
#define OKL4_PERMS_RX                5
#define OKL4_PERMS_RWX               7
#define OKL4_PERMS_FULL              7
#define OKL4_DEFAULT_PERMS           7

#define OKL4_DEFAULT_MEM_ATTRIBUTES  0

#endif

/**
 * Needed to convert between a _okl4_range_container subtype and it's 'base'
 * type which can be either a memsection or a zone.
 */
typedef enum _okl4_mem_container_type {
    _OKL4_TYPE_MEMSECTION,
    _OKL4_TYPE_ZONE,
    _OKL4_TYPE_EXTENSION
} _okl4_mem_container_type_t;

/**
 *  The okl4_word_t type defines the machine-native unsigned word type.
 *  This type should be used as the default scalar value when interfacing
 *  with libokl4.
 *
 */
#if defined(OKL4_KERNEL_MICRO)
typedef word_t okl4_word_t;

/**
 *  The okl4_utcb_t type defines the user thread control block (utcb)
 *  object.  Usage of the utcb object is further described in utcb.h.
 *
 */
typedef utcb_t okl4_utcb_t;
#else
typedef unsigned long okl4_word_t;
#endif
typedef int8_t    okl4_s8_t;
typedef int16_t   okl4_s16_t;
typedef int32_t   okl4_s32_t;
typedef uint8_t   okl4_u8_t;
typedef uint16_t  okl4_u16_t;
typedef uint32_t  okl4_u32_t;

/**
 *  Kernel resource identifiers.
 *
 *  The types listed above define identifiers for the following kernel
 *  resources, respectively:
 *
 *  @li capability lists (kclist.h);
 *
 *  @li mutexes (kmutexpool.h);
 *
 *  @li address spaces (kspace.h); and
 *
 *  @li capabilities (kclist.h).
 *
 */
#if defined(OKL4_KERNEL_MICRO)
typedef L4_CapId_t okl4_kcap_t;
typedef L4_ClistId_t okl4_kclistid_t;
typedef L4_MutexId_t okl4_kmutexid_t;
typedef L4_SpaceId_t okl4_kspaceid_t;
#else
typedef okl4_word_t okl4_kcap_t;
#endif

/**
 * The null "invalid" cap.
 */
#if defined(OKL4_KERNEL_MICRO)
#define OKL4_NULL_KCAP L4_nilthread
#else
#define OKL4_NULL_KCAP (~0UL)
#endif

/* LibOKL4 Objects. */
struct _okl4_mcnode;
struct _okl4_mem_container;
struct _okl4_pd_thread;
struct okl4_allocator_attr;
struct okl4_barrier;
struct okl4_barrier_attr;
struct okl4_bitmap_allocator;
struct okl4_bitmap_item;
struct okl4_env;
struct okl4_env_device_irqs;
struct okl4_envitem;
struct okl4_env_args;
struct okl4_env_kernel_info_t;
struct okl4_env_segment;
struct okl4_env_segments;
struct okl4_extension;
struct okl4_extension_attr;
struct okl4_extension_token;
struct okl4_irqset;
struct okl4_irqset_attr;
struct okl4_irqset_deregister_attr;
struct okl4_irqset_register_attr;
struct okl4_kcap_item;
struct okl4_kclist;
struct okl4_kclist_attr;
struct okl4_kspace;
struct okl4_kspace_attr;
struct okl4_kspace_map_attr;
struct okl4_kspace_unmap_attr;
struct okl4_kthread;
struct okl4_kthread_attr;
struct okl4_memsec;
struct okl4_memsec_attr;
struct okl4_mutex;
struct okl4_mutex_attr;
struct okl4_pd;
struct okl4_pd_attach_attr;
struct okl4_pd_attr;
struct okl4_pd_dict;
struct okl4_pd_dict_attr;
struct okl4_pd_thread_create_attr;
struct okl4_pd_zone_attach_attr;
struct okl4_physmem_item;
struct okl4_physmem_pagepool;
struct okl4_physmem_pool_attr;
struct okl4_physmem_segpool;
struct okl4_range_allocator;
struct okl4_range_item;
struct okl4_semaphore;
struct okl4_semaphore_attr;
struct okl4_static_memsec;
struct okl4_static_memsec_attr;
struct okl4_utcb;
struct okl4_utcb_area;
struct okl4_utcb_area_attr;
struct okl4_utcb_item;
struct okl4_virtmem_pool;
struct okl4_virtmem_pool_attr;
struct okl4_zone;
struct okl4_zone_attr;

/* LibOKL4 Typedefs. */
typedef struct _okl4_mem_container _okl4_mem_container_t;
typedef struct _okl4_mcnode _okl4_mcnode_t;
typedef struct _okl4_pd_thread _okl4_pd_thread_t;
typedef struct okl4_allocator_attr okl4_allocator_attr_t;
typedef struct okl4_barrier okl4_barrier_t;
typedef struct okl4_barrier_attr okl4_barrier_attr_t;
typedef struct okl4_bitmap_allocator okl4_bitmap_allocator_t;
typedef struct okl4_bitmap_allocator okl4_kclistid_pool_t;
typedef struct okl4_bitmap_allocator okl4_kmutexid_pool_t;
typedef struct okl4_bitmap_allocator okl4_kspaceid_pool_t;
typedef struct okl4_bitmap_item okl4_bitmap_item_t;
typedef struct okl4_env okl4_env_t;
typedef struct okl4_envitem okl4_envitem_t;
typedef struct okl4_env_args okl4_env_args_t;
typedef struct okl4_env_device_irqs okl4_env_device_irqs_t;
typedef struct okl4_env_kernel_info okl4_env_kernel_info_t;
typedef struct okl4_env_segment okl4_env_segment_t;
typedef struct okl4_env_segments okl4_env_segments_t;
typedef struct okl4_extension okl4_extension_t;
typedef struct okl4_extension_attr okl4_extension_attr_t;
typedef struct okl4_extension_token okl4_extension_token_t;
typedef struct okl4_irqset okl4_irqset_t;
typedef struct okl4_irqset_attr okl4_irqset_attr_t;
typedef struct okl4_irqset_deregister_attr okl4_irqset_deregister_attr_t;
typedef struct okl4_irqset_register_attr okl4_irqset_register_attr_t;
typedef struct okl4_kcap_item okl4_kcap_item_t;
typedef struct okl4_kclist okl4_kclist_t;
typedef struct okl4_kclist_attr okl4_kclist_attr_t;
typedef struct okl4_kspace okl4_kspace_t;
typedef struct okl4_kspace_attr okl4_kspace_attr_t;
typedef struct okl4_kspace_map_attr okl4_kspace_map_attr_t;
typedef struct okl4_kspace_unmap_attr okl4_kspace_unmap_attr_t;
typedef struct okl4_kthread okl4_kthread_t;
typedef struct okl4_kthread_attr okl4_kthread_attr_t;
typedef struct okl4_memsec okl4_memsec_t;
typedef struct okl4_memsec_attr okl4_memsec_attr_t;
typedef struct okl4_mutex okl4_mutex_t;
typedef struct okl4_mutex_attr okl4_mutex_attr_t;
typedef struct okl4_pd okl4_pd_t;
typedef struct okl4_pd_attach_attr_t okl4_pd_attach_attr_t;
typedef struct okl4_pd_attr okl4_pd_attr_t;
typedef struct okl4_pd_dict okl4_pd_dict_t;
typedef struct okl4_pd_dict_attr okl4_pd_dict_attr_t;
typedef struct okl4_pd_thread_create_attr okl4_pd_thread_create_attr_t;
typedef struct okl4_pd_zone_attach_attr okl4_pd_zone_attach_attr_t;
typedef struct okl4_physmem_item okl4_physmem_item_t;
typedef struct okl4_physmem_pagepool okl4_physmem_pagepool_t;
typedef struct okl4_physmem_pool_attr okl4_physmem_pool_attr_t;
typedef struct okl4_physmem_segpool okl4_physmem_segpool_t;
typedef struct okl4_range_item okl4_range_item_t;
typedef struct okl4_range_item okl4_virtmem_item_t;
typedef struct okl4_range_allocator okl4_range_allocator_t;
typedef struct okl4_semaphore okl4_semaphore_t;
typedef struct okl4_semaphore_attr okl4_semaphore_attr_t;
typedef struct okl4_static_memsec okl4_static_memsec_t;
typedef struct okl4_static_memsec_attr okl4_static_memsec_attr_t;
typedef struct okl4_utcb_area okl4_utcb_area_t;
typedef struct okl4_utcb_area_attr okl4_utcb_area_attr_t;
typedef struct okl4_utcb_item okl4_utcb_item_t;
typedef struct okl4_virtmem_pool okl4_virtmem_pool_t;
typedef struct okl4_virtmem_pool_attr okl4_virtmem_pool_attr_t;
typedef struct okl4_zone okl4_zone_t;
typedef struct okl4_zone_attr okl4_zone_attr_t;


#endif /* !__OKL4__TYPES_H__ */
