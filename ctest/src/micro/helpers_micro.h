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

#ifndef _CTEST_HELPERS_MICRO_H_
#define _CTEST_HELPERS_MICRO_H_

#include <stdio.h>
#include <l4/config.h>
#include <l4/ipc.h>
#include <ctest/ctest.h>

#include <okl4/memsec.h>
#include <okl4/zone.h>
#include <okl4/static_memsec.h>
#include <okl4/physmem_item.h>
#include <okl4/pd.h>
#include <okl4/pd_dict.h>
#include <okl4/kspace.h>
#include <okl4/bitmap.h>
#include <okl4/kclist.h>
#include <okl4/extension.h>

#define UTCB_ADDR              0x60000000
#define MAX_THREADS            32
#define NO_POOL                ((okl4_virtmem_pool_t *)1)

#define EXTENSION_BASE_ADDR 0x30000000
#define EXTENSION_SIZE      0x01000000

#define ANONYMOUS_BASE        ((okl4_word_t)-1)

/*
 * Micro-specific pools.
 */
extern okl4_physmem_segpool_t *root_physseg_pool;
extern okl4_kclistid_pool_t *root_kclist_pool;
extern okl4_kspaceid_pool_t  *root_kspace_pool;
extern okl4_kclist_t *root_kclist;
extern okl4_memsec_t *ctest_segments[];
extern word_t kernel_phys_base;
extern word_t kernel_phys_end;

/* Memsec wrapper. */
typedef struct ms
{
    okl4_memsec_t *memsec;
    okl4_static_memsec_t static_memsec;
    okl4_virtmem_item_t virt;
    okl4_physmem_item_t phys;
    okl4_virtmem_item_t allocated_virt;
} ms_t;

/* KSpace wrapper. */
typedef struct ks {
    okl4_kclistid_t kclist_id;
    okl4_kspaceid_t kspace_id;
    okl4_kclist_t *kclist;
    okl4_kspace_t *kspace;
    okl4_utcb_area_t *utcb_area;
} ks_t;

/* Static memsec creation / deletion. */
ms_t *create_memsec(okl4_word_t base, okl4_word_t size,
        okl4_virtmem_pool_t *virtmem_pool);
ms_t *create_memsec_with_perms(okl4_word_t base, okl4_word_t size,
        okl4_virtmem_pool_t *virtmem_pool, okl4_word_t perms);

void delete_memsec(ms_t *ms, okl4_virtmem_pool_t *virtmem_pool);

/* Paged memsection creation */
okl4_memsec_t * create_paged_memsec(okl4_virtmem_item_t virt_item);

/* Zone createion */
okl4_zone_t * create_zone(okl4_word_t base, okl4_word_t size);
void delete_zone(okl4_zone_t *zone);

/* KSpace creation / deletion. */
ks_t *create_kspace(void);
void delete_kspace(ks_t *ks);

/* PD creation / deletion. */
okl4_pd_t *create_pd(void);
void delete_pd(okl4_pd_t *pd);

/* Run a thread in a PD */
okl4_kthread_t * run_thread_in_pd(okl4_pd_t *pd,
        okl4_word_t sp, okl4_word_t ip);

/* Extension creation and deletion */
okl4_extension_t * create_extension(okl4_pd_t *base_pd);

/* Attach a zone to a PD. */
void attach_zone(okl4_pd_t *pd, okl4_zone_t *zone);
void attach_zone_perms(okl4_pd_t *pd, okl4_zone_t *zone, okl4_word_t perms);

/* Allocate a virtual memory range. */
void free_virtual_address_range(okl4_range_item_t *item);
okl4_range_item_t * get_virtual_address_range(okl4_word_t size);

/* Send IPC to a thread */
void send_ipc(L4_ThreadId_t dest);

#endif /* !_CTEST_HELPERS_MICRO_H_ */

