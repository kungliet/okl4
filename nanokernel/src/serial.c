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

/*
 * Serial output.
 */

#include <stdarg.h>

#include <nano.h>
#include <serial.h>

#if defined(CONFIG_DEBUG)

/* Output lock. */
static spinlock_t output_lock;

/*
 * Print a string.
 */
int
puts(const char *string)
{
    spin_lock(&output_lock);
    int i;
    for (i = 0; string[i] != 0; i++) {
        putchar(string[i]);
    }
    spin_unlock(&output_lock);
    return i;
}

/*
 * Print a hexadecimal constant.
 */
static int
print_hex(unsigned int x)
{
    const char *characters = "0123456789abcdef";
    for (int i = 28; i >= 0; i -= 4)
        putchar(characters[((x >> i) & 0xf)]);
    return 8;
}

/*
 * Print a decimal constant.
 */
static int
print_dec(int x)
{
    char buff[16];

    /* Signed? */
    unsigned int val;
    if (x < 0) {
        val = -x;
        putchar('-');
    } else {
        val = x;
    }

    /* Calculate the characters. */
    int n = 0;
    do {
        buff[n++] = (val % 10) + '0';
        val /= 10;
    } while (val > 0);

    /* Print out the characters. */
    for (int i = n - 1; i >= 0; i--)
        putchar(buff[i]);

    return n;
}

/*
 * Simple printf-like function.
 */
void
vprintk(const char *string, va_list args)
{
    spin_lock(&output_lock);
    int control = 0;
    for (int i = 0; string[i] != 0; i++) {
        if (!control) {
            if (string[i] == '%') {
                control = 1;
            } else {
                putchar(string[i]);
            }
        } else {
            switch (string[i]) {
                case '%':
                    putchar('%');
                    break;
                case 'd':
                    print_dec(va_arg(args, int));
                    break;
                case 'x':
                case 'p':
                    print_hex(va_arg(args, int));
                    break;
                default:
                    putchar('?');
                    break;
            }
            control = 0;
        }
    }
    spin_unlock(&output_lock);
}

/*
 * Kernel printing.
 */
void
printk(const char *string, ...)
{
    va_list args;
    va_start(args, string);
    vprintk(string, args);
    va_end(args);
}

#endif

