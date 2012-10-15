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

#ifndef __NAND_DEVICES_H_
#define __NAND_DEVICES_H_

#include <stdlib.h>

struct nand_manf {
    uint8_t     manf_id;
    uint8_t*    manf_name;
};

struct nand_cmd_set;

struct nand_device {
    uint8_t     manf_id;
    uint8_t     chip_id;
    uint8_t*    chip_name;

    uint32_t    blocks;
    uint32_t    block_size;
    uint32_t    erase_size;
    uint32_t    page_size;
    uint32_t    oob_size;

    struct nand_cmd_set* cmd_set;
};

#define NAND_CMD_RESET          0xff
#define NAND_CMD_READ_ID        0x90
#define NAND_CMD_READ_STATUS    0x70
#define NAND_CMD_READ_HALF0     0x00    // First half of page
#define NAND_CMD_READ_HALF1     0x01    // Second half of page
#define NAND_CMD_READ_OOB       0x50
#define NAND_CMD_BLOCK_ERASE    0x60
#define NAND_CMD_ERASE_CONFIRM  0xd0
#define NAND_CMD_WRITE          0x80
#define NAND_CMD_WRITE_CONFIRM  0x10

#define NAND_STATUS_BUSY        0x40

#if 0
enum nand_cmd_id {
    NAND_CMD_RESET,
    NAND_CMD_READ_ID,
    NAND_CMD_READ_STATUS,
    NAND_CMD_READ,
    NAND_CMD_READ_OOB,
    NAND_CMD_BLOCK_ERASE,
    // XXX more here
};

struct nand_cmd {
    int         supported;  // Chip supports this command
    uint8_t     cmd[4];     // Up to 4-byte commands for now
    uint8_t     len;        // cmd length
    int         busy_ok;    // can call when device busy
};

struct nand_cmd_set {
    struct nand_cmd cmds[16];
};
#endif

extern struct nand_manf nand_manf_ids[];
extern struct nand_device nand_device_ids[];

#endif /*__NAND_DEVICES_H_*/
