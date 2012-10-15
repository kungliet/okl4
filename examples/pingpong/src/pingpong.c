/* @LICENCE("Public", "2007")@ */

#include <stdio.h>
#include <stdlib.h>

#include <okl4/init.h>
#include <okl4/kernel.h>
#include <okl4/kthread.h>
#include <okl4/message.h>

#if defined(OKL4_KERNEL_MICRO)
#include <okl4/env.h>
#include <okl4/kclist.h>
#include <okl4/utcb.h>
#endif


#define THREADS     2
#define STACK_SIZE  4096


void pingpong(void);

struct args
{
    okl4_word_t id;
    okl4_kcap_t parent;
    okl4_kcap_t *peers;
    okl4_word_t pings;
};


int
main(int argc, char **argv)
{
    int error;
    okl4_word_t i;
    okl4_word_t pings;
    void *stacks[THREADS];
    okl4_kthread_t threads[THREADS];
    okl4_kcap_t caps[THREADS];
    okl4_kthread_t *root_thread;

#if defined(OKL4_KERNEL_MICRO)
    okl4_kclist_t *root_kclist;
    okl4_kspace_t *root_kspace;
    struct okl4_utcb_item *utcb_item[THREADS];
    struct okl4_kcap_item *kcap_item[THREADS];
#endif

    okl4_init_thread();

#if defined(NANOKERNEL)
    {
        okl4_kthread_t _root_thread;
        _root_thread.cap = okn_syscall_thread_myself();
        root_thread = &_root_thread;
    }
#else
    root_kspace = okl4_env_get("MAIN_KSPACE");
    assert(root_kspace != NULL);
    root_thread = root_kspace->kthread_list;
    assert(root_thread != NULL);
    root_kclist = root_kspace->kclist;
    assert(root_kclist != NULL);
#endif

    if (argc == 1)
        pings = atoi(argv[0]);
    else
        pings = 10;
    printf("Ping-Pong will do %d pings\n", (int)pings);

    for (i = 0; i < THREADS; i++)
    {
        okl4_kthread_attr_t kthread_attr;

#if defined(OKL4_KERNEL_MICRO)
        /* Allocate the utcb structure */
        utcb_item[i] = malloc(sizeof(struct okl4_utcb_item));
        assert(utcb_item[i]);

        /* Get any utcb for the thread from the root kspace utcb area */
        // XXX: We need an accessor method for a kspace's utcb_area
        error = okl4_utcb_allocany(root_kspace->utcb_area, utcb_item[i]);
        assert(error == OKL4_OK);

        /* Allocate a cap for the kthread from the root clist */
        kcap_item[i] = malloc(sizeof(struct okl4_kcap_item));
        assert(kcap_item[i]);
        error = okl4_kclist_kcap_allocany(root_kclist, kcap_item[i]);
        assert(error == OKL4_OK);
#endif

        stacks[i] = malloc(STACK_SIZE);
        assert(stacks[i]);

        okl4_kthread_attr_init(&kthread_attr);
        okl4_kthread_attr_setspip(&kthread_attr,
                (okl4_word_t)stacks[i] + STACK_SIZE,
                (okl4_word_t)pingpong);
#if defined(OKL4_KERNEL_MICRO)
        okl4_kthread_attr_setspace(&kthread_attr, root_kspace);
        okl4_kthread_attr_setutcbitem(&kthread_attr, utcb_item[i]);
        okl4_kthread_attr_setcapitem(&kthread_attr, kcap_item[i]);
#endif
        error = okl4_kthread_create(&threads[i], &kthread_attr);
        assert(!error);
        caps[i] = okl4_kthread_getkcap(&threads[i]);
        okl4_kthread_start(&threads[i]);
    }

    // Message child threads to start the ping pong
    for (i = 0; i < THREADS; i++)
    {
        struct args args;
        args.id = i;
        args.parent = okl4_kthread_getkcap(root_thread);
        args.peers = caps;
        args.pings = pings;
        error = okl4_message_send(caps[i], &args, sizeof(args));
        assert(!error);
    }

    // Wait for the child threads to finish
    for (i = 0; i < THREADS; i++)
    {
        okl4_word_t pings_done;
        error = okl4_message_wait(&pings_done, sizeof(pings_done), NULL, NULL);
        assert(!error);
        assert(pings_done == pings);
    }

    /* Delete the thread and free resources. */
    for (i = 0; i < THREADS; i++)
    {
#if defined(OKL4_KERNEL_MICRO)
        okl4_kthread_delete(&threads[i]);
        okl4_kclist_kcap_free(root_kclist, kcap_item[i]);
        free(kcap_item[i]);
        // XXX: We need an accessor method for a kspace's utcb_area
        okl4_utcb_free(root_kspace->utcb_area, utcb_item[i]);
        free(utcb_item[i]);
#else
        okl4_kthread_join(&threads[i]);
#endif
        free(stacks[i]);
    }

    printf("Ping-Pong complete. Exiting...");
    /* Finished */
#if defined(NANOKERNEL)
    okl4_kthread_exit();
#else
    while(1);
#endif
}

void
pingpong(void)
{
    int error;
    int pings = 0;
    struct args args;
    okl4_word_t bytes;
    okl4_kcap_t peer;

    okl4_init_thread();

    // Get args from parent
    error = okl4_message_wait(&args, sizeof(args), &bytes, NULL);
    assert(!error);
    assert(bytes == sizeof(args));
    printf("%s is alive.\n", args.id ? "Pong" : "Ping");

    // Thread 0 is PING, Thread 1 is PONG
    if (args.id == 0)
        peer = args.peers[1];
    else
        peer = args.peers[0];

    while (pings < args.pings)
    {
        okl4_word_t recv;
        if (args.id == 0)
        {
            //printf("ping %d\n", pings);
            // Equivalent to a send followed by a wait
            error = okl4_message_call(peer, &pings, sizeof(pings),
                &recv, sizeof(recv), &bytes);
            assert(!error);
        }
        else
        {
            error = okl4_message_wait(&recv, sizeof(recv), &bytes, NULL);
            assert(!error);
            assert(recv == pings);
            //printf("pong %d\n", pings);
            error = okl4_message_send(peer, &pings, sizeof(pings));
            assert(!error);
        }
        pings++;
    }

    // Finished
    printf("%s is finished. Total %ss: %d\n",
            args.id ? "Pong" : "Ping",
            args.id ? "pong" : "ping",
            pings);
    error = okl4_message_send(args.parent, &pings, sizeof(pings));
    assert(!error);

#if defined(NANOKERNEL)
    okl4_kthread_exit();
#else
    while(1);
#endif
}

