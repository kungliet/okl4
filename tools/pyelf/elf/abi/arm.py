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
Support for the ARM ABI.
"""

import elf.abi
from elf.abi import ElfRelocType
from elf.abi.common import reloc_calc_S_add_A, reloc_calc_S_add_A_sub_P, \
        reloc_calc_B_S_add_A_sub_P, reloc_calc_GOT_S_add_A_sub_GOT_ORG, \
        reloc_alloc_got_symbol, reloc_calc_S_add_A_sub_GOT_ORG
from elf.constants import EM_ARM

class ArmRelocType(ElfRelocType):
    """ElfRelocType for ARM processors. """
    _show = {}

def reloc_calc_arm_branch(elf, sect, symtab, addend, offset, symdx):
    """
    Do much arch specific magic.

    The ARM ELF spec doesn't seem to explicitly tell us what to do for this
    relocation (one assumes due to R_ARM_PC24, R_ARM_PLT32, etc. being
    deprecated) so this implementation is dervied from the binutils
    implementation.  Despite not doing anything to do with PLT's this
    function works correctly for simple R_ARM_PLT32's.
    """
    # We need to preserve the op code for when we return the final value
    op = addend & 0xff000000

    # This is a 24bit operation so we and in only the bits we want
    addend &= 0xffffff
    # Sign extension of the addend
    addend |= 0xff000000
    # Addend needs to be converted into bytes (*4)
    addend <<= 2

    # Now calculate the generic part
    val = reloc_calc_S_add_A_sub_P(elf, sect, symtab, addend, offset, symdx)

    # Convert back from bytes and cut back to 24 bits
    val >>= 2
    val &= 0xffffff

    return op | val

R_ARM_NONE = ArmRelocType(0, "NONE")
R_ARM_PC24 = ArmRelocType(1, "PC24", reloc_calc_arm_branch)
R_ARM_ABS32 = ArmRelocType(2, "ABS32", reloc_calc_S_add_A)
R_ARM_REL32 = ArmRelocType(3, "REL32")
R_ARM_LDR_PC_G0 = ArmRelocType(4, "LDR_PC_G0")
R_ARM_ABS16 = ArmRelocType(5, "ABS16")
R_ARM_ABS12 = ArmRelocType(6, "ABS12")
R_ARM_THM_ABS5 = ArmRelocType(7, "THM_ABS5")
R_ARM_ABS8 = ArmRelocType(8, "ABS8")
R_ARM_SBREL32 = ArmRelocType(9, "SBREL32")
R_ARM_THM_CALL = ArmRelocType(10, "THM_CALL")
R_ARM_THM_PC8 = ArmRelocType(11, "THM_PC8")
R_ARM_BREL_ADJ = ArmRelocType(12, "BREL_ADJ")
R_ARM_TLS_DESC = ArmRelocType(13, "TLS_DESC")
R_ARM_THM_SWI8 = ArmRelocType(14, "THM_SWI8")
R_ARM_XPC25 = ArmRelocType(15, "XPC25")
R_ARM_THM_XPC22 = ArmRelocType(16, "THM_XPC22")
R_ARM_TLS_DTPMOD32 = ArmRelocType(17, "TLS_DTPMOD32")
R_ARM_TLS_DTPOFF32 = ArmRelocType(18, "TLS_DTPOFF32")
R_ARM_TLS_TPOFF32 = ArmRelocType(19, "TLS_TPOFF32")
R_ARM_COPY = ArmRelocType(20, "COPY")
R_ARM_GLOB_DAT = ArmRelocType(21, "GLOB_DAT")
R_ARM_JUMP_SLOT = ArmRelocType(22, "JUMP_SLOT")
R_ARM_RELATIVE = ArmRelocType(23, "RELATIVE")
R_ARM_GOTOFF32 = ArmRelocType(24, "GOTOFF32", reloc_calc_S_add_A_sub_GOT_ORG, reloc_alloc_got_symbol)
R_ARM_BASE_PREL = ArmRelocType(25, "BASE_PREL", reloc_calc_B_S_add_A_sub_P)
R_ARM_GOT_BREL = ArmRelocType(26, "GOT_BREL", reloc_calc_GOT_S_add_A_sub_GOT_ORG, reloc_alloc_got_symbol)
R_ARM_PLT32 = ArmRelocType(27, "PLT32", reloc_calc_arm_branch)
R_ARM_CALL = ArmRelocType(28, "CALL")
R_ARM_JUMP24 = ArmRelocType(29, "JUMP24")
R_ARM_THM_JUMP24 = ArmRelocType(30, "THM_JUMP24")
R_ARM_BASE_ABS = ArmRelocType(31, "BASE_ABS")
R_ARM_ALU_PCREL_7_0 = ArmRelocType(32, "ALU_PCREL_7_0")
R_ARM_ALU_PCREL_15_8 = ArmRelocType(33, "ALU_PCREL_15_8")
R_ARM_ALU_PCREL_23_15 = ArmRelocType(34, "ALU_PCREL_23_15")
R_ARM_LDR_SBREL_11_0_NC = ArmRelocType(35, "LDR_SBREL_11_0_NC")
R_ARM_ALU_SBREL_19_12_NC = ArmRelocType(36, "ALU_SBREL_19_12_NC")
R_ARM_ALU_SBREL_27_20_CK = ArmRelocType(37, "ALU_SBREL_27_20_CK")
R_ARM_TARGET1 = ArmRelocType(38, "TARGET1")
R_ARM_SBREL31 = ArmRelocType(39, "SBREL31")
R_ARM_V4BX = ArmRelocType(40, "V4BX")
R_ARM_TARGET2 = ArmRelocType(41, "TARGET2")
R_ARM_PREL31 = ArmRelocType(42, "PREL31")
R_ARM_MOVW_ABS_NC = ArmRelocType(43, "MOVW_ABS_NC")
R_ARM_MOVT_ABS = ArmRelocType(44, "MOVT_ABS")
R_ARM_MOVW_PREL_NC = ArmRelocType(45, "MOVW_PREL_NC")
R_ARM_MOVT_PREL = ArmRelocType(46, "MOVT_PREL")
R_ARM_THM_MOVW_ABS_NC = ArmRelocType(47, "THM_MOVW_ABS_NC")
R_ARM_THM_MOVT_ABS = ArmRelocType(48, "THM_MOVT_ABS")
R_ARM_THM_MOVW_PREL_NC = ArmRelocType(49, "THM_MOVW_PREL_NC")
R_ARM_THM_MOVT_PREL = ArmRelocType(50, "THM_MOVT_PREL")
R_ARM_THM_JUMP19 = ArmRelocType(51, "THM_JUMP19")
R_ARM_THM_JUMP6 = ArmRelocType(52, "THM_JUMP6")
R_ARM_THM_ALU_PREL_11_0 = ArmRelocType(53, "THM_ALU_PREL_11_0")
R_ARM_THM_PC12 = ArmRelocType(54, "THM_PC12")
R_ARM_ABS32_NOI = ArmRelocType(55, "ABS32_NOI")
R_ARM_REL32_NOI = ArmRelocType(56, "REL32_NOI")
R_ARM_ALU_PC_G0_NC = ArmRelocType(57, "ALU_PC_G0_NC")
R_ARM_ALU_PC_G0 = ArmRelocType(58, "ALU_PC_G0")
R_ARM_ALU_PC_G1_NC = ArmRelocType(59, "ALU_PC_G1_NC")
R_ARM_ALU_PC_G1 = ArmRelocType(60, "ALU_PC_G1")
R_ARM_ALU_PC_G2 = ArmRelocType(61, "ALU_PC_G2")
R_ARM_LDR_PC_G1 = ArmRelocType(62, "LDR_PC_G1")
R_ARM_LDR_PC_G2 = ArmRelocType(63, "LDR_PC_G2")
R_ARM_LDRS_PC_G0 = ArmRelocType(64, "LDRS_PC_G0")
R_ARM_LDRS_PC_G1 = ArmRelocType(65, "LDRS_PC_G1")
R_ARM_LDRS_PC_G2 = ArmRelocType(66, "LDRS_PC_G2")
R_ARM_LDC_PC_G0 = ArmRelocType(67, "LDC_PC_G0")
R_ARM_LDC_PC_G1 = ArmRelocType(68, "LDC_PC_G1")
R_ARM_LDC_PC_G2 = ArmRelocType(69, "LDC_PC_G2")
R_ARM_ALU_SB_G0_NC = ArmRelocType(70, "ALU_SB_G0_NC")
R_ARM_ALU_SB_G0 = ArmRelocType(71, "ALU_SB_G0")
R_ARM_ALU_SB_G1_NC = ArmRelocType(72, "ALU_SB_G1_NC")
R_ARM_ALU_SB_G1 = ArmRelocType(73, "ALU_SB_G1")
R_ARM_ALU_SB_G2 = ArmRelocType(74, "ALU_SB_G2")
R_ARM_LDR_SB_G0 = ArmRelocType(75, "LDR_SB_G0")
R_ARM_LDR_SB_G1 = ArmRelocType(76, "LDR_SB_G1")
R_ARM_LDR_SB_G2 = ArmRelocType(77, "LDR_SB_G2")
R_ARM_LDRS_SB_G0 = ArmRelocType(78, "LDRS_SB_G0")
R_ARM_LDRS_SB_G1 = ArmRelocType(79, "LDRS_SB_G1")
R_ARM_LDRS_SB_G2 = ArmRelocType(80, "LDRS_SB_G2")
R_ARM_LDC_SB_G0 = ArmRelocType(81, "LDC_SB_G0")
R_ARM_LDC_SB_G1 = ArmRelocType(82, "LDC_SB_G1")
R_ARM_LDC_SB_G2 = ArmRelocType(83, "LDC_SB_G2")
R_ARM_MOVW_BREL_NC = ArmRelocType(84, "MOVW_BREL_NC")
R_ARM_MOVT_BREL = ArmRelocType(85, "MOVT_BREL")
R_ARM_MOVW_BREL = ArmRelocType(86, "MOVW_BREL")
R_ARM_THM_MOVW_BREL_NC = ArmRelocType(87, "THM_MOVW_BREL_NC")
R_ARM_THM_MOVT_BREL = ArmRelocType(88, "THM_MOVT_BREL")
R_ARM_THM_MOVW_BREL = ArmRelocType(89, "THM_MOVW_BREL")
R_ARM_TLS_GOTDESC = ArmRelocType(90, "TLS_GOTDESC")
R_ARM_TLS_CALL = ArmRelocType(91, "TLS_CALL")
R_ARM_TLS_DESCSEQ = ArmRelocType(92, "TLS_DESCSEQ")
R_ARM_THM_TLS_CALL = ArmRelocType(93, "THM_TLS_CALL")
R_ARM_PLT32_ABS = ArmRelocType(94, "PLT32_ABS")
R_ARM_GOT_ABS = ArmRelocType(95, "GOT_ABS")
R_ARM_GOT_PREL = ArmRelocType(96, "GOT_PREL")
R_ARM_GOT_BREL12 = ArmRelocType(97, "GOT_BREL12")
R_ARM_GOTOFF12 = ArmRelocType(98, "GOTOFF12")
R_ARM_GOTRELAX = ArmRelocType(99, "GOTRELAX")
R_ARM_GNU_VTENTRY = ArmRelocType(100, "GNU_VTENTRYLS_GD32")
R_ARM_TLS_LDM32 = ArmRelocType(105, "TLS_LDM32")
R_ARM_TLS_LDO32 = ArmRelocType(106, "TLS_LDO32")
R_ARM_TLS_IE32 = ArmRelocType(107, "TLS_IE32")
R_ARM_TLS_LE32 = ArmRelocType(108, "TLS_LE32")
R_ARM_TLS_LDO12 = ArmRelocType(109, "TLS_LDO12")
R_ARM_TLS_LE12 = ArmRelocType(110, "TLS_LE12")
R_ARM_TLS_IE12GP = ArmRelocType(111, "TLS_IE12GP")
R_ARM_PRIVATE_0 = ArmRelocType(112, "PRIVATE_0")
R_ARM_PRIVATE_1 = ArmRelocType(113, "PRIVATE_1")
R_ARM_PRIVATE_2 = ArmRelocType(114, "PRIVATE_2")
R_ARM_PRIVATE_3 = ArmRelocType(115, "PRIVATE_3")
R_ARM_PRIVATE_4 = ArmRelocType(116, "PRIVATE_4")
R_ARM_PRIVATE_5 = ArmRelocType(117, "PRIVATE_5")
R_ARM_PRIVATE_6 = ArmRelocType(118, "PRIVATE_6")
R_ARM_PRIVATE_7 = ArmRelocType(119, "PRIVATE_7")
R_ARM_PRIVATE_8 = ArmRelocType(120, "PRIVATE_8")
R_ARM_PRIVATE_9 = ArmRelocType(121, "PRIVATE_9")
R_ARM_PRIVATE_10 = ArmRelocType(122, "PRIVATE_10")
R_ARM_PRIVATE_11 = ArmRelocType(123, "PRIVATE_11")
R_ARM_PRIVATE_12 = ArmRelocType(124, "PRIVATE_12")
R_ARM_PRIVATE_13 = ArmRelocType(125, "PRIVATE_13")
R_ARM_PRIVATE_14 = ArmRelocType(126, "PRIVATE_14")
R_ARM_PRIVATE_15 = ArmRelocType(127, "PRIVATE_15")
R_ARM_ME_TOO = ArmRelocType(128, "ME_TOO")
R_ARM_THM_TLS_DESCSEQ16 = ArmRelocType(129, "THM_TLS_DESCSEQ16")
R_ARM_THM_TLS_DESCSEQ32 = ArmRelocType(130, "THM_TLS_DESCSEQ32")

elf.abi.ABI_REGISTRATION[EM_ARM] = ArmRelocType
