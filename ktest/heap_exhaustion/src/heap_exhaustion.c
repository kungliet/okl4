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

#include <l4/ipc.h>
#include <l4/message.h>
#include <l4/map.h>
#include <l4e/map.h>
#include <l4/space.h>
#include <l4/types.h>
#include <stdio.h>
#include <okl4/kspaceid_pool.h>
#include <okl4/env.h>

L4_ClistId_t HEAP_EXHAUSTION_CLIST;

/**
 * Utility function for creating address spaces.
 *
 * @param[in] space the space id to use for the new space
 * @param[in] utcb_fpage the memory to be used for UTCBs
 */
static int
create_address_space(L4_SpaceId_t space, L4_Fpage_t utcb_fpage)
{
    L4_Word_t res;

#ifdef NO_UTCB_RELOCATE
    utcb_fpage = L4_Nilpage;
#endif

    res = L4_SpaceControl(space, L4_SpaceCtrl_new, HEAP_EXHAUSTION_CLIST,
            utcb_fpage, 0, NULL);
    return res;
}

/**
 * Setup a series of mappings.
 *
 * @param[in] page_size the page size to use for all the mappings
 * @param[in] n_pages how many pages to use in the sequence
 * @param[in] v the virtual address to start the sequence of mappings
 * @param[in] p the physical address to start the sequence of mappings
 *
 * @return 0 for success and non-zero on error
 */
static int
map_series(L4_SpaceId_t space, L4_Word_t page_size, uintptr_t n_pages, uintptr_t v, uintptr_t p)
{
    int i, res = 1;
    L4_Fpage_t f;

    for (i = 0; i < n_pages; i++, p += page_size) {
        f = L4_Fpage(v + i*page_size, page_size);
        f.X.rwx = L4_FullyAccessible;
        res = l4e_map_fpage(space, f, p, L4_DefaultMemory);
        if(res != 1)
            break;
    }

    return res;
}

int
main(void)
{
    int i = 0;
    int res = 1;
    L4_SpaceId_t space;
    okl4_allocator_attr_t attr;
    L4_Word_t end, result;
    L4_CapId_t sender;
    L4_MsgTag_t tag;
    L4_Msg_t msg;
    struct okl4_bitmap_allocator *spaceid_pool = okl4_env_get("MAIN_SPACE_ID_POOL");

    HEAP_EXHAUSTION_CLIST = L4_ClistId(*(OKL4_ENV_GET_MAIN_CLISTID("MAIN_CLIST_ID")));
    while(res == 1)
    {
        result = okl4_kspaceid_allocany(spaceid_pool, &space);
        if (result != OKL4_OK) {
            printf("Failed to allocate space id\n");
        }
        res = create_address_space(space,
                                   L4_Fpage(0xb10000, 0x1000));

        if (res != 1) {
            printf("SpaceControl failed, space id=%lu, Error code: %lu\n", space.space_no, L4_ErrorCode());
            break;
        }
        /* 2gb mapping of 512k pages*/
        res = map_series(space, 0x80000, 4096, 0, 0);
        i++;
    }

    printf("Created %d address spaces\n", i);
    if (i == 0) {
        okl4_kspaceid_free(spaceid_pool, space);
        res = 0;
        goto ipc_ktest;
    }

    /* clean up */
    okl4_kspaceid_getattribute(spaceid_pool, &attr);
    end = attr.base + attr.size;
    for (i = attr.base; i < end; i++)
    {
        L4_SpaceId_t id = L4_SpaceId(i);
        if (okl4_kspaceid_isallocated(spaceid_pool, id))
        {
            L4_SpaceControl(id, L4_SpaceCtrl_delete,
                            HEAP_EXHAUSTION_CLIST, L4_Nilpage, 0, NULL);
            okl4_kspaceid_free(spaceid_pool, id);
        }
    }
    res = 1;

ipc_ktest:
    tag = L4_Wait(&sender);
    L4_MsgClear(&msg);
    L4_MsgAppendWord(&msg, (L4_Word_t)res);
    L4_MsgLoad(&msg);
    L4_Reply(sender);
    assert(L4_IpcSucceeded(tag));

    L4_WaitForever();
}

