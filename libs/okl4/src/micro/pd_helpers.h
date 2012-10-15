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

#ifndef __PD_HELPERS_H__
#define __PD_HELPERS_H__

#include <okl4/types.h>

/* Ensure that 'b' is true if and only if 'a' is true. */
#define IFF(a, b) (!!(a) == !!(b))

/* If 'a' is true, ensure 'b' is true. */
#define IMPLIES(a, b) (!(a) || (b))

/* Either 'a' is true or 'b' is true but not both. */
#define XOR(a, b) ((!(a) && (b)) || ((a) && !(b)))

/* Round the given number 'n' up to the next multiple of 'd'. */
#define ROUND_UP(n, d) ((((n) + (d) - 1) / (d)) * (d))

/*
 * Insert a generic memory container (e.g. memsec or zone) into the specified
 * list. Also tests that the address of the new element does not conflict with
 * addresses of existing elements.
 */
int _okl4_mem_container_list_insert(_okl4_mcnode_t **head,
        _okl4_mcnode_t *node);

/*
 * Remove a generic memory container (e.g. memsec or zone) from the specified
 * list. Returns a pointer to the node that the container was held in.
 */
_okl4_mcnode_t *_okl4_mem_container_list_remove(_okl4_mcnode_t **head,
        _okl4_mem_container_t *container);

/*
 * Determine if two containers overlap.
 */
int _okl4_is_container_overlap(_okl4_mem_container_t *a,
        _okl4_mem_container_t *b);

/*
 * Determine if container 'small' is contained completely within container
 * 'large'.
 */
int _okl4_is_contained_in(_okl4_mem_container_t *small,
        _okl4_mem_container_t *large);

/*
 * Determine if the point 'a' falls within the interval '[x,y)'.
 */
int _okl4_is_point_in_range(okl4_word_t a, okl4_word_t x, okl4_word_t y);

/*
 * Premap the given memsec into the kspace with given kspaceid.
 */
void _okl4_kspace_premap_memsec(okl4_kspaceid_t kspaceid, okl4_memsec_t *memsec,
                                okl4_word_t perm_mask);

/*
 * Unmap the given memsec from the kspace with given kspaceid.
 */
void _okl4_kspace_unmap_memsec(okl4_kspaceid_t kspaceid, okl4_memsec_t *memsec);

/*
 * Premap the given zone into the kspace with the given kspaceid.
 */
void _okl4_kspace_premap_zone(okl4_kspaceid_t kspaceid, okl4_zone_t *zone,
          okl4_word_t perms);

/*
 * Unmap the given zone from the kspace with the given ksapceid.
 */
void _okl4_kspace_unmap_zone(okl4_kspaceid_t kspaceid, okl4_zone_t *zone);

/*
 * Given a container list, find the container that the given 'vaddr' falls in.
 */
_okl4_mem_container_t * _okl4_get_container_from_vaddr(_okl4_mcnode_t *first,
        okl4_word_t vaddr, _okl4_mem_container_type_t *type,
        okl4_word_t *attach_perms);

/* 
 * UTCB memsection callbacks.
 * */
int
_okl4_utcb_memsec_lookup(okl4_memsec_t *memsec, okl4_word_t vaddr,
        okl4_physmem_item_t *map_item, okl4_word_t *dest_vaddr);

int
_okl4_utcb_memsec_map(okl4_memsec_t *memsec, okl4_word_t vaddr,
        okl4_physmem_item_t *map_item, okl4_word_t *dest_vaddr);

void
_okl4_utcb_memsec_destroy(okl4_memsec_t *ms);

#endif /* ! __PD_HELPERS_H__ */

