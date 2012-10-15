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

#include <okl4/kthread.h>
#include <okl4/errno.h>

#include <stdlib.h>

#define NUM_THREADS  (OKL4_DEFAULT_PAGESIZE / UTCB_SIZE)
#define ALLOC_BASE 2
#define ALLOC_SIZE 50

#define UTCB_ADDR 0x60000000ULL

/* The root kclist, needed to get capids for making the threads. */
extern okl4_kclist_t *root_kclist;

/*
 * Static initialization and creation of the kthread
 */

OKL4_BITMAP_ALLOCATOR_DECLARE_OFRANGE(test0200_kclistidpool, ALLOC_BASE,
        ALLOC_SIZE);
OKL4_BITMAP_ALLOCATOR_DECLARE_OFRANGE(test0200_kspaceidpool, ALLOC_BASE,
        ALLOC_SIZE);

START_TEST(KTHREAD0100)
{
    int ret;
    struct okl4_kclist *kclist;
    struct okl4_utcb_area *utcb_area;
    struct okl4_utcb_item *utcb_item;
    struct okl4_kcap_item *kcap_item;
    struct okl4_kspace *kspace;
    struct okl4_kthread *kthread;

    okl4_kclist_attr_t kclist_attr;
    okl4_utcb_area_attr_t utcb_area_attr;
    okl4_kspace_attr_t kspace_attr;
    okl4_kthread_attr_t kthread_attr;

    okl4_kclistid_t kclist_id;
    okl4_kspaceid_t kspace_id;
    okl4_allocator_attr_t kclistid_pool_attr;
    okl4_allocator_attr_t kspaceid_pool_attr;

    /* Allocate any kclist identifier */
    okl4_allocator_attr_init(&kclistid_pool_attr);
    okl4_allocator_attr_setrange(&kclistid_pool_attr, ALLOC_BASE, ALLOC_SIZE);
    okl4_bitmap_allocator_init(test0200_kclistidpool, &kclistid_pool_attr);
    ret = okl4_kclistid_allocany(test0200_kclistidpool, &kclist_id);
    fail_unless(ret == OKL4_OK, "Failed to allocate any kclist id.");

    /* Set the kclist attributes */
    okl4_kclist_attr_init(&kclist_attr);
    okl4_kclist_attr_setrange(&kclist_attr, ALLOC_BASE, ALLOC_SIZE);
    okl4_kclist_attr_setid(&kclist_attr, kclist_id);
    /* Allocate the kclist structure */
    kclist = malloc(OKL4_KCLIST_SIZE_ATTR(&kclist_attr));

    /* Create the kclist */
    ret = okl4_kclist_create(kclist, &kclist_attr);
    fail_unless(ret == OKL4_OK,"Failed to create a kclist");

    /* Set the utcb area attributes */
    okl4_utcb_area_attr_init(&utcb_area_attr);
    okl4_utcb_area_attr_setrange(&utcb_area_attr, UTCB_ADDR,
            (UTCB_SIZE * NUM_THREADS));

    /* Allocate the utcb area structure */
    utcb_area = malloc(OKL4_UTCB_AREA_SIZE_ATTR(&utcb_area_attr));
    assert(utcb_area != NULL);

    /* Initialize the utcb area */
    okl4_utcb_area_init(utcb_area, &utcb_area_attr);

    /* Allocate any kspace identifier for the kspace */
    okl4_allocator_attr_init(&kspaceid_pool_attr);
    okl4_allocator_attr_setrange(&kspaceid_pool_attr,
            ALLOC_BASE, ALLOC_BASE + ALLOC_SIZE);
    okl4_bitmap_allocator_init(test0200_kspaceidpool, &kspaceid_pool_attr);
    ret = okl4_kspaceid_allocany(test0200_kspaceidpool, &kspace_id);
    fail_unless(ret == OKL4_OK, "Failed to allocate any space id.");

    /* Set the kspace attributes */
    okl4_kspace_attr_init(&kspace_attr);
    okl4_kspace_attr_setkclist(&kspace_attr, kclist);
    okl4_kspace_attr_setid(&kspace_attr, kspace_id);
    okl4_kspace_attr_setutcbarea(&kspace_attr, utcb_area);

    /* Allocate the kspace  */
    kspace = malloc(OKL4_KSPACE_SIZE_ATTR(&kspace_attr));
    assert(kspace);

    /* Create the kspace */
    ret = okl4_kspace_create(kspace, &kspace_attr);
    fail_unless(ret == OKL4_OK, "Failed to create a kspace");

    /* Set the kspace for the kthread */
    okl4_kthread_attr_init(&kthread_attr);
    okl4_kthread_attr_setspace(&kthread_attr, kspace);
    okl4_kthread_attr_setspip(&kthread_attr, 0xdeadbeefU, 0xdeadbeefU);

    /* Allocate the utcb structure */
    utcb_item = malloc(sizeof(struct okl4_utcb_item));
    assert(utcb_item);

    /* Get any utcb for the thread from the utcb area of the kspace */
    ret = okl4_utcb_allocany(kspace->utcb_area, utcb_item);
    fail_unless(ret == OKL4_OK, "Failed to allocate a utcb");

    okl4_kthread_attr_setutcbitem(&kthread_attr, utcb_item);

    /* Allocate a cap for the kthread from the root clist. Note:
     * the cap does not come from the clist associated with this
     * space.
     */
    kcap_item = malloc(sizeof(struct okl4_kcap_item));
    assert(kcap_item);
    ret = okl4_kclist_kcap_allocany(root_kclist, kcap_item);
    fail_unless(ret == OKL4_OK,"Failed to allocate a thread id");
    okl4_kthread_attr_setcapitem(&kthread_attr, kcap_item);

    /* Allocate the kthread */
    kthread = malloc(OKL4_KTHREAD_SIZE_ATTR(&kthread_attr));
    assert(kthread);

    /* Create the kthread */
    ret = okl4_kthread_create(kthread, &kthread_attr);
    fail_unless(ret == OKL4_OK,"The Kthread creation failed");

    /* Delete the thread and free resources. */
    okl4_kthread_delete(kthread);

    /* Delete the kspace */
    okl4_kspace_delete(kspace);

    /* Delete the clist */
    okl4_kclist_delete(kclist);
    /* Destroy the UTCB */
    okl4_utcb_area_destroy(utcb_area);
    /* Free the kernel IDs */
    okl4_kclist_kcap_free(root_kclist, kcap_item);
    okl4_kspaceid_free(test0200_kspaceidpool, kspace_id);
    okl4_kclistid_free(test0200_kclistidpool, kclist_id);
}
END_TEST

OKL4_KSPACE_DECLARE(kspace);

/* Statically declare the UTCB area */
OKL4_UTCB_AREA_DECLARE_OFSIZE(utcb_area, UTCB_ADDR, UTCB_SIZE * NUM_THREADS);

/* Statically declare the kthread */
OKL4_KTHREAD_DECLARE(kthread);


/* Static creation of threads */
START_TEST(KTHREAD0200)
{
    int ret;
    struct okl4_kclist *kclist;
    struct okl4_utcb_item *utcb_item;
    struct okl4_kcap_item *kcap_item;

    okl4_kclist_attr_t kclist_attr;
    okl4_utcb_area_attr_t utcb_area_attr;
    okl4_kspace_attr_t kspace_attr;
    okl4_kthread_attr_t kthread_attr;
    okl4_allocator_attr_t kclistid_pool_attr;
    okl4_allocator_attr_t kspaceid_pool_attr;

    okl4_kclistid_t kclist_id;
    okl4_kspaceid_t kspace_id;


    /* Allocate a specific kclist identifier */
    kclist_id.clist_no = 10;
    okl4_allocator_attr_init(&kclistid_pool_attr);
    okl4_allocator_attr_setrange(&kclistid_pool_attr, ALLOC_BASE, ALLOC_SIZE);
    okl4_bitmap_allocator_init(test0200_kclistidpool, &kclistid_pool_attr);
    ret = okl4_kclistid_allocunit(test0200_kclistidpool, &kclist_id);
    fail_unless(ret == OKL4_OK, "Failed to allocate any kclist id.");

    /* Set the kclist attributes */
    okl4_kclist_attr_init(&kclist_attr);
    okl4_kclist_attr_setrange(&kclist_attr, ALLOC_BASE, ALLOC_SIZE);
    okl4_kclist_attr_setid(&kclist_attr, kclist_id);

    /* Allocate the kclist structure */
    kclist = malloc(OKL4_KCLIST_SIZE_ATTR(&kclist_attr));
    assert(kclist);

        /* Create the kclist */
    ret = okl4_kclist_create(kclist, &kclist_attr);
    fail_unless(ret == OKL4_OK,"Failed to create a kclist");

    /* Set the UTCB area attributes */
    okl4_utcb_area_attr_init(&utcb_area_attr);
    okl4_utcb_area_attr_setrange(&utcb_area_attr, UTCB_ADDR,
            UTCB_SIZE * NUM_THREADS);

        /* Initialize the UTCB */
    okl4_utcb_area_init(utcb_area, &utcb_area_attr);

    /* Allocate a specific kspace identifier */
    kspace_id.space_no = 3;
    okl4_allocator_attr_init(&kspaceid_pool_attr);
    okl4_allocator_attr_setrange(&kspaceid_pool_attr, ALLOC_BASE, ALLOC_SIZE);
    okl4_bitmap_allocator_init(test0200_kspaceidpool, &kspaceid_pool_attr);
    ret = okl4_kspaceid_allocunit(test0200_kspaceidpool, &kspace_id);
    fail_unless(ret == OKL4_OK, "Failed to allocate any space id.");

    /* Set the kspace attributes */
    okl4_kspace_attr_init(&kspace_attr);
    okl4_kspace_attr_setkclist(&kspace_attr, kclist);
    okl4_kspace_attr_setid(&kspace_attr, kspace_id);
    okl4_kspace_attr_setutcbarea(&kspace_attr, utcb_area);

    /* Statically declare the kspace */
    ret = okl4_kspace_create(kspace, &kspace_attr);
    fail_unless(ret == OKL4_OK, "Failed to create a kspace");

    /* Set the kspace for the kthread */
    okl4_kthread_attr_init(&kthread_attr);
    okl4_kthread_attr_setspace(&kthread_attr, kspace);
    okl4_kthread_attr_setspip(&kthread_attr, 0xdeadbeefU, 0xdeadbeefU);

    /* Allocate the UTCB for the thread */
    utcb_item = malloc(sizeof(struct okl4_utcb_item));
    assert(utcb_item != NULL);

    /* Get any utcb for the thread from the utcb area of the kspace */
    ret = okl4_utcb_allocunit(kspace->utcb_area, utcb_item, 4);
    fail_unless(ret == OKL4_OK, "Failed to allocate the specific utcb");

    /* Set the utcb for the thread */
    okl4_kthread_attr_setutcbitem(&kthread_attr, utcb_item);

    /* Allocate a specific cap slot for the thread from the root
     * clist. */
    kcap_item = malloc(sizeof(struct okl4_kcap_item));
    ret = okl4_kclist_kcap_allocunit(root_kclist, kcap_item, 12);
    fail_unless(ret == OKL4_OK,"Failed to allocate a cap slot for the thread");
    /* Set the cap item */
    okl4_kthread_attr_setcapitem(&kthread_attr, kcap_item);

    /* Create the thread */
    ret = okl4_kthread_create(kthread, &kthread_attr);
    fail_unless(ret == OKL4_OK,"The Kthread creation failed");

    /* Re-create the same thread twice */
    ret = okl4_kthread_create(kthread, &kthread_attr);
    fail_unless(ret == OKL4_INVALID_ARGUMENT, "The Kthread creation failed");

    /* Delete the kthread */
    okl4_kthread_delete(kthread);
    /* Delete the kspace */
    okl4_kspace_delete(kspace);

    /* Delete the clist */
    okl4_kclist_delete(kclist);
    /* Destroy the UTCB */
    okl4_utcb_area_destroy(utcb_area);
    /* Free the kernel IDs */
    okl4_kclist_kcap_free(root_kclist, kcap_item);
    okl4_kspaceid_free(test0200_kspaceidpool, kspace_id);
    okl4_kclistid_free(test0200_kclistidpool, kclist_id);

}
END_TEST

TCase *
make_kthread_tcase(void)
{
    TCase * tc;
    tc = tcase_create("KThread");
    tcase_add_test(tc, KTHREAD0100);
    tcase_add_test(tc, KTHREAD0200);

    return tc;
}
