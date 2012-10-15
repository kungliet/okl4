/* @LICENCE("Public", "2008")@ */

#include <stdio.h>
#include <stdlib.h>

#include <okl4/init.h>
#include <okl4/kernel.h>
#include <okl4/message.h>

/*
 * The maximum number of characters we are willing to receive each IPC.
 * Any characters sent by our clients in excess of this will be dropped.
 */
#define MAX_CHARS  OKL4_MESSAGE_MAX_SIZE

int
main(int argc, char **argv)
{
    int error;
    okl4_word_t bytes;
    char buffer[MAX_CHARS];
    okl4_kcap_t client;

    /* Initialise the libokl4 API for this thread. */
    okl4_init_thread();
    printf("ECHO SERVER INITIALISED\n\n");

    /* Wait for the first message to come in. */
    error = okl4_message_wait(buffer, MAX_CHARS, &bytes, &client);
    assert(!error);

    /* Main server loop. */
    while (1) {
        okl4_word_t i;

        /* If the number of bytes sent by the client exceeds our buffer
         * size, we would have lost some of the message. */
        if (bytes > MAX_CHARS) {
            bytes = MAX_CHARS;
        }

        /* Print out the message. */
        printf("ECHO: ");
        for (i = 0; i < bytes; i++) {
            putchar(buffer[i]);
        }
        printf("\n");

        /* Reply to the client, letting them know how many bytes
         * we processed, and wait for the next message to arrive. */
        error = okl4_message_replywait(client, &bytes, sizeof(okl4_word_t),
                buffer, MAX_CHARS, &bytes, &client);
        assert(!error);
    }

    /* Not reached. */
    while (1);
}

