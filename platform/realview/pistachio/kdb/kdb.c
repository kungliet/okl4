/*
 * Copyright (c) 2006, National ICT Australia (NICTA)
 */
/*
 * Copyright (c) 2007 Open Kernel Labs, Inc. (Copyright Holder).
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
 * Description: ARM versatile console
 */

#if defined(CONFIG_KDB_CONS)
#include <soc/soc.h>
#include <kernel/arch/asm.h>
#include <console.h>
#include <soc.h>

static void console_putc(char c);
static char console_getc(bool block);
static void console_init(void);

/****************************************************************************
 *
 *    JTAG console support
 *
 ****************************************************************************/

#if !defined(CONSOLE_PORT_SERIAL)

/*
 * Note: For inline assembly in C++, the RCVT and GCC compilers work close
 *       enough that a single block of ASM can be used.  When you go to C,
 *       there are _more_ syntatcic differences. Hence the repeated blocks for
 *       the jtag console.
 */
static void console_putc(char c)
{
    if (c == '\n') {
        console_putc('\r');
    }
#if defined(__RVCT_GNU__)
    __asm__ __volatile__ (
        "   mov     r2, #2                      "
        "   mov     r3, #3072                   "
        "retry_send: nop                        "
        "   cmp     r3, #0                      "
        "   beq     finish_send                 "
        "   sub     r3, r3, #1                  "
        "   mrc     p14, 0, r1, c0, c0          "
        "   ands    r1, r1, r2                  "
        "   bne     retry_send                  "
        "   mcr     p14, 0, " _(c) ", c1, c0    "
        "finish_send: nop                       "
        );
#else
    /* Wait until the debug channel is clear, then put the character in the
     * debug register.  If this fails after 3072 tries, then don't place
     * the character in debug register -- this allows the phone to run even
     * without a JTAG and without the term.view window open.
     *
     * **NOTE** The 3072 number will need to be tweaked for different
     * processors and clock speeds as to not exit before the T32 software
     * can read the byte; otherwise, some characters will not be written
     * out.
     */
    __asm__ __volatile__ (
        "   mov     r2, #2                      ;"
        "   mov     r3, #3072                   ;"
        "retry_send:                            ;"
        "   cmp     r3, #0                      ;"
        "   beq     finish_send                 ;"
        "   sub     r3, r3, #1                  ;"
        "   mrc     p14, 0, r1, c0, c0          ;"
        "   ands    r1, r1, r2                  ;"
        "   bne     retry_send                  ;"
        "   mcr     p14, 0, " _(c) ", c1, c0    ;"
        "finish_send:                           ;"
#if defined(__GNUC__)
        :: [c] "r" (c)
        : "r1", "r2", "r3"
#endif
        );
#endif
}

static char console_getc(bool block)
{
    word_t c = 0;

#if defined(__RVCT_GNU__)
    __asm__ __volatile__ (
        "   mov         r2, #1                  "
        "   cmp         " _(block) ", #0        "  //are we blocking ?
        "   beq         non_blocking            "  //if so just get once
        "get_again:   nop                       "
        "   mrc         p14, 0, r1, c0, c0      "  //get character
        "   and         r1, r1, r2              "
        "   cmp         r1, #0                  "  //got a character ?
        "   beq         get_again               "  //if not, try again
        "   mrc         p14, 0, " _(c) ", c1, c0"
        "   b           done                    "  //if so, just leave
        "non_blocking: nop                      "
        "   mrc         p14, 0, r1, c0, c0      "  //get character
        "   and         r1, r1, r2              "
        "   cmp         r1, #0                  "  //got a character ?
        "   mrcne       p14, 0, " _(c) ", c1, c0"
        "   moveq       " _(c) ", #0xff         "
        "done: nop                              "
        );
#else
    __asm__ __volatile__ (
        "   mov         r2, #1                  ;"
        "   cmp         " _(block) ", #0        ;"  //are we blocking ?
        "   beq         non_blocking            ;"  //if so just get once
        "get_again:                             ;"
        "   mrc         p14, 0, r1, c0, c0      ;"  //get character
        "   and         r1, r1, r2              ;"
        "   cmp         r1, #0                  ;"  //got a character ?
        "   beq         get_again               ;"  //if not, try again
        "   mrc         p14, 0, " _(c) ", c1, c0;"
        "   b           done                    ;"  //if so, just leave
        "non_blocking:                          ;"
        "   mrc         p14, 0, r1, c0, c0      ;"  //get character
        "   and         r1, r1, r2              ;"
        "   cmp         r1, #0                  ;"  //got a character ?
        "   mrcne       p14, 0, " _(c) ", c1, c0;"
        "   moveq       " _(c) ", #0xff         ;"
        "done:                                  ;"
#if defined(__GNUC__)
        : [c] "=r" (c)
        : [block] "r" (block)
#endif
        );
#endif
    return c;
}

#else

/****************************************************************************
 *
 *    Serial console support
 *
 ****************************************************************************/

/* Serial UART Hardware Constants. */
#define DATAR           0x00000000
#define STATUSR         0x00000018

#define CONFIG_PL011_CLOCK    (24000000)
#define DEFAULT_BAUD 115200

#define UART_PL011_IBRD                 0x24
#define UART_PL011_FBRD                 0x28
#define UART_PL011_LCRH                 0x2C
#define UART_PL011_CR                   0x30
#define UART_PL011_IMSC                 0x38
#define UART_PL011_PERIPH_ID0           0xFE0

#define UART_PL011_LCRH_SPS             (1 << 7)
#define UART_PL011_LCRH_WLEN_8          (3 << 5)
#define UART_PL011_LCRH_WLEN_7          (2 << 5)
#define UART_PL011_LCRH_WLEN_6          (1 << 5)
#define UART_PL011_LCRH_WLEN_5          (0 << 5)
#define UART_PL011_LCRH_FEN             (1 << 4)
#define UART_PL011_LCRH_STP2            (1 << 3)
#define UART_PL011_LCRH_EPS             (1 << 2)
#define UART_PL011_LCRH_PEN             (1 << 1)
#define UART_PL011_LCRH_BRK             (1 << 0)

#define UART_PL011_CR_CTSEN             (1 << 15)
#define UART_PL011_CR_RTSEN             (1 << 14)
#define UART_PL011_CR_OUT2              (1 << 13)
#define UART_PL011_CR_OUT1              (1 << 12)
#define UART_PL011_CR_RTS               (1 << 11)
#define UART_PL011_CR_DTR               (1 << 10)
#define UART_PL011_CR_RXE               (1 << 9)
#define UART_PL011_CR_TXE               (1 << 8)
#define UART_PL011_CR_LPE               (1 << 7)
#define UART_PL011_CR_IIRLP             (1 << 2)
#define UART_PL011_CR_SIREN             (1 << 1)
#define UART_PL011_CR_UARTEN            (1 << 0)

#define UART_PL011_IMSC_OEIM            (1 << 10)
#define UART_PL011_IMSC_BEIM            (1 << 9)
#define UART_PL011_IMSC_PEIM            (1 << 8)
#define UART_PL011_IMSC_FEIM            (1 << 7)
#define UART_PL011_IMSC_RTIM            (1 << 6)
#define UART_PL011_IMSC_TXIM            (1 << 5)
#define UART_PL011_IMSC_RXIM            (1 << 4)
#define UART_PL011_IMSC_DSRMIM          (1 << 3)
#define UART_PL011_IMSC_DCDMIM          (1 << 2)
#define UART_PL011_IMSC_CTSMIM          (1 << 1)
#define UART_PL011_IMSC_RIMIM           (1 << 0)

static unsigned char console_initialised = 0;

static void console_init(void)
{
	unsigned int temp;
	unsigned int divider;
	unsigned int remainder;
	unsigned int fraction;
	volatile char *base = (char *)versatile_uart0_vbase;
	const unsigned int baudRate = DEFAULT_BAUD;

	/*
	 ** First, disable everything.
	 */
	*((volatile word_t *)(base + UART_PL011_CR)) = 0x0;
	

	/*
	 ** Set baud rate
	 **
	 ** IBRD = UART_CLK / (16 * BAUD_RATE)
	 ** FBRD = ROUND((64 * MOD(UART_CLK,(16 * BAUD_RATE))) / (16 * BAUD_RATE))
	 */
	temp = 16 * baudRate;
	divider = CONFIG_PL011_CLOCK / temp;
	remainder = CONFIG_PL011_CLOCK % temp;
	temp = (8 * remainder) / baudRate;
	fraction = (temp >> 1) + (temp & 1);

	*((volatile word_t *)(base + UART_PL011_IBRD)) = divider;
	*((volatile word_t *)(base + UART_PL011_FBRD)) = fraction;

	/*
	 ** Set the UART to be 8 bits, 1 stop bit, no parity, fifo enabled.
	 */
	*((volatile word_t *)(base + UART_PL011_LCRH)) = UART_PL011_LCRH_WLEN_8 | UART_PL011_LCRH_FEN;

	/*
	 ** Finally, enable the UART
	 */
	*((volatile word_t *)(base + UART_PL011_CR)) = UART_PL011_CR_UARTEN | UART_PL011_CR_TXE | UART_PL011_CR_RXE;

	return;
}

static void console_putc(char c)
{
    if (console_initialised) {
        volatile char *base = (char *)versatile_uart0_vbase;

        if (c == '\n') {
            console_putc('\r');
        }

        while ( ((*(volatile long *)(base + STATUSR)) & 0x20) != 0 );
        *(volatile unsigned char *)(base + DATAR)  = c;
    }
}

static char console_getc(bool block)
{
    if (console_initialised) {
        volatile char *base = (char *)versatile_uart0_vbase;

        int c = 0;

        if (!((*(volatile long *)(base + STATUSR)) & 0x10)) {
            c = *(volatile unsigned char *) (base + DATAR);
        } else {
            c = -1;
        }

        return c;
    }

    return -1;
}

#endif /* ! CONSOLE_PORT_SERIAL */

void soc_console_putc(char c)
{
    console_putc(c);
}

int soc_console_getc(bool can_block)
{
    return console_getc(can_block);
}

void soc_kdb_init(void)
{
	console_init();    /* andy */
#if defined(CONSOLE_PORT_SERIAL)
    console_initialised = 1;
#endif
}

#endif /* CONFIG_KDB_CONS */
