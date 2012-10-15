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

"""
Support for the AMD64 ABI.
"""

import elf.abi
from elf.abi import ElfRelocType
from elf.constants import EM_X86_64

class AMD64RelocType(ElfRelocType):
    """ElfRelocType for AMD-64 processors. """
    _show = {}

R_X86_64_NONE = AMD64RelocType(0, "NONE")
R_X86_64_64 = AMD64RelocType(1, "64")
R_X86_64_PC32 = AMD64RelocType(2, "PC32")
R_X86_64_GOT32 = AMD64RelocType(3, "GOT32")
R_X86_64_PLT32 = AMD64RelocType(4, "PLT32")
R_X86_64_COPY = AMD64RelocType(5, "COPY")
R_X86_64_GLOB_DAT = AMD64RelocType(6, "GLOB_DAT")
R_X86_64_JUMP_SLOT = AMD64RelocType(7, "JUMP_SLOT")
R_X86_64_RELATIVE = AMD64RelocType(8, "RELATIVE")
R_X86_64_GOTPCREL = AMD64RelocType(9, "GOTPCREL")
R_X86_64_32 = AMD64RelocType(10, "32")
R_X86_64_32S = AMD64RelocType(11, "32S")
R_X86_64_16 = AMD64RelocType(12, "16")
R_X86_64_PC16 = AMD64RelocType(13, "PC16")
R_X86_64_8 = AMD64RelocType(14, "8")
R_X86_64_PC8 = AMD64RelocType(15, "PC8")
R_X86_64_DTPMOD64 = AMD64RelocType(16, "DTPMOD64")
R_X86_64_DTPOFF64 = AMD64RelocType(17, "DTPOFF64")
R_X86_64_TPOFF64 = AMD64RelocType(18, "TPOFF64")
R_X86_64_TLSGD = AMD64RelocType(19, "TLSGD")
R_X86_64_TLSLD = AMD64RelocType(20, "TLSLD")
R_X86_64_DTPOFF32 = AMD64RelocType(21, "DTPOFF32")
R_X86_64_GOTTPOFF = AMD64RelocType(22, "GOTTPOFF")
R_X86_64_TPOFF32 = AMD64RelocType(23, "TPOFF32")
R_X86_64_PC64 = AMD64RelocType(24, "PC64")
R_X86_64_GOTOFF64 = AMD64RelocType(25, "GOTOFF64")
R_X86_64_GOTPC32 = AMD64RelocType(26, "GOTPC32")
R_X86_64_SIZE32 = AMD64RelocType(32, "SIZE32")
R_X86_64_SIZE64 = AMD64RelocType(33, "SIZE64")
R_X86_64_GOTPC32_TLSDESC = AMD64RelocType(34, "GOTPC32_TLSDESC")
R_X86_64_TLSDESC_CALL = AMD64RelocType(35, "TLSDESC_CALL")
R_X86_64_TLSDESC = AMD64RelocType(36, "TLSDESC")

elf.abi.ABI_REGISTRATION[EM_X86_64] = AMD64RelocType
