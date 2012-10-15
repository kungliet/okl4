/*
 * Copyright (c) 2005-2006, National ICT Australia (NICTA)
 */
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
/*
 * Description:   Mapping implementation
 */

#include <l4.h>
#include <debug.h>
#include <kmemory.h>
#include <kdb/tracepoints.h>
#include <map.h>
#include <tcb.h>
#include <syscalls.h>

#if 1
#define TRACE_MAP(x...)
#else
#define TRACE_MAP   printf
#endif


DECLARE_TRACEPOINT (SYSCALL_MAP_CONTROL);

SYS_MAP_CONTROL (spaceid_t space_id, word_t control)
{
    LOCK_PRIVILEGED_SYSCALL();
    PROFILE_START(sys_map_ctrl);
    continuation_t continuation = ASM_CONTINUATION;
    NULL_CHECK(continuation);
    map_control_t ctrl;
    tcb_t *current;
    space_t *space, *curr_space;
    word_t count;
    kmem_resource_t *kresource;

    ctrl.set_raw(control);

    TRACEPOINT (SYSCALL_MAP_CONTROL,
                printf("SYS_MAP_CONTROL: space=%d, control=%p "
                       "(n = %d, %c%c)\n", space_id.get_spaceno(),
                       control, ctrl.highest_item(),
                       ctrl.is_modify() ? 'M' : '~',
                       ctrl.is_query() ? 'Q' : '~'));

    current = get_current_tcb();

    curr_space = get_current_space();
    space = curr_space->lookup_space(space_id);
    /*
     * Check for valid space id and that either the caller is the root task
     * or the destination space has the current space as its pager
     */
    if (EXPECT_FALSE(space == NULL))
    {
        current->set_error_code (EINVALID_SPACE);
        TRACE_MAP("invalid_space: %ld\n", space_id.get_raw());
        goto error_out;
    }

    if (EXPECT_FALSE(!is_kresourced_space(curr_space))) {
        current->set_error_code (EINVALID_SPACE);
        TRACE_MAP("no resources: %ld\n", space_id.get_raw());
        goto error_out;
    }

    if (EXPECT_FALSE(curr_space->phys_segment_list == NULL))
    {
        current->set_error_code (EINVALID_PARAM);
        TRACE_MAP("no segments: %ld\n", space_id.get_raw());
        goto error_out;
    }

    count = ctrl.highest_item()+1;

    /* check for message overflow !! */
    if (EXPECT_FALSE((count*3) > IPC_NUM_MR))
    {
        current->set_error_code (EINVALID_PARAM);
        TRACE_MAP("overflow: %ld, count: %d\n", space_id.get_raw(), count);
        goto error_out;
    }

    kresource = get_current_kmem_resource();

    for (word_t i = 0; i < count; i++)
    {
        phys_desc_t tmp_base;
        perm_desc_t tmp_perm;

#if defined(CONFIG_ARM_V5)
        if (ctrl.is_window())
        {
            fpage_t fpg;
            space_t * target;
            spaceid_t target_sid;
            bool r;

            target_sid = spaceid(current->get_mr(i*2));
            fpg.raw = current->get_mr(i*2+1);

            target = get_current_space()->lookup_space(target_sid);
            // Check for valid space id
            if (EXPECT_FALSE (target == NULL))
            {
                get_current_tcb ()->set_error_code (EINVALID_SPACE);
                TRACE_MAP("invalid space: dest: %ld\n", target_sid.get_raw());
                goto error_out;
            }

            if (space->is_sharing_domain(target)) {
                // printf("map: map window : %S -> %S - fpg: %lx\n", space, target, fpg.raw);
                r = space->window_share_fpage(target, fpg, kresource);

                if (r == false) {
                    /* Error code set in window_share_fpage() */
                    goto error_out;
                }
            } else {
                //printf("No permission - domain not shared!!\n");
                get_current_tcb ()->set_error_code (EINVALID_SPACE);
                goto error_out;
            }

            continue;
        }
#endif

        // Read the existing pagetable
        // Query will be removed soon.
        if (ctrl.is_query())
        {
            fpage_t fpg;
            virt_desc_t virt;
            segment_desc_t seg;

            tmp_perm.clear();

            virt.set_raw(current->get_mr((i * 3) + 2));
            seg.set_raw(current->get_mr(i * 3));

            fpg.raw = 0;
            fpg.x.base = virt.get_base() >> 10;

            space->read_fpage(fpg, &tmp_base, &tmp_perm);
        }

        // Modify the page table
        if (ctrl.is_modify())
        {
            phys_desc_t base;
            fpage_t fpg;
            virt_desc_t virt;
            segment_desc_t seg;
            size_desc_t size;
            phys_segment_t *segment;
            bool valid_bit_set;

            seg.set_raw(current->get_mr(i * 3));
            size.set_raw(current->get_mr((i * 3) + 1));
            virt.set_raw(current->get_mr((i * 3) + 2));

            if (size.get_valid() == 1) {
                valid_bit_set = true;
            } else {
                valid_bit_set = false;
            }

            TRACEPOINT (SYSCALL_MAP_CONTROL,
                printf("SYS_MAP_CONTROL: (segment=%ld, offset=0x%lx) => "
                       "(virt=0x%lx, page_size=0x%lx)\n",
                       seg.get_segment(),
                       seg.get_offset(),
                       virt.get_base(),
                       size.get_page_size()));

            /* We don't support multiple page batching yet */
            if (size.get_num_pages() != 1) {
                get_current_tcb()->set_error_code(EINVALID_PARAM);
                TRACE_MAP("no batching\n");
                goto error_out;
            }

            fpg.raw = size.get_raw() & 0x7;
            fpg.x.size = size.get_page_size();
            fpg.x.base = virt.get_base() >> 10;

            base.set_attributes(((word_t)virt.get_attributes() & 0x3f));

            /*
             * Check for valid fpage
             */
            if (EXPECT_FALSE(fpg.get_address() != fpg.get_base()))
            {
                get_current_tcb ()->set_error_code (EINVALID_PARAM);
                TRACE_MAP("not aligned\n");
                goto error_out;
            }

            if (!valid_bit_set)
            {
                space->unmap_fpage(fpg, false, kresource);
            }
            else
            {
                /* We only check this if we are mapping. */
                segment = curr_space->phys_segment_list->lookup_segment(seg.get_segment());
//printf("map: %p seg: %d, offset: %lx, s=%d, a=%x\n", virt.get_base(), seg.get_segment(), seg.get_offset(), size.get_page_size(), virt.get_attributes());
                if (EXPECT_FALSE(!segment || !segment->is_contained(seg.get_offset(),
                                                        (1 << size.get_page_size())))) {
                    get_current_tcb()->set_error_code(EINVALID_PARAM);
                    TRACE_MAP("not contained\n");
                    goto error_out;
                }

                if (EXPECT_FALSE((segment->get_rwx() & size.get_rwx()) !=
                                                         size.get_rwx())) {
                    get_current_tcb()->set_error_code(EINVALID_PARAM);
                    TRACE_MAP("bad rights\n");
                    goto error_out;
                }

                if (EXPECT_FALSE(!segment->attribs_allowed(virt.get_attributes()))) {
                    get_current_tcb()->set_error_code(EINVALID_PARAM);
                    TRACE_MAP("bad attributes: %ld\n", virt.get_attributes());
                    goto error_out;
                }

                base.set_base(segment->get_base() + seg.get_offset());

                if (EXPECT_FALSE(!space->map_fpage(base, fpg, kresource)))
                {
                    TRACE_MAP("map error\n");
                    /* Error code set in map_fpage */
                    goto error_out;
                }
            }
        }

        /*
         * Query will be removed from MapControl soon.
         */
        if (ctrl.is_query())
        {
            segment_desc_t seg;
            virt_desc_t virt;
            word_t query_desc;

            seg.set_offset(tmp_base.get_base());

            /* 
             * We don't have a query descriptor
             * because queries will be removed 
             */
            query_desc = 0UL;
            /* Setting the permissions and the page size */
            query_desc |= (tmp_perm.get_raw() & 0x3ff);
            /* Setting the references */
            query_desc |= tmp_perm.get_reference() << 10;

            virt.set_attributes((int)tmp_base.get_attributes());

            current->set_mr((i*3), seg.get_raw());
            current->set_mr((i*3)+1, query_desc);
            current->set_mr((i*3)+2, virt.get_raw());
        }

    }

    PROFILE_STOP(sys_map_ctrl);
    UNLOCK_PRIVILEGED_SYSCALL();
    return_map_control(1, continuation);

error_out:
    PROFILE_STOP(sys_map_ctrl);
    UNLOCK_PRIVILEGED_SYSCALL();
    return_map_control(0, continuation);
}
