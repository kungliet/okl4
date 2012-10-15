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

#include <okl4/kspace.h>

#include <stdlib.h>

#define NUM_THREADS (OKL4_DEFAULT_PAGESIZE / UTCB_SIZE)
#define ALLOC_BASE 3
#define ALLOC_SIZE 20
#define UTCB_ADDR_1 0x60000000ULL
#define UTCB_ADDR_2 0x70000000ULL


OKL4_BITMAP_ALLOCATOR_DECLARE_OFRANGE(kstest0200_kclistidpool, ALLOC_BASE,
        ALLOC_SIZE);
OKL4_BITMAP_ALLOCATOR_DECLARE_OFRANGE(kstest0200_kspaceidpool, ALLOC_BASE,
        ALLOC_SIZE);
OKL4_UTCB_AREA_DECLARE_OFSIZE(utcb_area_2, UTCB_ADDR_2, UTCB_SIZE * NUM_THREADS);

/*
 * This test creates two kspaces associated with the same kclist and
 * deletes them one by one. It also tries to create the same kspace twice
 * and checks for failures.
 */
START_TEST(KSPACE0100)
{

    int ret;

    struct okl4_kclist *kclist;
    struct okl4_utcb_area *utcb_area_1;
    struct okl4_kspace *kspace_1;
    struct okl4_kspace *kspace_2;

    okl4_kclist_attr_t kclist_attr;
    okl4_utcb_area_attr_t utcb_area_attr_1;
    okl4_utcb_area_attr_t utcb_area_attr_2;
    okl4_kspace_attr_t kspace_attr_1;
    okl4_kspace_attr_t kspace_attr_2;

    okl4_kclistid_t kclist_id;
    okl4_kspaceid_t kspace_id_1;
    okl4_kspaceid_t kspace_id_2;


    okl4_allocator_attr_t kclistid_pool_attr;
    okl4_allocator_attr_t kspaceid_pool_attr;

    /* Allocate any kclist identifier */
    okl4_allocator_attr_init(&kclistid_pool_attr);
    okl4_allocator_attr_setrange(&kclistid_pool_attr, ALLOC_BASE, ALLOC_SIZE);
    okl4_bitmap_allocator_init(kstest0200_kclistidpool, &kclistid_pool_attr);
    ret = okl4_kclistid_allocany(kstest0200_kclistidpool, &kclist_id);
    fail_unless(ret == OKL4_OK, "Failed to allocate any kclist id.");

    /* Set the kclist attributes */
    okl4_kclist_attr_init(&kclist_attr);
    okl4_kclist_attr_setrange(&kclist_attr, ALLOC_BASE, ALLOC_SIZE);
    okl4_kclist_attr_setid(&kclist_attr, kclist_id);

    /* Allocate the kclist structure */
    kclist = malloc(OKL4_KCLIST_SIZE_ATTR(&kclist_attr));
    assert(kclist != NULL);

    /* Create the kclist */
    ret = okl4_kclist_create(kclist, &kclist_attr);
    fail_unless(ret == OKL4_OK,"Failed to create a kclist");

    /* Set attributes for the first UTCB area */
    okl4_utcb_area_attr_init(&utcb_area_attr_1);
    okl4_utcb_area_attr_setrange(&utcb_area_attr_1, UTCB_ADDR_1,
            (UTCB_SIZE * NUM_THREADS));
    /* Allocate the utcb area structure */
    utcb_area_1 = malloc(OKL4_UTCB_AREA_SIZE_ATTR(&utcb_area_attr_1));
    assert(utcb_area_1 != NULL);

    /* Set attributes for the second UTCB area */
    okl4_utcb_area_attr_init(&utcb_area_attr_2);
    okl4_utcb_area_attr_setrange(&utcb_area_attr_2, UTCB_ADDR_2,
            (UTCB_SIZE * NUM_THREADS));

    /* Initialize the utcb areas */
    okl4_utcb_area_init(utcb_area_1, &utcb_area_attr_1);
    okl4_utcb_area_init(utcb_area_2, &utcb_area_attr_2);

    /* Allocate any kspace identifier for the kspace */
    kspace_id_1.space_no = 12;
    okl4_allocator_attr_init(&kspaceid_pool_attr);
    okl4_allocator_attr_setrange(&kspaceid_pool_attr, ALLOC_BASE, ALLOC_SIZE);
    okl4_bitmap_allocator_init(kstest0200_kspaceidpool, &kspaceid_pool_attr);
    ret = okl4_kspaceid_allocunit(kstest0200_kspaceidpool, &kspace_id_1);
    fail_unless(ret == OKL4_OK, "Failed to allocate any space id.");

    kspace_id_2.space_no = 13;
    ret = okl4_kspaceid_allocunit(kstest0200_kspaceidpool, &kspace_id_2);
    fail_unless(ret == OKL4_OK, "Failed to allocate any space id.");

    /* Set the first kspace's attributes */
    okl4_kspace_attr_init(&kspace_attr_1);
    okl4_kspace_attr_setkclist(&kspace_attr_1, kclist);
    okl4_kspace_attr_setid(&kspace_attr_1, kspace_id_1);
    okl4_kspace_attr_setutcbarea(&kspace_attr_1, utcb_area_1);

    /* Set the second kspace's attributes */
    okl4_kspace_attr_init(&kspace_attr_2);
    okl4_kspace_attr_setkclist(&kspace_attr_2, kclist);
    okl4_kspace_attr_setid(&kspace_attr_2, kspace_id_2);
    okl4_kspace_attr_setutcbarea(&kspace_attr_2, utcb_area_2);

    /* Allocate the kspace  */
    kspace_1 = malloc(OKL4_KSPACE_SIZE_ATTR(&kspace_attr_1));
    assert(kspace_1 != NULL);

    kspace_2 = malloc(OKL4_KSPACE_SIZE_ATTR(&kspace_attr_1));
    assert(kspace_2 != NULL);

    /* Create the kspace */
    ret = okl4_kspace_create(kspace_1, &kspace_attr_1);
    fail_unless(ret == OKL4_OK, "Failed to create a kspace");

    /* Try to create the same space again.
     * SpaceControl should fail in this case
     */
    ret = okl4_kspace_create(kspace_1, &kspace_attr_1);
    fail_unless(ret == OKL4_INVALID_ARGUMENT, "Failed to create a kspace");

    /* Create the second kspace associated with the same kclist */
    ret = okl4_kspace_create(kspace_2, &kspace_attr_2);
    fail_unless(ret == OKL4_OK, "Failed to create a kspace");

    /* Delete kspace_1 */
    okl4_kspace_delete(kspace_1);

    /* Delete the utcb area */
    okl4_utcb_area_destroy(utcb_area_1);

    /* Delete kspace_2 */
    okl4_kspace_delete(kspace_2);

    /* Delete the utcb area */
    okl4_utcb_area_destroy(utcb_area_2);

    /* Delete the kclist */
    okl4_kclist_delete(kclist);

    /* free the kernel IDs */
    okl4_kspaceid_free(kstest0200_kspaceidpool, kspace_id_1);
    okl4_kspaceid_free(kstest0200_kspaceidpool, kspace_id_2);
    okl4_kclistid_free(kstest0200_kclistidpool, kclist_id);

}
END_TEST

START_TEST(KSPACE0200)
{
    struct okl4_kspace *rootkspace;
    struct okl4_kclist *rootkclist;
    struct okl4_kthread *rootkthread;
    struct okl4_utcb_area *rootutcbarea;

    rootkspace = okl4_env_get("MAIN_KSPACE");
    assert(rootkspace != NULL);

    printf("rootkspace ID: %d\n", (int)rootkspace->id.raw);
    printf("rootkspace is privileged?: %d\n", (int)rootkspace->privileged);

    rootkclist = rootkspace->kclist;
    assert(rootkclist != NULL);
    printf("rootkspace->kclist points to %p\n", rootkclist);
    printf("rootkclist ID: %d\n", (int)rootkclist->id.raw);
    printf("rootkclist num slots: %d\n", (int)rootkclist->num_cap_slots);

    rootkthread = rootkspace->kthread_list;
    assert(rootkthread != NULL);
    printf("rootkspace->kthread_list points to %p\n", rootkthread);
    printf("rootkthread cap: %d\n", (int)rootkthread->cap.raw);
    printf("rootkthread sp: 0x%x\n", (unsigned int)rootkthread->sp);
    printf("rootkthread ip: 0x%x\n", (unsigned int)rootkthread->ip);
    printf("rootkthread priority: %d\n", (int)rootkthread->priority);
    printf("rootkthread utcb: 0x%x\n", (unsigned int)rootkthread->utcb);
    printf("rootkthread->kspace points to %p\n", rootkthread->kspace);
    assert(rootkspace == rootkthread->kspace);

    rootutcbarea = rootkspace->utcb_area;
    assert(rootutcbarea != NULL);
    printf("rootkspace->utcb_area points to %p\n", rootutcbarea);
    printf("utcb area base: 0x%x\n",
            (unsigned int)rootutcbarea->utcb_memory.base);
    printf("utcb area size: 0x%x\n",
            (unsigned int)rootutcbarea->utcb_memory.size);
}
END_TEST

TCase *
make_kspace_tcase(void)
{
    TCase *tc;
    tc = tcase_create("kspace");
    tcase_add_test(tc, KSPACE0100);
    tcase_add_test(tc, KSPACE0200);

    return tc;
}

