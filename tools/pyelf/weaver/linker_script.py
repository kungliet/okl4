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

from weaver.linker_commands import Command, Memory, Section, Merge, Align

def perform_link(elf, sym_tab, command_func, section_vaddr, verbose):
    """
    Perform a link using the provided command function to generate the list
    of commands.
    """

    Command.elf = elf
    Command.sym_tab = sym_tab
    Command.verbose = verbose

    commands = command_func(section_vaddr)

    for command in commands:
        command.parse()

    for sect in Command.sections:
        Command.elf.add_section(sect)

    return Command.segments, Command.merged_sects, Command.discarded_sects

def standard_link(section_vaddr):
    """
    Perform a standard link using a simple form of the command syntax.  Returns
    the segments to use as well as any merged or discarded sections.
    """

    text_addr = 0x08000000
    data_addr = None

    if section_vaddr:
        if section_vaddr.has_key('.text'):
            text_addr = section_vaddr['.text']
        if section_vaddr.has_key('.data'):
            data_addr = section_vaddr['.data']

    commands = [Memory(text_addr),
                Section('.text', [Merge(['.text', '.text.*'])]),
                Section('.rodata', [Merge(['.rodata', '.rodata.*'])],
                        eos = True)]

    if data_addr:
        commands.append(Memory(data_addr))

    commands.extend([Section('.data', [Merge(['.data', '.data.*'])]),
                     Section('.got', [Merge(['.got'])]),
                     Section('.got.plt', [Merge(['.got.plt'])]),
                     Section('.bss', [Merge(['.bss'])], eos = True)])

    return commands

def zonetests_link(_):
    """
    Perform a zonetest link as per the iguana example zonetests linker script.
    """

    return [Memory(0x80500000),
            Section('.text', [Merge(['.text'])]),
            Section('.rodata', [Merge(['.rodata', '.rodata.*'])], eos = True),
            Align(0x100000),
            Section('.data', [Merge(['.data'])]),
            Section('.got', [Merge(['.got'])]),
            Section('.got.plt', [Merge(['.got.plt'])]),
            Section('.bss', [Merge(['.bss'])], eos = True),
            Align(0x100000),
            Section('.zone', [Merge(['.zone'])])]
