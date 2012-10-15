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
 * Description: Capability management
 */

#include <l4.h>
#include <debug.h>
#include <tcb.h>
#include <caps.h>
#include <clist.h>
#include <kmemory.h>
#include <kdb/tracepoints.h>

DECLARE_TRACEPOINT(SYSCALL_CAP_CONTROL);

SYS_CAP_CONTROL(clistid_t clist_id, cap_control_t control)
{
    LOCK_PRIVILEGED_SYSCALL();
    PROFILE_START(sys_cap_ctrl);
    continuation_t continuation = ASM_CONTINUATION;
    kmem_resource_t *kresource;

    TRACEPOINT (SYSCALL_CAP_CONTROL,
                printf("SYS_CAP_CONTROL: clist=%d, control=%p\n",
                    clist_id.get_clistno(), control.get_raw()));

    tcb_t *current = get_current_tcb();
    tcb_t *tcb_locked;
    clist_t * clist;

    clist = get_current_space()->lookup_clist(clist_id);

    switch (control.get_op())
    {
    case cap_control_t::op_create_clist:
        {
            word_t entries, alloc_size;

            /* Check that provided clist-id is valid */
            if (EXPECT_FALSE(!get_current_space()->clist_range.is_valid(
                             clist_id.get_clistno())))
            {
                current->set_error_code (EINVALID_CLIST);
                goto error_out;
            }

            /* Check for non-existing clist. */
            if (EXPECT_FALSE(clist != NULL))
            {
                current->set_error_code(EINVALID_CLIST);
                goto error_out;
            }

            kresource = get_current_kmem_resource();

            entries = current->get_mr(0);
            alloc_size = (word_t)addr_align_up((addr_t)(sizeof(cap_t) * entries + sizeof(clist_t)), KMEM_CHUNKSIZE);

            clist_t* new_list = (clist_t*) kresource->alloc(kmem_group_clist, alloc_size, true);
            if (new_list == NULL)
            {
                current->set_error_code(ENO_MEM);
                goto error_out;
            }

            new_list->init(entries);

            get_clist_list()->add_clist(clist_id, new_list);
            //printf("Create clist id: %ld size: %ld\n", clist_id.get_raw(), entries);
        }
        break;
    case cap_control_t::op_delete_clist:
        {
            word_t entries, alloc_size;

            /* Check that a valid clist was provided. */
            if (EXPECT_FALSE(clist == NULL))
            {
                current->set_error_code(EINVALID_CLIST);
                goto error_out;
            }

            /* Check for empty clist. */
            if (EXPECT_FALSE(clist->get_space_count() != 0))
            {
                current->set_error_code(ECLIST_NOT_EMPTY);
                goto error_out;
            }

            if (EXPECT_FALSE(!clist->is_empty()))
            {
                current->set_error_code(ECLIST_NOT_EMPTY);
                goto error_out;
            }

            entries = clist->num_entries();

            get_clist_list()->remove_clist(clist_id);

            kresource = get_current_kmem_resource();

            alloc_size = (word_t)addr_align_up((addr_t)(sizeof(cap_t) * entries + sizeof(clist_t)), KMEM_CHUNKSIZE);
            kresource->free(kmem_group_clist, clist, alloc_size);
            //printf("Deleted clist: %ld\n", clist_id.get_raw());
        }
        break;
    case cap_control_t::op_copy_cap:
        {
            capid_t src_id = capid(current->get_mr(0));
            clistid_t target_list_id = clistid(current->get_mr(1));
            capid_t target_id = capid(current->get_mr(2));
            clist_t *target_list;

            /* Check that a valid clist was provided. */
            if (EXPECT_FALSE(clist == NULL))
            {
                current->set_error_code(EINVALID_CLIST);
                goto error_out;
            }

            target_list = get_current_space()->lookup_clist(target_list_id);

            /* Check that a valid target clist was provided. */
            if (EXPECT_FALSE(target_list == NULL)) {
                current->set_error_code(EINVALID_CLIST);
                goto error_out;
            }

            /* Ensure target_id is within range */
            if (EXPECT_FALSE(!target_list->is_valid(target_id))) {
                current->set_error_code(EINVALID_CAP);
                goto error_out;
            }

            tcb_locked = clist->lookup_thread_cap_locked(src_id);
            if (EXPECT_FALSE(tcb_locked == NULL)) {
                current->set_error_code(EINVALID_CAP);
                goto error_out;
            }

            /* try to insert the cap, will fail if slot is used */
            if (!target_list->add_ipc_cap(target_id, tcb_locked)) {
                current->set_error_code(EINVALID_CAP);
                goto error_out_locked;
            }
            //printf("Add IPC cap: %lx to list %ld   - (copy from %lx  in list %ld)\n", target_id.get_raw(), target_list_id.get_raw(), src_id.get_raw(), clist_id.get_raw());
            tcb_locked->unlock_read();
        }
        break;
    case cap_control_t::op_delete_cap:
        {
            capid_t target_id = capid(current->get_mr(0));

            /* Check for valid clist id. */
            if (EXPECT_FALSE(clist == NULL))
            {
                current->set_error_code(EINVALID_CLIST);
                goto error_out;
            }

            /* Ensure target_id is within range */
            if (EXPECT_FALSE(!clist->is_valid(target_id))) {
                current->set_error_code(EINVALID_CAP);
                goto error_out;
            }

            /* try to remove the cap, will fail if not ipc-cap or invalid */
            if (!clist->remove_ipc_cap(target_id)) {
                current->set_error_code(EINVALID_CAP);
                goto error_out;
            }

            //printf("Del IPC cap: %lx in list %ld\n", target_id.get_raw(), clist_id.get_raw());
        }
        break;
    default:
        current->set_error_code(EINVALID_PARAM);
        goto error_out;
    }

    PROFILE_STOP(sys_cap_ctrl);
    UNLOCK_PRIVILEGED_SYSCALL();
    return_cap_control(1, continuation);

error_out_locked:
    tcb_locked->unlock_read();
error_out:
    PROFILE_STOP(sys_cap_ctrl);
    UNLOCK_PRIVILEGED_SYSCALL();
    return_cap_control(0, continuation);
}

