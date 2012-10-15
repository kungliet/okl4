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
Support for the IA64 ABI.
"""

import elf.abi
from elf.abi import ElfRelocType
from elf.constants import EM_IA_64

class IA64RelocType(ElfRelocType):
    """ElfRelocType for IA-64 processors. """
    _show = {}

R_IA_64_NONE = IA64RelocType(0, "NONE")
R_IA_64_IMM14 = IA64RelocType(0x21, "IMM14")
R_IA_64_IMM22 = IA64RelocType(0x22, "IMM22")
R_IA_64_IMM64 = IA64RelocType(0x23, "IMM64")
R_IA_64_DIR32MSB = IA64RelocType(0x24, "DIR32MSB")
R_IA_64_DIR32LSB = IA64RelocType(0x25, "DIR32LSB")
R_IA_64_DIR64MSB = IA64RelocType(0x26, "DIR64MSB")
R_IA_64_DIR64LSB = IA64RelocType(0x27, "DIR64LSB")
R_IA_64_GPREL22 = IA64RelocType(0x2a, "GPREL22")
R_IA_64_GPREL64I = IA64RelocType(0x2b, "GPREL64I")
R_IA_64_GPREL32MSB = IA64RelocType(0x2c, "GPREL32MSB")
R_IA_64_GPREL32LSB = IA64RelocType(0x2d, "GPREL32LSB")
R_IA_64_GPREL64MSB = IA64RelocType(0x2e, "GPREL64MSB")
R_IA_64_GPREL64LSB = IA64RelocType(0x2f, "GPREL64LSB")
R_IA_64_LTOFF22 = IA64RelocType(0x32, "LTOFF22")
R_IA_64_LTOFF64I = IA64RelocType(0x33, "LTOFF64I")
R_IA_64_PLTOFF22 = IA64RelocType(0x3a, "PLTOFF22")
R_IA_64_PLTOFF64I = IA64RelocType(0x3b, "PLTOFF64I")
R_IA_64_PLTOFF64MSB = IA64RelocType(0x3e, "PLTOFF64MSB")
R_IA_64_PLTOFF64LSB = IA64RelocType(0x3f, "PLTOFF64LSB")
R_IA_64_FPTR64I = IA64RelocType(0x43, "FPTR64I")
R_IA_64_FPTR32MSB = IA64RelocType(0x44, "FPTR32MSB")
R_IA_64_FPTR32LSB = IA64RelocType(0x45, "FPTR32LSB")
R_IA_64_FPTR64MSB = IA64RelocType(0x46, "FPTR64MSB")
R_IA_64_FPTR64LSB = IA64RelocType(0x47, "FPTR64LSB")
R_IA_64_PCREL60B = IA64RelocType(0x48, "PCREL60B")
R_IA_64_PCREL21B = IA64RelocType(0x49, "PCREL21B")
R_IA_64_PCREL21M = IA64RelocType(0x4a, "PCREL21M")
R_IA_64_PCREL21F = IA64RelocType(0x4b, "PCREL21F")
R_IA_64_PCREL32MSB = IA64RelocType(0x4c, "PCREL32MSB")
R_IA_64_PCREL32LSB = IA64RelocType(0x4d, "PCREL32LSB")
R_IA_64_PCREL64MSB = IA64RelocType(0x4e, "PCREL64MSB")
R_IA_64_PCREL64LSB = IA64RelocType(0x4f, "PCREL64LSB")
R_IA_64_LTOFF_FPTR22 = IA64RelocType(0x52, "LTOFF_FPTR22")
R_IA_64_LTOFF_FPTR64I = IA64RelocType(0x53, "LTOFF_FPTR64I")
R_IA_64_LTOFF_FPTR32MSB = IA64RelocType(0x54, "LTOFF_FPTR32MSB")
R_IA_64_LTOFF_FPTR32LSB = IA64RelocType(0x55, "LTOFF_FPTR32LSB")
R_IA_64_LTOFF_FPTR64MSB = IA64RelocType(0x56, "LTOFF_FPTR64MSB")
R_IA_64_LTOFF_FPTR64LSB = IA64RelocType(0x57, "LTOFF_FPTR64LSB")
R_IA_64_SEGREL32MSB = IA64RelocType(0x5c, "SEGREL32MSB")
R_IA_64_SEGREL32LSB = IA64RelocType(0x5d, "SEGREL32LSB")
R_IA_64_SEGREL64MSB = IA64RelocType(0x5e, "SEGREL64MSB")
R_IA_64_SEGREL64LSB = IA64RelocType(0x5f, "SEGREL64LSB")
R_IA_64_SECREL32MSB = IA64RelocType(0x64, "SECREL32MSB")
R_IA_64_SECREL32LSB = IA64RelocType(0x65, "SECREL32LSB")
R_IA_64_SECREL64MSB = IA64RelocType(0x66, "SECREL64MSB")
R_IA_64_SECREL64LSB = IA64RelocType(0x67, "SECREL64LSB")
R_IA_64_REL32MSB = IA64RelocType(0x6c, "REL32MSB")
R_IA_64_REL32LSB = IA64RelocType(0x6d, "REL32LSB")
R_IA_64_REL64MSB = IA64RelocType(0x6e, "REL64MSB")
R_IA_64_REL64LSB = IA64RelocType(0x6f, "REL64LSB")
R_IA_64_LTV32MSB = IA64RelocType(0x74, "LTV32MSB")
R_IA_64_LTV32LSB = IA64RelocType(0x75, "LTV32LSB")
R_IA_64_LTV64MSB = IA64RelocType(0x76, "LTV64MSB")
R_IA_64_LTV64LSB = IA64RelocType(0x77, "LTV64LSB")
R_IA_64_PCREL21BIa = IA64RelocType(0x79, "PCREL21BIa")
R_IA_64_PCREL22 = IA64RelocType(0x7A, "PCREL22")
R_IA_64_PCREL64I = IA64RelocType(0x7B, "PCREL64I")
R_IA_64_IPLTMSB = IA64RelocType(0x80, "IPLTMSB")
R_IA_64_IPLTLSB = IA64RelocType(0x81, "IPLTLSB")
R_IA_64_SUB = IA64RelocType(0x85, "SUB")
R_IA_64_LTOFF22X = IA64RelocType(0x86, "LTOFF22X")
R_IA_64_LDXMOV = IA64RelocType(0x87, "LDXMOV")
R_IA_64_TPREL14 = IA64RelocType(0x91, "TPREL14")
R_IA_64_TPREL22 = IA64RelocType(0x92, "TPREL22")
R_IA_64_TPREL64I = IA64RelocType(0x93, "TPREL64I")
R_IA_64_TPREL64MSB = IA64RelocType(0x96, "TPREL64MSB")
R_IA_64_TPREL64LSB = IA64RelocType(0x97, "TPREL64LSB")
R_IA_64_LTOFF_TPREL22 = IA64RelocType(0x9A, "LTOFF_TPREL22")
R_IA_64_DTPMOD64MSB = IA64RelocType(0xA6, "DTPMOD64MSB")
R_IA_64_DTPMOD64LSB = IA64RelocType(0xA7, "DTPMOD64LSB")
R_IA_64_LTOFF_DTPMOD22 = IA64RelocType(0xAA, "LTOFF_DTPMOD22")
R_IA_64_DTPREL14 = IA64RelocType(0xB1, "DTPREL14")
R_IA_64_DTPREL22 = IA64RelocType(0xB2, "DTPREL22")
R_IA_64_DTPREL64I = IA64RelocType(0xB3, "DTPREL64I")
R_IA_64_DTPREL32MSB = IA64RelocType(0xB4, "DTPREL32MSB")
R_IA_64_DTPREL32LSB = IA64RelocType(0xB5, "DTPREL32LSB")
R_IA_64_DTPREL64MSB = IA64RelocType(0xB6, "DTPREL64MSB")
R_IA_64_DTPREL64LSB = IA64RelocType(0xB7, "DTPREL64LSB")
R_IA_64_LTOFF_DTPREL22 = IA64RelocType(0xBA, "LTOFF_DTPREL22")

elf.abi.ABI_REGISTRATION[EM_IA_64] = IA64RelocType
