/* @LICENCE("Public", "2007")@ */

#include <stdio.h>
#include <l4/ipc.h>
#include <l4/schedule.h>

#include <okl4/env.h>
#include <okl4/virtmem_pool.h>
#include <okl4/physmem_segpool.h>
#include <okl4/kspace.h>
#include <okl4/kthread.h>
#include <okl4/kclist.h>
#include <okl4/memsec.h>
#include <okl4/static_memsec.h>
#include <okl4/zone.h>
#include <okl4/pd.h>

#define MAX(a, b) (((b)>(a))?(b):(a))
#define MIN(a, b) (((b)<(a))?(b):(a))

/* Stacks for the child threads to use */
#define STACK_SIZE ((0x1000 / sizeof(okl4_word_t)))
static okl4_word_t thread_stacks[2][STACK_SIZE] ALIGNED(32);

/* Maximum number of threads per PD. */
#define MAX_THREADS    32

/* Maximum number of PDs each zone can be attached to. */
#define MAX_PDS    10

/* Size and base address of shared memory buffer. */
#define SHARED_BUF_SIZE  (2 * OKL4_DEFAULT_PAGESIZE)
okl4_word_t shared_mem_base;

/* Default priority of new threads. */
#define DEFAULT_PRIORITY    100

/* Data needed by thread 0 */
okl4_kcap_t thread1_ipc_cap;
#define THREAD0_MAGIC 0x98765432

/* Data needed by thread 1 */
okl4_kcap_t thread0_ipc_cap;
#define THREAD1_MAGIC   0x12345678

/*
 * The cells's root data structures, provided by Elfweaver and looked up
 * in the environment
 */
okl4_virtmem_pool_t *root_virtmem_pool;
okl4_physmem_segpool_t *root_physseg_pool;
okl4_kspaceid_pool_t *root_kspace_pool;
okl4_kclistid_pool_t *root_kclist_pool;
okl4_kspace_t *root_kspace;
okl4_kclist_t *root_kclist;
okl4_kthread_t *root_kthread;

/*
 * Look up the weaved root objects in the environment.
 */
static void
get_root_weaved_objects(void)
{
    root_virtmem_pool = okl4_env_get("MAIN_VIRTMEM_POOL");
    assert(root_virtmem_pool);

    root_physseg_pool = okl4_env_get("MAIN_PHYSMEM_SEGPOOL");
    assert(root_physseg_pool);

    root_kspace_pool = okl4_env_get("MAIN_SPACE_ID_POOL");
    assert(root_kspace_pool);

    root_kclist_pool = okl4_env_get("MAIN_CLIST_ID_POOL");
    assert(root_kclist_pool);

    root_kspace = okl4_env_get("MAIN_KSPACE");
    assert(root_kspace);

    root_kclist = okl4_env_get("MAIN_KCLIST");
    assert(root_kclist);

    root_kthread = okl4_env_get("MAIN_KSPACE_THREAD_0");
    assert(root_kthread);
}

/*
 * Attach a zone to a PD using default permissions
 */
static int
attach_zone_to_pd(okl4_pd_t *pd, okl4_zone_t *zone)
{
    okl4_pd_zone_attach_attr_t attr;

    okl4_pd_zone_attach_attr_init(&attr);

    return okl4_pd_zone_attach(pd, zone, &attr);
}

/*
 * Create the root PD object.
 */
static okl4_pd_t *
create_root_pd(void)
{
    okl4_pd_attr_t attr;
    okl4_pd_t *rootpd;
    int error;

    /* Set up an attribute object so we can set up the root PD */
    okl4_pd_attr_init(&attr);
    okl4_pd_attr_setconstructrootpd(&attr, 1);

    /* Allocate some memory from our heap to store the root PD object */
    rootpd = malloc(OKL4_PD_SIZE_ATTR(&attr));
    assert(rootpd);

    /* Now we actually contsruct the root PD */
    error = okl4_pd_create(rootpd, &attr);
    assert(!error);

    return rootpd;
}

/*
 * Create a new PD from the root weaved objects.
 */
static okl4_pd_t *
create_pd_from_root_pools(void)
{
    okl4_pd_attr_t attr;
    okl4_pd_t *pd;
    int error;

    /* Initialise the PD attribute object. */
    okl4_pd_attr_init(&attr);

    /* Set up the attribute object to use our root objects */
    okl4_pd_attr_setkspaceidpool(&attr, root_kspace_pool);
    okl4_pd_attr_setkclistidpool(&attr, root_kclist_pool);
    okl4_pd_attr_setutcbmempool(&attr, root_virtmem_pool);
    okl4_pd_attr_setrootkclist(&attr, root_kclist);
    okl4_pd_attr_setmaxthreads(&attr, MAX_THREADS);
    okl4_pd_attr_setprivileged(&attr, 0);

    pd = malloc(OKL4_PD_SIZE_ATTR(&attr));
    assert(pd);

    error = okl4_pd_create(pd, &attr);
    assert(!error);

    return pd;
}

/*
 * Create a static memsection covering an ELF segment of given name.
 */
static okl4_memsec_t *
create_memsec_from_elf_segment(char *name)
{
    okl4_env_segment_t *seg;
    okl4_static_memsec_attr_t attr;
    okl4_static_memsec_t *ms;

    assert(name != NULL);

    /* Look up the text segment */
    seg = okl4_env_get_segment(name);
    assert(seg);

    /* Create a memsection object representing the segment, so that it can
     * be attached to the children */

    /* Set up the static memsection attribute object. */
    okl4_static_memsec_attr_init(&attr);
    okl4_static_memsec_attr_setsegment(&attr, seg);

    /* Create the memsection */
    ms = malloc(OKL4_STATIC_MEMSEC_SIZE_ATTR(&attr));
    assert(ms);
    okl4_static_memsec_init(ms, &attr);

    return okl4_static_memsec_getmemsec(ms);
}

/*
 * Given memsections covering the text and data segments, create
 * a zone containing those memsections.
 */
static okl4_zone_t *
create_elf_segment_zone(okl4_memsec_t *text, okl4_memsec_t *data)
{
    okl4_virtmem_item_t virt;
    okl4_zone_attr_t zone_attr;
    okl4_word_t min_vaddr, max_vaddr, size;
    okl4_range_item_t text_range, data_range;
    okl4_zone_t *elf_zone;
    int error;

    /* Calculate the virtual address range the zone must cover. */
    text_range = okl4_memsec_getrange(text);
    data_range = okl4_memsec_getrange(data);

    min_vaddr = MIN(okl4_range_item_getbase(&text_range),
                    okl4_range_item_getbase(&data_range));
    max_vaddr = MAX(okl4_range_item_getbase(&text_range) +
                    okl4_range_item_getsize(&text_range),
                    okl4_range_item_getbase(&data_range) +
                    okl4_range_item_getsize(&data_range));
    size = max_vaddr - min_vaddr;

    /* Set up the zone attribute object. */
    okl4_range_item_setrange(&virt, min_vaddr, size);
    okl4_zone_attr_init(&zone_attr);
    okl4_zone_attr_setrange(&zone_attr, virt);
    okl4_zone_attr_setmaxpds(&zone_attr, MAX_PDS);

    /* Create the zone object. */
    elf_zone = malloc(OKL4_ZONE_SIZE_ATTR(&zone_attr));
    assert(elf_zone);

    error = okl4_zone_create(elf_zone, &zone_attr);
    assert(!error);

    /* Now attach the memsections representing the ELF segments to the zone. */
    error = okl4_zone_memsec_attach(elf_zone, text);
    assert(!error);
    error = okl4_zone_memsec_attach(elf_zone, data);
    assert(!error);

    return elf_zone;
}

/*
 * Create a static memsection from the root memory pools.
 */
static okl4_memsec_t *
create_memsec_from_pools(okl4_virtmem_item_t *ms_virt,
        okl4_physmem_item_t *ms_phys, okl4_word_t size)
{
    okl4_static_memsec_attr_t ms_attr;
    okl4_static_memsec_t *ms;
    int error;

    okl4_range_item_setsize(ms_virt, size);
    error = okl4_virtmem_pool_alloc(root_virtmem_pool, ms_virt);
    assert(!error);
    printf("Allocated virtual memory from the root pool\n");

    okl4_physmem_item_setsize(ms_phys, size);
    error = okl4_physmem_segpool_alloc(root_physseg_pool, ms_phys);
    assert(!error);
    printf("Allocated physical memory from the root pool\n");

    okl4_static_memsec_attr_init(&ms_attr);
    okl4_static_memsec_attr_setrange(&ms_attr, *ms_virt);
    okl4_static_memsec_attr_settarget(&ms_attr, *ms_phys);
    okl4_static_memsec_attr_setperms(&ms_attr, L4_Readable | L4_Writable);
    okl4_static_memsec_attr_setattributes(&ms_attr, L4_DefaultMemory);

    ms = malloc(OKL4_STATIC_MEMSEC_SIZE_ATTR(&ms_attr));
    okl4_static_memsec_init(ms, &ms_attr);

    return okl4_static_memsec_getmemsec(ms);
}

/*
 * Create a new zone covering a specified virtual address range.
 */
static okl4_zone_t *
create_zone(okl4_word_t base, okl4_word_t size)
{
    okl4_virtmem_item_t virt;
    okl4_zone_attr_t zone_attr;
    okl4_zone_t *zone;
    int error;

    /* Set up the zone attribute object. */
    okl4_range_item_setrange(&virt, base, size);
    okl4_zone_attr_init(&zone_attr);
    okl4_zone_attr_setrange(&zone_attr, virt);
    okl4_zone_attr_setmaxpds(&zone_attr, MAX_PDS);

    /* Create the zone. */
    zone = malloc(OKL4_ZONE_SIZE_ATTR(&zone_attr));
    assert(zone);

    error = okl4_zone_create(zone, &zone_attr);
    assert(!error);

    return zone;
}

/*
 * Create a thread in the specified protection domain, using a default
 * priority and pager.
 */
static okl4_kthread_t *
create_thread_in_pd(okl4_pd_t *pd, okl4_word_t sp, okl4_word_t ip)
{
    okl4_pd_thread_create_attr_t thread_attr;
    okl4_kthread_t *kthread;
    int error;

    /* Set up the thread create attrbute object. */
    okl4_pd_thread_create_attr_init(&thread_attr);
    okl4_pd_thread_create_attr_setspip(&thread_attr, sp, ip);
    okl4_pd_thread_create_attr_setpriority(&thread_attr, DEFAULT_PRIORITY);
    /* Set the root thread to be the pager. */
    okl4_pd_thread_create_attr_setpager(&thread_attr,
            okl4_kthread_getkcap(root_kthread));

    /* Create the thread */
    error = okl4_pd_thread_create(pd, &thread_attr, &kthread);
    assert(!error);

    return kthread;
}

/*
 * Destroy a zone.
 */
static void
destroy_zone(okl4_zone_t *zone)
{
    okl4_zone_delete(zone);
    free(zone);
}

/*
 * Destroy a memsection.
 */
static void
destroy_memsec(okl4_memsec_t *ms)
{
    okl4_memsec_destroy(ms);
    free(ms);
}

static void
child_thread0_routine(void)
{
    L4_MsgTag_t tag;
    L4_Msg_t msg;
    okl4_word_t num;

    printf("[thread0] Hello from child thread 0\n");
    printf("[thread0] Waiting for IPC from thread 1 (in pd 1)\n");

    tag = L4_Receive(thread1_ipc_cap);
    L4_MsgStore(tag, &msg);
    assert(L4_IpcSucceeded(tag));
    printf("[thread0] Got IPC from thread 1\n");

    printf("[thread0] Reading from shared buffer...");
    num = *(okl4_word_t *)shared_mem_base;
    printf(" got 0x%x\n", (unsigned int)num);
    assert(num == THREAD1_MAGIC);

    printf("[thread0] Writing to shared buffer\n");
    *(okl4_word_t *)shared_mem_base = (okl4_word_t)THREAD0_MAGIC;

    L4_MsgClear(&msg);
    L4_MsgLoad(&msg);
    tag = L4_Reply(thread1_ipc_cap);
    assert(L4_IpcSucceeded(tag));
    printf("[thread0] Replied to thread 1\n");

    L4_WaitForever();
}

static void
child_thread1_routine(void)
{
    L4_MsgTag_t tag;
    L4_Msg_t msg;
    okl4_word_t num;

    printf("[thread1] Hello from child thread 1\n");

    printf("[thread1] Writing to shared buffer....\n");
    *(okl4_word_t *)shared_mem_base = (okl4_word_t)THREAD1_MAGIC;

    printf("[thread1] Sending IPC to thread 0 (in pd 0)\n");

    L4_MsgClear(&msg);
    L4_MsgLoad(&msg);
    tag = L4_Call(thread0_ipc_cap);
    assert(L4_IpcSucceeded(tag));

    L4_MsgStore(tag, &msg);
    assert(L4_IpcSucceeded(tag));
    printf("[thread1] Got reply from thread 0\n");

    printf("[thread1] Reading from shared buffer...");
    num = *(okl4_word_t *)shared_mem_base;
    printf(" got 0x%x\n", (unsigned int)num);
    assert(num == THREAD0_MAGIC);

    L4_WaitForever();
}

int
main(int argc, char **argv)
{
    int error, i;
    okl4_word_t base, size;

    okl4_pd_t *rootpd;
    okl4_pd_t *pds[2];
    okl4_kthread_t *threads[2];

    okl4_memsec_t *text, *data;
    okl4_zone_t *elf_zone;

    okl4_memsec_t *ms;
    okl4_zone_t *zone;
    okl4_range_item_t range;
    okl4_virtmem_item_t ms_virt;
    okl4_physmem_item_t ms_phys;

    /* Announce that we are running :) */
    printf("Hello, World\n");

    rootpd = create_root_pd();
    printf("Created root PD object\n");

    /* Look up the weaved root data structures in the environment */
    get_root_weaved_objects();

    /* Create two child PDs */
    for (i=0; i<2; i++) {
        pds[i] = create_pd_from_root_pools();
        printf("Child pd %d created successfully\n", i);
    }

    /*
     * We now have 2 empty PDs. We want to run some code in them, so we will
     * have to look up our ELF segments and map the code and data into the new
     * PDs.
     */

    text = create_memsec_from_elf_segment("MAIN_RX");
    assert(text);
    printf("Created a memsection object for the text segment\n");

    /* Now do the same for the data segment. */
    data = create_memsec_from_elf_segment("MAIN_RW");
    assert(data);
    printf("Created a memsection object for the data segment\n");

    /*
     * Now we have the memsections, we want to map them into the child PDs.
     * However, since we want to share them between more than one PD, we
     * must first attach them to a zone, rather than directly to each PD.
     */

    /* Create a zone for the ELF segments */
    elf_zone = create_elf_segment_zone(text, data);
    printf("Created a zone for the ELF segments, attached both memsections\n");

    /* Now attach the zone to both child PDs, using default permissions. */
    for (i=0; i<2; i++) {
        error = attach_zone_to_pd(pds[i], elf_zone);
        assert(!error);
    }
    printf("Attached the zone containing the ELF segments to both PDs\n");

    /* Create a shared memory buffer to be used by both PDs. Allocate memory
     * from the root memory pools. */
    ms = create_memsec_from_pools(&ms_virt, &ms_phys, SHARED_BUF_SIZE);
    assert(ms);
    printf("Created new static memsection\n");

    /* Create a new zone for the memsection. */
    range = okl4_memsec_getrange(ms);
    base = okl4_range_item_getbase(&range);
    shared_mem_base = base;
    size = okl4_range_item_getsize(&range);
    zone = create_zone(base, size);
    printf("Created a zone for the shared buffer\n");

    error = okl4_zone_memsec_attach(zone, ms);
    assert(!error);
    printf("Attached memsection to the zone\n");

    for (i=0; i<2; i++) {
        error = attach_zone_to_pd(pds[i], zone);
        assert(!error);
    }
    printf("Attached zone to both PDs\n");

    /* Create and start a thread in each PD. */
    threads[0] = create_thread_in_pd(pds[0],
                        (okl4_word_t)&thread_stacks[0][STACK_SIZE],
                        (okl4_word_t)child_thread0_routine);
    threads[1] = create_thread_in_pd(pds[1],
                        (okl4_word_t)&thread_stacks[1][STACK_SIZE],
                        (okl4_word_t)child_thread1_routine);
    printf("Created a thread in each PD\n");

    /* Give each new thread an IPC cap to the other one. */
    error = okl4_pd_createcap(pds[0], threads[1], &thread1_ipc_cap);
    assert(!error);
    printf("Gave pd 0 an IPC cap to thread 1\n");

    error = okl4_pd_createcap(pds[1], threads[0], &thread0_ipc_cap);
    assert(!error);
    printf("Gave pd 1 an IPC cap to thread 0\n");

    /* Lower our priority so the child threads can run until completion. */
    L4_Set_Priority(okl4_kthread_getkcap(root_kthread), DEFAULT_PRIORITY - 1);

    /* Start the threads */
    okl4_pd_thread_start(pds[0], threads[0]);
    okl4_pd_thread_start(pds[1], threads[1]);

    /* Delete IPC caps. */
    okl4_pd_deletecap(pds[0], thread1_ipc_cap);
    okl4_pd_deletecap(pds[1], thread0_ipc_cap);

    /* Delete threads from the PDs, then delete the PDs. */
    for (i=0; i<2; i++) {
        okl4_pd_thread_delete(pds[i], threads[i]);
        okl4_pd_zone_detach(pds[i], zone);
        okl4_pd_zone_detach(pds[i], elf_zone);
        free(pds[i]);
    }
    printf("Deleted threads and PDs\n");

    /*
     * Detach shared buffer, destroy the zone and memsec, return memory
     * to pools
     */
    okl4_zone_memsec_detach(zone, ms);
    destroy_zone(zone);
    destroy_memsec(ms);
    okl4_virtmem_pool_free(root_virtmem_pool, &ms_virt);
    okl4_physmem_segpool_free(root_physseg_pool, &ms_phys);
    printf("Destroyed the shared buffer\n");

    /*
     * Detach the ELF zone, then destroy the zone and memsections.
     */
    okl4_zone_memsec_detach(elf_zone, text);
    okl4_zone_memsec_detach(elf_zone, data);
    destroy_zone(elf_zone);
    destroy_memsec(text);
    destroy_memsec(data);
    printf("Destroyed the ELF zone and memsections\n");

    printf("Hello example finished successfully!\n");
    L4_KDB_Enter("Done");
}
