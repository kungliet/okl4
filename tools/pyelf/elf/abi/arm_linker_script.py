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
Provide implementations of our linker scripts in a python format.
"""

import elf.abi
from elf.constants import EM_ARM, SHT_GROUP, SHT_ARM_EXIDX, SHF_ALLOC, \
        SHF_EXECINSTR, SHF_GROUP
from weaver.linker_commands import Memory, Section, Merge, Align, Symbol, Pad, \
        Discard, MergeFlags, DiscardType

def arm_gnu_kernel_soc_link(section_vaddr):
    """
    Perform the kernel + soc link, use a command syntax similar to the gnu
    linker.  Appropiate sections and merged and updated, returns the list of
    segments and a list of the merged and discarded sections.
    """
    return [Memory(0xF0000000),
            Section('.text',
                    [Symbol('_start_rom'),
                     Align(0x10000),
                     Merge(['.init.head',
                            '.init.start',
                            '.base',
                            '.text.startup',
                            '.text',
                            '.gnu.linkonce.*'])]),
            Section('.rodata', [Merge(['.rodata', '.rodata.*'])]),
            Section('.kdebug', [Merge(['.kdebug'])]),
            Symbol('_end_rom'),
            Symbol('_start_init'),
            Section('.init', [Merge(['.init', '.init.*'])]),
            Section('.roinit', [Merge(['.roinit'])]),
            Section('.elfweaver_info', [Merge(['.elfweaver_info'])]),
            Symbol('_end_init'),
            Memory(0xF1000000),
            Section('.data',
                    [Symbol('_start_ram'),
                     Symbol('_start_data'),
                     Pad(0x100),
                     Merge(['.data.cpulocal',
                            '.data.cpulocal.stack',
                            '.data.cpulocal.utcb',
                            '.data.cpulocal.tcb',
                            '.sdata',
                            '.data'])]),
            Section('.got', [Merge(['.got'])]),
            Section('.got.plt', [Merge(['.got.plt'])]),
            Section('.kdebug-data', [Merge(['.data.kdebug', '.kdebug.data'])],
                    base_sect = '.kdebug.data'),
            Section('.bss',
                    [Align(0x10),
                     Symbol('_start_setlist'),
                     Merge(['.setlist']),
                     Symbol('_end_setlist'),
                     Align(0x10),
                     Symbol('_start_sets'),
                     # SORT(set_*)
                     Symbol('_end_sets'),
                     Align(0x20),
                     Merge(['.bss', '.kdebug-bss'])], eos = True),
            Align(0x4000),
            Section('.kspace', [Merge(['.data.kspace'])],
                    base_sect = '.data.kspace'),
            Align(0x1000),
            Section('.traps', [Merge(['.data.traps'])],
                    base_sect = '.data.traps'),
            Section('.utcb_page', [Merge(['.data.utcb'])],
                    base_sect = '.data.utcb'),
            Align(0x1000),
            Symbol('_end_ram'),
            Align(0x1000),
            Symbol('_end'),
            Discard(['.eh_frame', '.note', '.comment', '.delete'])]

def arm_rvct_kernel_soc_link(section_vaddr):
    """
    Perform the kernel + soc link, use a command syntax similar to the gnu
    linker.  Appropiate sections and merged and updated, returns the list of
    segments and a list of the merged and discarded sections.
    """
    return [Memory(0xF0000000),
            Symbol('._start_rom'),
            Section('ER_ROOT', [Merge(['.init.head', '.init.start'])],
                    base_sect = '.init.head'),
            #Section('ER_RO', [MergeFlags(SHF_ALLOC),
            #                  MergeFlags(SHF_ALLOC | SHF_GROUP)],
            #        base_sect = '.constdata'),
            Symbol('._end_rom'),
            Symbol('._start_init'),
            #Section('ER_INIT', [Merge(['.init', 'i.*'])], base_sect = '.init'),
            Section('ER_INIT', [MergeFlags(SHF_ALLOC | SHF_EXECINSTR),
                                MergeFlags(SHF_ALLOC | SHF_EXECINSTR | SHF_GROUP)],
                    base_sect = '.init'),
            Symbol('._end_init'),
            Section('.roinit', [Merge(['.roinit'])]),
            Section('.elfweaver_info', [Merge(['.elfweaver_info'])]),
            Memory(0xF1000000),
            Symbol('._start_ram'),
            # XXX: I assume the .got section is being rolled into ._start_got
            # this should be looked into at some point
            Section('ER_GOT', [Merge(['._start_got'])],
                    base_sect = '._start_got'),
            Symbol('._end_got'),
            Section('ER_TRAPS', [Merge(['.traps.data'])],
                    base_sect = '.traps.data'),
            #Section('ER_RW', [MergeFlags(SHF_ALLOC | SHF_WRITE),
            #                  MergeFlags(SHF_ALLOC | SHF_WRITE | SHF_GROUP)],
            #        base_sect = '.data'),
            Section('ER_KSPACE', [Merge(['.data.kspace'])],
                    base_sect = '.data.kspace'),
            Symbol('._start_bss'),
            Section('ER_ZI', [Merge(['.bss'])], base_sect = '.bss'),
            Symbol('._end_bss'),
            Symbol('._end_ram'),
            Symbol('._end'),
            Section('.debug_abbrev', [Merge(['.debug_abbrev*'])]),
            Section('.debug_frame', [Merge(['.debug_frame*'])],
                    base_sect = '.debug_abbrev'),
            Section('.debug_info', [Merge(['.debug_info*'])],
                    base_sect = '.debug_abbrev'),
            Section('.debug_line', [Merge(['.debug_line*'])],
                    base_sect = '.debug_abbrev'),
            Section('.debug_loc', [Merge(['.debug_loc*'])],
                    base_sect = '.debug_abbrev'),
            Section('.debug_macinfo', [Merge(['.debug_macinfo*'])],
                    base_sect = '.debug_abbrev'),
            Section('.debug_pubnames', [Merge(['.debug_pubnames*'])],
                    base_sect = '.debug_abbrev'),
            DiscardType([SHT_GROUP, SHT_ARM_EXIDX])]

elf.abi.KERNEL_SOC_LINKER_SCRIPT[EM_ARM] = {'gnu' : arm_gnu_kernel_soc_link,
                                            'rvct' : arm_rvct_kernel_soc_link}
