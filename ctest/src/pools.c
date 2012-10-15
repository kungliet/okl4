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

#include <ctest/ctest.h>

#include <okl4/range.h>
#include <okl4/virtmem_pool.h>
#include <okl4/env.h>
#include <okl4/static_memsec.h>

#include <stdlib.h>
#include <assert.h>

#define NUM_SEGMENTS 20

okl4_virtmem_pool_t *root_virtmem_pool;
okl4_pd_t *root_pd;
okl4_kthread_t *root_thread;

okl4_memsec_t *ctest_segments[NUM_SEGMENTS];

/*
 * Create a static memsection from a weaved segment description.
 */
static okl4_memsec_t *
create_memsec_from_seg(okl4_env_segment_t *seg)
{
    okl4_static_memsec_t *memsec;
    okl4_static_memsec_attr_t attr;

    /* Convert into a static memsec. */
    okl4_static_memsec_attr_init(&attr);
    okl4_static_memsec_attr_setsegment(&attr, seg);
    memsec = malloc(OKL4_STATIC_MEMSEC_SIZE_ATTR(&attr));
    assert(memsec);
    okl4_static_memsec_init(memsec, &attr);
    return okl4_static_memsec_getmemsec(memsec);
}

static void
init_ctest_segments_from_env(void)
{
    unsigned long i;
    okl4_env_segments_t *segs;

    segs = OKL4_ENV_GET_SEGMENTS("SEGMENTS");
    assert(segs != NULL);
    assert(segs->num_segments+1 < NUM_SEGMENTS); /* +1 for NULL terminator */

    /*
     * Allocate a new memsection for ctest for each segment from environment.
     */
    for (i = 0; i < segs->num_segments; i++) {
        ctest_segments[i] = create_memsec_from_seg(&segs->segments[i]);
    }
    ctest_segments[i] = NULL;
}

void
init_root_pools(void)
{
    root_virtmem_pool = (okl4_virtmem_pool_t *)okl4_env_get("MAIN_VIRTMEM_POOL");
    assert(root_virtmem_pool != NULL);

    /* Find our root PD object. */
    root_pd = okl4_env_get("MAIN_PD");
    assert(root_pd != NULL);

    /* Setup the root thread. */
    root_thread = okl4_env_get("MAIN_KSPACE_THREAD_0");
    assert(root_thread != NULL);

    init_ctest_segments_from_env();
}

