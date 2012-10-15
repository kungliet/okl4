##############################################################################
# Copyright (c) 2008 Open Kernel Labs, Inc. (Copyright Holder).
# All rights reserved.
# 
# 1. Redistribution and use of OKL4 (Software) in source and binary
# forms, with or without modification, are permitted provided that the
# following conditions are met:
# 
#     (a) Redistributions of source code must retain this clause 1
#         (including paragraphs (a), (b) and (c)), clause 2 and clause 3
#         (Licence Terms) and the above copyright notice.
# 
#     (b) Redistributions in binary form must reproduce the above
#         copyright notice and the Licence Terms in the documentation and/or
#         other materials provided with the distribution.
# 
#     (c) Redistributions in any form must be accompanied by information on
#         how to obtain complete source code for:
#        (i) the Software; and
#        (ii) all accompanying software that uses (or is intended to
#        use) the Software whether directly or indirectly.  Such source
#        code must:
#        (iii) either be included in the distribution or be available
#        for no more than the cost of distribution plus a nominal fee;
#        and
#        (iv) be licensed by each relevant holder of copyright under
#        either the Licence Terms (with an appropriate copyright notice)
#        or the terms of a licence which is approved by the Open Source
#        Initative.  For an executable file, "complete source code"
#        means the source code for all modules it contains and includes
#        associated build and other files reasonably required to produce
#        the executable.
# 
# 2. THIS SOFTWARE IS PROVIDED ``AS IS'' AND, TO THE EXTENT PERMITTED BY
# LAW, ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
# PURPOSE, OR NON-INFRINGEMENT, ARE DISCLAIMED.  WHERE ANY WARRANTY IS
# IMPLIED AND IS PREVENTED BY LAW FROM BEING DISCLAIMED THEN TO THE
# EXTENT PERMISSIBLE BY LAW: (A) THE WARRANTY IS READ DOWN IN FAVOUR OF
# THE COPYRIGHT HOLDER (AND, IN THE CASE OF A PARTICIPANT, THAT
# PARTICIPANT) AND (B) ANY LIMITATIONS PERMITTED BY LAW (INCLUDING AS TO
# THE EXTENT OF THE WARRANTY AND THE REMEDIES AVAILABLE IN THE EVENT OF
# BREACH) ARE DEEMED PART OF THIS LICENCE IN A FORM MOST FAVOURABLE TO
# THE COPYRIGHT HOLDER (AND, IN THE CASE OF A PARTICIPANT, THAT
# PARTICIPANT). IN THE LICENCE TERMS, "PARTICIPANT" INCLUDES EVERY
# PERSON WHO HAS CONTRIBUTED TO THE SOFTWARE OR WHO HAS BEEN INVOLVED IN
# THE DISTRIBUTION OR DISSEMINATION OF THE SOFTWARE.
# 
# 3. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR ANY OTHER PARTICIPANT BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
# BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import struct
from elf.ByteArray import ByteArray

structure = {
    "r0": 0,
    "r1": 4,
    "r2": 8,
    "r3": 12,
    "r4": 16,
    "r5": 20,
    "r6": 24,
    "r7": 28,
    "r8": 32,
    "r9": 36,
    "r10": 40,
    "r11": 44,
    "r12": 48,
    "ip": 48,
    "r13": 52,
    "sp": 52,
    "r14": 56,
    "lr": 56,
    "r15": 60,
    "pc": 60,
    "cpsr": 64,
    "full_context_saved": 68,
    "arch_padding_0": 72,
    "arch_padding_1": 76,
    "arch_padding_2": 80,
    "arch_padding_3": 84,
    "arch_padding_4": 88,
    "arch_padding_5": 92,
    "priority": 96,
    "thread_state": 100,
    "next": 104,
    "prev": 108,
    "futex_tag": 112,
    "tid": 116,
    "ipc_waiting_for": 120,
    "ipc_send_head": 124,
    "signals": 128,
    "signal_mask": 132,
    "registered_interrupt": 136,
    "joiner": 140,
    "generic_padding_0": 144,
    "generic_padding_1": 148,
    "generic_padding_2": 152,
    "generic_padding_3": 156,
    }

def tcb_size():
    """
    Return the size of the tcb structure.
    """
    return 160

def _get_tcb_field(tcb, name):
    """
    Get the field with a given name from a tcb struct
    """
    base = structure[name]
    return struct.unpack('<L', tcb[base:base+4])[0]

def _set_tcb_field(tcb, name, val):
    """
    Set the value of the field with the given name in a tcb struct.
    """
    base = structure[name]
    tcb[base:base+4] = ByteArray(struct.pack('<L', val))

def make_functions(entry):
    """
    This function creates a set of functions in the modue namespace.

    Two functions, a getter and setter, are created for each entry in the
    structure dictionary. These functions allow a user to get or set any
    entry in a tcb struct.
    """
    globals()["set_tcb_" + entry] = lambda tcb, val: _set_tcb_field(tcb, entry, val)
    globals()["get_tcb_" + entry] = lambda tcb: _get_tcb_field(tcb, entry)

# We call make_functions() at import time to populate the module namespace
for entry in structure:
    make_functions(entry)

