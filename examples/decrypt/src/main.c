/* @LICENCE("Public", "2008")@ */

#include <okl4/env.h>
#include <okl4/init.h>
#include <okl4/notify.h>

int
main(int argc, char **argv)
{
    okl4_env_segment_t *ms;
    okl4_kcap_t linux_cap;
    okl4_word_t mask, shift;
    char *c, tmp;

    okl4_init_thread();

    ms = okl4_env_get_segment("MAIN_SHMEM");
    assert(ms != NULL);

    shift = *(okl4_word_t *)okl4_env_get("CAESAR_CIPHER_SHIFT");

    linux_cap = *(okl4_kcap_t *)okl4_env_get("ROOT_CELL_CAP");


    while (1) {
        okl4_notify_wait(DECRYPT_MASK, &mask);

        c = (char *)ms->virt_addr;

        while (*c != '\0') {
            tmp = *c - shift;
            if ((tmp < 65 && (*c >= 65 && *c <= 90)) || (tmp < 97 && (*c >= 97 && *c <= 122))) {
                tmp += 26;
            }
            *c = tmp;
            c++;
        }

        okl4_notify_send(linux_cap, DECRYPT_MASK);
    }
}
