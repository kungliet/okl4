/* @LICENCE("Public", "2008")@ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <okl4/init.h>
#include <okl4/env.h>
#include <okl4/kernel.h>
#include <okl4/message.h>

/* The number of characters to send each IPC. */
#define MAX_CHARS  OKL4_MESSAGE_MAX_SIZE

int
main(int argc, char **argv)
{
    int error;
    okl4_word_t i, bytes, msglen;
    okl4_kcap_t *echo_server;

    /* The message to send to the echo server. */
    char *message =
            "Lorem ipsum dolor sit amet, consectetur adipisicing elit, "
            "sed do eiusmod tempor incididunt ut labore et dolore magna "
            "aliqua. Ut enim ad minim veniam, quis nostrud exercitation "
            "ullamco laboris nisi ut aliquip ex ea commodo consequat. "
            "Duis aute irure dolor in reprehenderit in voluptate velit "
            "esse cillum dolore eu fugiat nulla pariatur. Excepteur sint "
            "occaecat cupidatat non proident, sunt in culpa qui officia "
            "deserunt mollit anim id est laborum.\0";
    msglen = strlen(message) + 1;

    /* Initialise the libokl4 API for this thread. */
    okl4_init_thread();

    /* Get the capability entry for the echo server. */
    echo_server = okl4_env_get("ROOT_CELL_CAP");
    assert(echo_server != NULL);

    /* Send the message. */
    for (i = 0; i < msglen; i += bytes) {
        /* Determine how many bytes to send. */
        if (i < msglen - MAX_CHARS) {
            bytes = MAX_CHARS;
        } else {
            bytes = msglen - i;
        }

        /* Send this chunk to the server. The server will reply the
         * number of bytes actually written. */
        error = okl4_message_call(*echo_server, &message[i], bytes,
                &bytes, sizeof(bytes), NULL);
        assert(!error);
    }
}

