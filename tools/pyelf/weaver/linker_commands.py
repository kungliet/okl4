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
Provide functionality to model linker scripts as a series of nested commands.
"""

from elf.constants import STT_OBJECT, STB_GLOBAL, STV_DEFAULT, SHN_ABS, \
        SHF_WRITE, SHF_ALLOC, SHF_EXECINSTR, SHF_GROUP, SHF_ALLOC
from elf.user_structures import ElfSymbol
from elf.util import align_up
from elf.ByteArray import ByteArray

SEGMENT_ALIGN = 0x8000

def merge_sections(base_sect, merge_sect, merged_sects, remove_sects, verbose):
    if verbose:
        print "\tMerging in section %s to section %s" % (merge_sect.name,
                                                         base_sect.name)

    size = base_sect.get_size()
    offset = align_up(size, merge_sect.addralign)

    for _ in range(0, offset - size):
        base_sect.append_data(ByteArray('\0'))

    base_sect.append_data(merge_sect.get_data())
    merged_sects.append((merge_sect, base_sect, offset))

    if remove_sects != None:
        remove_sects.append(merge_sect)

    if verbose:
        print "\tPadded by %x bytes" % (offset - size)
        print "\tMerged data starts at %x" % offset

class Command:
    """
    Top level class, sole purpose of existance is to hold a collection of
    variables that are shared between all the commands and drastically reduces
    the need to pass multiple arguments around.
    """

    elf = None
    sym_tab = None
    section = None
    segment = None
    addr = 0
    merged_sects = []
    discarded_sects = []
    sections = []
    segments = []
    verbose = False

    def __init__(self):
        pass

    def parse(self):
        pass

class Memory(Command):
    """
    Simulate the memory section of a gnu linker script, sets the base point
    that we start addressing from after the command.  Instead of having a
    MEMORY {} section then referring to the entries (rom, ram, etc.) in it
    all following sections are assumed to go into the section described by
    the base and size until a new memory entry is found.
    """

    def __init__(self, base):
        Command.__init__(self)

        self.base = base

    def parse(self):
        if Command.verbose:
            print "Memory object"

        Command.addr = align_up(self.base, SEGMENT_ALIGN)
        Command.segment = [None, Command.addr, []]
        Command.segments.append(Command.segment)

        if Command.verbose:
            print "\tSetting address to %x" % Command.addr
            print "\tAdding new segment at index %d" % \
                    Command.segments.index(Command.segment)

class Section(Command):
    """
    Simulate the section command in the gnu linker script.  Creates a section
    named name based on base_sect.  If not provided base_sect is assumed to
    be the same as name.  Can have a collection of commands within it to merge
    other sections or define symbols.  eos is End of Segment and is used to
    tell sections (such as the .bss) that they need to start a new segment.
    """

    def __init__(self, name, commands = None, eos = False, base_sect = None):
        Command.__init__(self)

        if commands is None:
            commands = []

        self.name = name
        self.commands = commands
        self.eos = eos
        self.base_sect = name

        if base_sect:
            self.base_sect = base_sect

    def parse(self):
        if Command.verbose:
            print "Section object"

        sect = Command.elf.find_section_named(self.base_sect)

        if sect == None:
            if Command.verbose:
                print "\tNot creating section, unable to find base section %s" \
                        % self.base_sect

            return
        elif Command.verbose:
            print "\tCreating section %s based on base section %s" % \
                    (self.name, self.base_sect)

        Command.section = Command.elf.clone_section(sect, Command.addr)
        Command.sections.append(Command.section)

        # Now set the address of the section given the current address
        Command.section.address = align_up(Command.addr,
                                           Command.section.addralign)
        Command.addr = Command.section.address

        # XXX: This checks works for the current rvct but if we are merging
        # sections that aren't going into a segment they should not affect
        # the address value at all.  Fix later.  (Related to merging all the
        # 8000+ .debug_foo$$$morefoo sections in RVCT)
        if Command.section.flags & SHF_ALLOC:
            if len(Command.segment[2]) == 0:
                Command.segment[1] = Command.addr

            Command.segment[2].append(Command.section)

        if self.name != self.base_sect:
            Command.section.name = self.name

        if Command.verbose:
            print "\tSection address is %x" % Command.section.address
            print "\tParsing commands..."

        for command in self.commands:
            command.parse()

        if self.eos:
            Command.addr = align_up(Command.addr, SEGMENT_ALIGN)
            Command.segment = [None, Command.addr, []]
            Command.segments.append(Command.segment)

            if Command.verbose:
                print "Section %s marks end of segment" % self.name
                print "\tSetting address to %x" % Command.addr
                print "\tAdding new segment at index %d" % \
                        Command.segments.index(Command.segment)

        Command.section = None

        if Command.verbose:
            print "Finished section %s" % self.name

class Merge(Command):
    """
    Merge the list of sections into the current section being created.
    """

    def __init__(self, sections):
        Command.__init__(self)

        self.sections = sections

    def parse(self):
        if Command.verbose:
            print "Merge object"

        remove_sects = []

        for sect in self.sections:
            if sect[-1] == '*':
                if Command.verbose:
                    print "\tMerging in all sections %s" % sect

                glob_name = sect[:-1]

                for merge_sect in Command.elf.sections:
                    if merge_sect.name.find(glob_name) == 0 and \
                            merge_sect.name != glob_name and \
                            merge_sect != Command.section:
                        merge_sections(Command.section, merge_sect,
                                       Command.merged_sects, remove_sects,
                                       Command.verbose)
            else:
                merge_sect = Command.elf.find_section_named(sect)

                if not merge_sect:
                    if Command.verbose:
                        print "\tUnable to find section %s to merge" % sect

                    continue

                merge_sections(Command.section, merge_sect,
                               Command.merged_sects, remove_sects,
                               Command.verbose)

        for sect in remove_sects:
            Command.elf.remove_section(sect)

        # Reset the current address based on the extra data added
        Command.addr = Command.section.address + Command.section.get_size()

        if Command.verbose:
            print "\tSetting address to %x" % Command.addr

class MergeFlags(Command):
    """
    Merge all sections with the given flags (read, write, execute).
    """

    def __init__(self, flags):
        Command.__init__(self)

        self.flags = flags

    def parse(self):
        if Command.verbose:
            print "MergeFlags object"
            print "\tMerging all sectioins with flags %d" % self.flags
            print "\t\t(A %d, W %d, X %d, G %d)" % (self.flags & SHF_ALLOC > 0,
                                                    self.flags & SHF_WRITE > 0,
                                                    self.flags & SHF_EXECINSTR > 0,
                                                    self.flags & SHF_GROUP > 0)

        remove_sects = []

        for sect in Command.elf.sections:
            print sect.name
            print sect.flags
            if sect.flags == self.flags:
                merge_sections(Command.section, sect, Command.merged_sects,
                               remove_sects, Command.verbose)

        for sect in remove_sects:
            Command.elf.remove_section(sect)

        # Reset the current address based on the extra data added
        Command.addr = Command.section.address + Command.section.get_size()

        if Command.verbose:
            print "\tSetting address to %x" % Command.addr

class Align(Command):
    """
    Align the current address to the given value.
    """

    def __init__(self, value):
        Command.__init__(self)

        self.value = value

    def parse(self):
        if Command.verbose:
            print "Align object"

        old_addr = Command.addr
        Command.addr = align_up(Command.addr, self.value)

        if Command.verbose:
            print "\tSetting address to %x" % Command.addr

        if Command.section:
            for _ in range(0, Command.addr - old_addr):
                Command.section.append_data(ByteArray('\0'))

            if Command.verbose:
                print "\tPadding current section by %x bytes" % (Command.addr -
                                                                 old_addr)

class Symbol(Command):
    """
    Define a symbol at the current address.
    """

    def __init__(self, name):
        Command.__init__(self)

        self.name = name

    def parse(self):
        class SymbolStruct:
            def __init__(self, value, size, sym_type, bind, other, shndx):
                self.st_value = value
                self.st_size = size
                self.st_type = sym_type
                self.st_bind = bind
                self.st_other = other
                self.st_shndx = shndx

        if Command.verbose:
            print "Symbol object"

        sym = Command.sym_tab.get_symbol(self.name)
        if sym:
            sym.value = Command.addr
            sym.type = STT_OBJECT
            sym.shndx = SHN_ABS
            sym.section = None
        else:
            sym_st = SymbolStruct(Command.addr, 0, STT_OBJECT, STB_GLOBAL,
                                  STV_DEFAULT, SHN_ABS)
            Command.sym_tab.add_symbol(ElfSymbol(self.name, None, sym_st))

        if Command.verbose:
            print "\tCreated symbol %s at address %x" % (self.name,
                                                         Command.addr)

class Pad(Command):
    """
    Pad out memory by a given number of bytes.
    """

    def __init__(self, value):
        Command.__init__(self)

        self.value = value

    def parse(self):
        if Command.verbose:
            print "Pad object"

        old_addr = Command.addr
        Command.addr += self.value

        if Command.verbose:
            print "\tSetting address to %x" % Command.addr

        if Command.section:
            for _ in range(0, Command.addr - old_addr):
                Command.section.append_data(ByteArray('\0'))

            if Command.verbose:
                print "\tPadding current section by %x bytes" % (Command.addr -
                                                                 old_addr)

class Discard(Command):
    """
    Discard the list of sections.
    """

    def __init__(self, sections):
        Command.__init__(self)

        self.sections = sections

    def parse(self):
        if Command.verbose:
            print "Discard object"

        remove_sects = []

        for sect in self.sections:
            if sect[-1] == '*':
                if Command.verbose:
                    print "\tDiscard all sections %s" % sect

                glob_name = sect[:-1]

                for discard_sect in Command.elf.sections:
                    if discard_sect.name.find(glob_name) == 0:
                        Command.discarded_sects.append(discard_sect)
                        remove_sects.append(discard_sect)

                        if Command.verbose:
                            print "\t\tDiscarding section %s" % \
                                    discard_sect.name
            else:
                discard_sect = Command.elf.find_section_named(sect)
                if not discard_sect:
                    if Command.verbose:
                        print "\tUnable to find section %s to discard" % sect

                    continue

                Command.discarded_sects.append(discard_sect)
                remove_sects.append(discard_sect)

                if Command.verbose:
                    print "\tDiscarding section %s" % sect

        for sect in remove_sects:
            Command.elf.remove_section(sect)

class DiscardType(Command):
    """
    Discard sections based on their types.
    """

    def __init__(self, types):
        Command.__init__(self)

        self.types = types

    def parse(self):
        if Command.verbose:
            print "DiscardType object"
            print "\tDiscarding sections with types", \
                  [sect_type.__str__() for sect_type in self.types]

        remove_sects = []

        for sect in Command.elf.sections:
            if sect.type in self.types:
                Command.discarded_sects.append(sect)
                remove_sects.append(sect)

                if Command.verbose:
                    print "\t\tDiscarding section %s" % sect.name

        for sect in remove_sects:
            Command.elf.remove_section(sect)
