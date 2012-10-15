##############################################################################
# Copyright (c) 2007 Open Kernel Labs, Inc. (Copyright Holder).
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
This module contains classes which can be used an manipulated by the user.
These are analogues of the various underlying classes which represent the
structures in their raw file format.
"""

import sys

from elf.constants import STT_NOTYPE, STT_OBJECT, STT_FUNC, STT_SECTION, \
     ElfSymbolVisibility, SHN_UNDEF, SHN_COMMON, SHN_LOPROC, SHN_HIPROC
from elf.util import align_up

class ElfSymbol(object):
    """Represents an ELF symbol"""

    type_strings = {STT_NOTYPE : "Unspecified",
                    STT_OBJECT : "Data",
                    STT_FUNC : "Code",
                    STT_SECTION : "Section",
                    }

    def __init__(self, name, section, sym_struct=None, old_sym=None):
        """
        Create a new symbol with a given name associated with the given
        section. Either sym_struct or old_sym can be passed in to initialise
        the symbol values, otherwise they are all set to zero.
        """
        self.name = name
        # The section this symbol points to (as opposed to the symbol table,
        # which it lives in)
        self.section = section
        if sym_struct:
            self.value = sym_struct.st_value
            self.size = sym_struct.st_size
            self.type = sym_struct.st_type
            self.bind = sym_struct.st_bind
            self.other = sym_struct.st_other
            self.shndx = sym_struct.st_shndx
        elif old_sym:
            self.value = old_sym.value
            self.size = old_sym.size
            self.type = old_sym.type
            self.bind = old_sym.bind
            self.other = old_sym.other
            self.shndx = old_sym.shndx
        else:
            self.value = 0
            self.size = 0
            self.type = 0
            self.bind = 0
            self.other = 0
            self.shndx = 0

        self.got_offset = None

    def copy_into(self, section):
        """
        Copy this symbol in a new section.

        Returns a new ElfSymbol which belongs to the given section.
        """
        return ElfSymbol(self.name, section, old_sym=self)

    def get_type_string(self):
        """
        Return a string representing the symbol's type. Returns
        unknown if we don't know what the type value means.
        """
        return ElfSymbol.type_strings.get(self.type, "Unknown")

    def __repr__(self):
        """Return a representation of the ElfSymbol instance."""
        rep =  "<ElfSymbol (%s) %s @ 0x%x Size: %d " % \
              (self.get_type_string(), str(self.name), self.value, self.size)
        if self.section and self.section.name:
            rep += self.section.name
        return rep + ">"

    def output(self, idx, file_=sys.stdout, wide=False):
        """
        Pretty print the symbol to a given file, which defaults to
        stdout.
        """
        print >> file_, "%6d:" % idx,
        print >> file_, "%8.8x" % self.value,
        print >> file_, "%5d" % self.size,
        print >> file_, "%-7s" % self.type,
        print >> file_, "%-6s" % self.bind,
        print >> file_, "%3s" % ElfSymbolVisibility(self.other % 0x3),
        print >> file_, "%3.3s" % self.shndx,
        print >> file_, {True: "%s", False: "%-.25s"}[wide] % self.name

    def _allocate_sym_from_section(self, elf, sect_name):
        """
        Allocate space in the requested section for this symbol.
        """
        sect = elf.allocate_section(sect_name)

        # We must byte align to the symbols value field
        sect_end_addr = sect.address + sect.get_size()
        sect_aligned_addr = align_up(sect_end_addr, self.value)
        padding = sect_aligned_addr - sect_end_addr

        sect.increment_size(padding + self.size)

        # Now update the symbol's info to point to the section
        self.section = sect
        self.value = sect_aligned_addr - sect.address
        self.shndx = 0

    def _set_sym_from_section(self, sect):
        """
        Point the symbol at the given section.
        """
        self.section = sect
        self.value = 0
        self.shndx = 0

    def allocate(self, elf):
        """
        Allocate any data that this symbol needs e.g. space in the .bss section.
        """
        if self.type == STT_OBJECT and self.shndx == SHN_COMMON:
            # SHN_COMMON symbols need to have space put for them in the
            # .bss section
            self._allocate_sym_from_section(elf, '.bss')
        elif self.shndx == SHN_UNDEF:
            if self.name == '_GLOBAL_OFFSET_TABLE_':
                # Point the symbol at the .got section
                self._set_sym_from_section(elf.allocate_section('.got'))
            elif self.name == '_SDA_BASE_':
                # Point the symbol at the .sdata section
                self._set_sym_from_section(elf.allocate_section('.sdata'))
        elif self.shndx >= SHN_LOPROC and self.shndx <= SHN_HIPROC:
            self._allocate_sym_from_section(elf, '.sbss')
        elif self.type == STT_NOTYPE and self.shndx == SHN_COMMON:
            self._allocate_sym_from_section(elf, '.sbss')

    def update(self, elf):
        """
        Update the value field of this symbol with the address of the section
        that the symbol is linked to.
        """
        if self.section != None:
            self.value += self.section.address

        if self.got_offset != None:
            got = elf.find_section_named('.got')
            got.set_word_at(self.got_offset, self.value)

    def update_merged_sections(self, merged_sects):
        """
        If this symbol points to any of the merged sections then update it to
        point at the new section and modify the value by the offset.
        """
        for old_sect, new_sect, offset in merged_sects:
            if self.section == old_sect:
                self.section = new_sect
                self.value += offset

                return

class ElfReloc(object):
    """Represents an ELF relocation"""

    def __init__(self, offset=0, type_=0, symdx=0, reloc_struct=None):
        if reloc_struct:
            self.offset = reloc_struct.r_offset
            self.type = reloc_struct.r_type
            self.symdx = reloc_struct.r_symdx
        else:
            self.offset = offset
            self.type = type_
            self.symdx = symdx

        self.verbose = False

    def _output(self, file_):
        """
        Pretty print the relocation to a given file.
        """
        print >> file_, "%8.8x" % self.offset,
        print >> file_, "%8.8x" % self.type,
        print >> file_, "%s" % self.symdx

    def output(self, file_=sys.stdout, wide=False):
        """
        Pretty print the relocation to a given file, which defaults to
        stdout.
        """
        self._output(file_)
        print >> file_

    def to_struct(self, struct_cls, endianess, machine):
        """
        Create and populate a struct of the given class and endianess
        using the values from this user structure.
        """
        relocstruct = struct_cls(endianess, machine)
        relocstruct.r_offset = self.offset
        relocstruct.r_symdx = self.symdx
        relocstruct.r_type = self.type
        return relocstruct

    def apply(self, elf, sect, symtab):
        """
        Apply this relocation to the specified section using the provided
        symbol table.
        """
        addend = sect.get_word_at(self.offset)

        if self.verbose:
            print "\tApplying relocation (REL)", self.type
            print "\t\tSection", sect.name
            print "\t\tOffset %x" % self.offset
            print "\t\tAddend %x" % addend
            print "\t\tSymdx", self.symdx
            print "\t\tSymbol (deref)", symtab.symbols[self.symdx]

        #pylint: disable-msg=E1103
        val = self.type.calculate(elf, sect, symtab, addend, self.offset,
                                  self.symdx)

        if self.verbose:
            if val == None or val == True:
                print "\tCalculated value is", val
            else:
                print "\tCalculated value is %x" % val

        # Catch both not knowing how to handle this reloc as well as the
        # reloc_none (which returns True)
        if val != None and val != True:
            sect.set_word_at(self.offset, val)

        return val != None

    def allocate(self, elf, sect, symtab):
        """
        Allocate any data that this relocation requires e.g. adding .got
        entries.
        """
        if self.verbose:
            print "\tAllocating relocation (REL)", self.type
            print "\t\tSection", sect.name
            print "\t\tOffset %x" % self.offset
            print "\t\tSymdx", self.symdx
            print "\t\tSymbol (deref)", symtab.symbols[self.symdx]

        #pylint: disable-msg=E1103
        self.type.allocate(elf, sect, symtab, self.offset, self.symdx)

    def update_merged_sections(self, offset):
        """
        These relocations point at a merged section, modify thier offsets by
        the given offset so they point at the correct data to modify.
        """
        self.offset += offset

    def update_discarded_symbols(self, discarded_syms):
        """
        Update the symbol index of this relocation to match what it should be
        after dropping some symbols.  Discarded_syms holds a list of the symbol
        index's that have been dropped, pull everything above the index's down.
        """
        decrement = 0

        for x in discarded_syms:
            if self.symdx > x:
                decrement += 1
            else:
                break

        self.symdx -= decrement

    def update_merged_symbols(self, merged_syms):
        """
        Update this relocation to point at the correct new symbol index using
        the mappings provided.
        """
        # If we refer to the null symbol (for whatever reason) this will always
        # be entry 0 in the symbol table, so we don't need to udpate
        if self.symdx == 0:
            return

        for old_ind, new_ind in merged_syms:
            if self.symdx == old_ind:
                self.symdx = new_ind
                return

        raise Exception, "Unable to find new symbol matching when updating relocation"

    def set_verbose(self, verbose):
        self.verbose = verbose
        self.type.verbose = verbose

class ElfReloca(ElfReloc):
    """Represents an ELF RELA relocation"""

    def __init__(self, offset=0, type_=0, symdx=0, addend=0, reloc_struct=None):
        ElfReloc.__init__(self, offset, type_, symdx, reloc_struct)
        if reloc_struct:
            self.addend = reloc_struct.r_addend
        else:
            self.addend = addend

    def output(self, file_=sys.stdout, wide=False):
        """
        Pretty print the relocation to a given file, which defaults to
        stdout.
        """
        self._output(file_)
        print >> file_, "%8.8x" % self.addend

    def to_struct(self, struct_cls, endianess, machine):
        """
        Create and populate a struct of the given class and endianess
        using the values from this user structure.
        """
        relocstruct = struct_cls(endianess, machine)
        relocstruct.r_offset = self.offset
        relocstruct.r_symdx = self.symdx
        relocstruct.r_type = self.type
        relocstruct.r_addend = self.addend
        return relocstruct

    def apply(self, elf, sect, symtab):
        """
        Apply this relocation to the specified section using the provided
        symbol table.
        """
        if self.verbose:
            print "\tApplying relocation (RELA)", self.type
            print "\t\tSection", sect.name
            print "\t\tOffset %x" % self.offset
            print "\t\tAddend %x" % self.addend
            print "\t\tSymdx", self.symdx
            print "\t\tSymbol (deref)", symtab.symbols[self.symdx]

        #pylint: disable-msg=E1103
        val = self.type.calculate(elf, sect, symtab, self.addend, self.offset,
                                  self.symdx)

        if self.verbose:
            if val == None or val == True:
                print "\tCalculated value is", val
            else:
                print "\tCalculated value is %x" % val

        # Catch both not knowing how to handle this reloc as well as the
        # reloc_none (which returns True)
        if val != None and val != True:
            sect.set_word_at(self.offset, val)

        return val != None
