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
#    pylint: disable-msg=R0902,R0903,R0904,R0913
"""
ELF section types
"""
__revision__ = 1.0

from elf.ByteArray import ByteArray
from elf.constants import SHT_NULL, SHT_STRTAB, SHT_DYNSYM, SHT_NOBITS, \
     SHT_SYMTAB, SHT_PROGBITS, SHT_NOTE, SHT_REL, SHT_RELA, SHN_LORESERVE, \
     SHN_UNDEF, STB_GLOBAL, SHN_ABS
from elf.structures import InvalidArgument, ELF_SH_CLASSES, \
     ELF_SYMBOL_STRUCT_CLASSES, ELF_RELOC_STRUCT_CLASSES, \
     ELF_RELOCA_STRUCT_CLASSES, is_integer
from elf.util import align_up
from elf.user_structures import ElfSymbol, ElfReloc, ElfReloca

import struct

class BaseElfSection(object):
    """
    ElfSection instances represent sections in an ELF file. Note that this
    differs from ElfSectionheader, which just describes the header, not the
    data itself.

    ElfSection objects can exist in two states: prepared and unprepared. The
    subclasses UnpreparedElfSection and PreparedElfSection represent these two
    states.

    In the prepared state the section is ready to be written out to a file, but
    can not be modified (unless the changes maintain certain invariants).

    In the unprepared state the section can be modified, but must be prepared
    for writing before it can actually be written out.

    A section can be intialised from into either the prepared or unprepared
    state. Once in the prepared state, it is not possible to revert to the
    unprepared state.

    An unprepared section has a method .prepare which will return a prepared
    version of the section.

    Two important properties of the section, the wordsize and endianess, are
    only available in the prepared state.

    This class is written with the expectation that it will be sub-classed. To
    create a subclass (e.g. to implement a particular type of section) you need
    to create subclasses following the inheritance pattern below (this pattern
    is a little complicated, but would look much saner in 3D).

                           BaseElfSection
                                 |
             |-------------------------------------------|
             |                      |                    |
    UnpreparedElfSection     PreparedElfSection       BaseSubClass
             |                      |                 |  |
             |                      |-----------------|  |
             |                      |                    |
             |              PreparedSubClass             |
             |                                           |
             |-------------------------------------------|
             |
     UnpreparedSubClass

     class BaseSubClass(BaseElfSection): pass
     class UnpreparedSubClass(UnpreparedElfSection, BaseSubClass): pass
     class PreparedSubClass(PreparedElfSection, BaseSubClass): pass

     In this example, BaseSubClass would provide any class specific methods and
     also override any methods of BaseElfSection. When overriding methods of
     BaseElfSection, it is important to pass these overrides to subclasses.

     For example, if BaseSubClass defines get_size() (overriding
     BaseElfSection.get_size), then each of its subclasses would require a line

     get_size = BaseSubClass.get_size

     This is because python resolves multiple inheritance depth first, left to
     right, so BaseElfSection.get_size would be the first method resolved unless
     the explicit override is given.
    """

    def __init__(self, elffile, name=None, section_type=SHT_NULL, address=0,
                 data=None, flags=0, addralign=0, info=None, link=None,
                 entsize=None):
        self.elffile = elffile
        self.address = address
        self.type = section_type
        self._name = name
        self.flags = flags
        self.addralign = addralign
        self.entsize = entsize
        self.info = info
        self.link = link
        self._backlinks = [] # keeps track of sections which link to us.
        self._in_segment_offset = None
        self.in_segment = False

        if self.address is None:
            self.address = 0

        # If the section is a 'NOBITS' section, then it just has size, and
        # no real data. So _data, may be an integer or a ByteArray depending
        # on the different circumstance
        if data is None:
            if section_type == SHT_NOBITS:
                self._data = 0
            else:
                self._data = ByteArray()
        else:
            self._data = data
        if section_type == SHT_NOBITS:
            assert is_integer(self._data)
        else:
            assert self._data.__class__ == ByteArray

    def __repr__(self):
        return "<%s %s %s>" % (self.__class__.__name__, self.type, self.name)

    def get_size(self):
        """Return the size of the section in bytes."""
        if self.type == SHT_NOBITS:
            return self._data
        else:
            return len(self._data)

    def _get_name(self):
        """Return the name of this segment."""
        return self._name
    name = property(_get_name)

    def _set_link(self, link):
        """Set a related section."""
        if link is not None:
            if not issubclass(link.__class__, BaseElfSection):
                raise InvalidArgument, "Link must be an ElfSection. %s" % \
                      str(link.__class__)
            link._backlinks.append(self)
        self._link = link

    def _get_link(self):
        """Return the related ElfSection."""
        return self._link
    link = property(_get_link, _set_link)

    def get_data(self):
        """
        Return the data the will exist in memory. This may differ
        from what is copied into the file itself.
        """
        if self.type == SHT_NOBITS:
            return ByteArray('\0' * self.get_size())
        else:
            return self._data

    def get_file_data(self, clean=False):
        """
        Return the data that should be copied into the file. The
        data is returned as a byte array.
        """
        if self.type == SHT_NOBITS:
            return ByteArray()
        else:
            if clean:
                return self._data.copy()
            else:
                return self._data

    def remove_nobits(self):
        """If this section is nobits, change it to PROGBITS."""
        if self.type == SHT_NOBITS:
            # Note: Order is important here. self.get_size() is implemented
            # differently depending on NOBITS vs PROGBITS
            self._data = ByteArray('\0' * self.get_size())
            self.type = SHT_PROGBITS

    def get_num_entries(self):
        """Return number of entries in the section."""
        if self.entsize is None:
            raise InvalidArgument, "This section does not use entries"
        return self.get_size() / self.entsize

    def get_entry(self, index):
        """Return the data for a given entry"""
        if self.entsize is None:
            raise InvalidArgument, "This section does not use entries"
        size = self.entsize
        return self._data[index*size:(index+1)*size]

    def get_entries(self):
        """Return a generator of all entries."""
        if self.entsize is None:
            raise InvalidArgument, "This section does not use entries"
        for i in range(self.get_num_entries()):
            yield self.get_entry(i)

    def set_in_segment_offset(self, offset):
        self._in_segment_offset = offset

    def calc_in_segment_offset(self, segment):
        """
        Return the offset into the given segment.
        """
        offset = self._in_segment_offset
        if not segment.is_scatter_load() or offset is None or \
               self.name.startswith('kernel_') or self.name.startswith('ER_'):
            return self.address - segment.vaddr
        else:
            return offset

    def data_trim(self, frm, to):
        """Trim the data so it is just [frm:to]"""
        self._data = self._data[frm:to]

    def info_is_section(self):
        """ Do we expect the .link for this section to be a section itself? """
        return self.type in (SHT_REL, SHT_RELA)

    def _get_wordsize(self):
        """ Wordsize is proxied to the elf file. """
        return self.elffile.wordsize
    wordsize = property(_get_wordsize)

    def _get_endianess(self):
        """ Endianess is proxied to the elf file. """
        return self.elffile.endianess
    endianess = property(_get_endianess)

class PreparedElfSection(BaseElfSection):

    def __init__(self, elffile, index, name_offset=None, offset=None, address=0,
                 section_type=SHT_NULL, name=None, data=None, flags=0,
                 addralign=0, info=None, link=None, entsize=None):
        BaseElfSection.__init__(self, elffile, name, section_type, address,
                                data, flags, addralign, info, link, entsize)

        self._index = index
        self.name_offset = name_offset
        self._offset = offset

    def get_index(self):
        """
        Return the index of the file, that is,its position in the array
        of sections.
        """
        return self._index
    index = property(get_index)

    def get_offset(self):
        """Return the offset of this section in the file."""
        return self._offset
    offset = property(get_offset)

#pylint: disable-msg=E1103
    def get_section_header(self):
        """Return an Elf section header structure."""

        sh = ELF_SH_CLASSES[self.wordsize](self.endianess)
        sh.sh_name = self.name_offset
        sh.sh_type = self.type
        sh.sh_addr = self.address
        sh.sh_offset = self.offset
        sh.sh_size = self.get_size()
        if self.entsize is None:
            sh.sh_entsize = 0
        else:
            sh.sh_entsize = self.entsize
        sh.sh_flags = self.flags
        if isinstance(self.info, PreparedElfSection):
            sh.sh_info = self.info.index
        elif isinstance(self.info, UnpreparedElfSection):
            sh.sh_info = self.info.prepared_to.index
        else:
            if self.info:
                sh.sh_info = self.info
            else:
                sh.sh_info = 0
        sh.sh_addralign = self.addralign
        sh._name = self._name
        sh._index = self.index
        if self.link:
            sh.sh_link = self.link.index
        else:
            sh.sh_link = 0

        return sh

class UnpreparedElfSection(BaseElfSection):

    prepares_to = PreparedElfSection

    def __init__(self, elffile, name=None, section_type=SHT_NULL, address=0,
                 data=None, flags=0, addralign=0, info=None, link=None,
                 entsize=None):

        BaseElfSection.__init__(self, elffile, name, section_type, address,
                                data, flags, addralign, info, link, entsize)
        self.prepared_to = None

        # FIXME: If we create a file without an endianess what should we do?
        # This seems to work but is ugggllly, also prone to breakage if we
        # try to use the enocde/decoder on a file setup without a valid elf.
        if self.elffile != None and self.elffile.endianess != NotImplemented:
            # Figure out the correct format values for writing into the
            # section's data string
            if self.elffile.wordsize == 64:
                word_char = 'Q'
                signed_word_char = 'q'
            else:
                word_char = 'I'
                signed_word_char = 'i'

            self.format_word = self.elffile.endianess + word_char
            self.format_signed_word = self.elffile.endianess + signed_word_char

    def prepare(self, offset, index, name_offset, **kwargs):
        # Make a copy of the data
        cls = self.__class__.prepares_to
        if self.type == SHT_NOBITS:
            data = self._data
        elif cls in [PreparedElfSymbolTable, PreparedElfReloc,
                     PreparedElfReloca]:
            # These classes will have passed their data in via appropriate
            # kwargs
            data = None
        else:
            data = self.get_file_data(clean=True)
            assert data.__class__ == ByteArray

        new_section = cls(self.elffile, index, name_offset, offset,
                          self.address, self.type, self.name, data, self.flags,
                          self.addralign, self.info, self.link, self.entsize,
                          **kwargs)
        # Go through the sections which linked to us and update their link
        for sect in self._backlinks:
            if sect.link == self:
                sect.link = new_section

        # Keep track of the new section, so anyone who happens to point to us
        # can find out what we turned into. In theory this shouldn't be needed
        # and we should aim to make it redundant.
        self.prepared_to = new_section
        return new_section

    def copy_into(self, elffile):
        """
        Copy this section into a different elf file. This creates and returns
        the new section, which retains all the properties or the original,
        including new copies of the sections symbols, which will be placed
        in the new elf file's symbol table.

        The new section is returned.
        """
        # Make a copy of the data
        if self.type == SHT_NOBITS:
            data = self._data
        else:
            data = self.get_file_data(clean=True)
            assert data.__class__ == ByteArray
        new_section = self.__class__(elffile, self.name, self.type,
                                     self.address, data, self.flags,
                                     self.addralign, self.info, self.link)

        new_symbols = [sym.copy_into(new_section) for
                       sym in self.elffile.section_symbols(self)]
        if new_symbols:
            elffile.add_symbols(new_symbols)
        new_section._in_segment_offset = self._in_segment_offset
        return new_section

    def _encode(self, offset, val):
        """
        Encode the provided value at the offset in this section's data.
        """
        if val >= 0:
            self._data[offset:offset+4] = \
                    ByteArray(struct.pack(self.format_word, val))
        else:
            self._data[offset:offset+4] = \
                    ByteArray(struct.pack(self.format_signed_word, val))

    def _decode(self, offset):
        """
        Decode a word at the offset in this section's data.
        """
        return struct.unpack(self.format_word, self._data[offset:offset+4])

    def append_data(self, data):
        """
        Appends data to this section, this data is assumed to already be
        correctedly formatted.
        """
        if self.type == SHT_NOBITS:
            self._data += len(data)
        else:
            self._data.extend(data)

    def append_word(self, val):
        """
        Add a word to the data in the section.
        """
        offset = len(self._data)
        self._data.extend([0, 0, 0, 0])

        self._encode(offset, val)

    def get_word_at(self, offset):
        """
        Returns a decoded word from the data in this section at the given
        offset.
        """
        return self._decode(offset)[0]

    def set_word_at(self, offset, val):
        """
        Enocde a word and write it to the data in this section at the provided
        offset.
        """
        self._encode(offset, val & 0xffffffff)

    def increment_size(self, size):
        """
        Increment the size of the data section, this only work for a NOBITS
        sectoin, sections with actual data should use data_append
        """
        self._data += size

    def _set_name(self, name):
        """
        Set the name of this segment. This can only be called on
        unprepared Sections
        """
        self._name = name
    name = property(BaseElfSection._get_name, _set_name)

class BaseElfStringTable(BaseElfSection):
    """
    Represents an ELF string table (Ref 1-16). This is a subclass of
    the ElfSection object and all those methods are available here.
    """

    def __init__(self):
        # We always start with a null character
        self.offsets = {"": 0}
        self.string_dict = {0: ""}
        self.strings = ['\x00']
        if self._data != ByteArray('\x00'):
            strings = self._data.tostring().split('\x00')[1:-1]
            cum_offset = 1
            for string in strings:
                self.strings.append(string + '\x00')
                self.offsets[string] = cum_offset
                self.string_dict[cum_offset] = string
                cum_offset += len(string) + 1
# Commented out because the else clause doesn't work with the
# .segment_names section, which uses NUL strings to indicate unnamed
# segments.
#                 if string:
#                     self.strings.append(string + '\x00')
#                     self.offsets[string] = cum_offset
#                     self.string_dict[cum_offset] = string
#                     cum_offset += len(string) + 1
#                 else:
#                     # This happens when you have a \0\0 in the packed data
#                     cum_offset += 1

    def get_file_data(self, clean=False):
        """Return an array of bytes representing the data."""
        return ByteArray("".join(self.strings))

    def get_string_idx(self, idx):
        """
        Get the idx-th string. Return None if there is no string at the offset.
        """
        if idx < len(self.strings):
            return self.strings[idx][:-1]
        return None

    def get_string_ofs(self, offset):
        """
        Return the string at a given offset. None if it there is no string.
        """
        string = self.string_dict.get(offset, None)
        if string is None:
            # The offset points inside a string, so we go hunting
            cur_off = 0
            for string in self.strings:
                next_offset = cur_off + len(string)
                if offset < next_offset:
                    return string[offset - cur_off:-1]
                cur_off = next_offset
        return string

    def get_strings(self):
        """Return a list of string in the string table."""
        return [x[:-1] for x in self.strings]

    def get_size(self):
        """Return the size of the string table in bytes"""
        return sum([len(string) for string in self.strings])

    def index_of(self, string):
        """
        Return the index of a given string if it exists in the string
        table. Return None if it doesn't exist.

        Note: This does note work for partial strings (e.g. suffixes)
        """
        try:
            return [s[:-1] for s in self.strings].index(string)
        except ValueError:
            return None

    def offset_of(self, string):
        """
        Return the offset of a given string if it exists in the string
        table. Return None if it doesn't exist.

        This method, unlike index_of, does work with suffixes.
        """
        offset = self.offsets.get(string, None)
        if offset is not None:
            return offset
        # OK, didn't find it the easy way, let's look for it as a suffix.
        string = string + "\x00"
        for s in self.strings:
            if s.endswith(string):
                offset = s.find(string)
                return self.offsets[s[:-1]] + offset
        return None


class PreparedElfStringTable(PreparedElfSection, BaseElfStringTable):

    get_size = BaseElfStringTable.get_size
    get_file_data = BaseElfStringTable.get_file_data

    def __init__(self, elffile, index, name_offset=None, offset=None, address=0,
                 section_type=SHT_STRTAB, name=None, data=None, flags=0,
                 addralign=1, info=None, link=None, entsize=None):

        PreparedElfSection.__init__(self, elffile, index, name_offset, offset,
                                    address, section_type, name, data, flags,
                                    addralign, info, link, entsize)
        BaseElfStringTable.__init__(self)


class UnpreparedElfStringTable(UnpreparedElfSection, BaseElfStringTable):

    prepares_to = PreparedElfStringTable

    def __init__(self, elffile, name=None, section_type=SHT_STRTAB, address=0,
                 data=ByteArray('\x00'), flags=0, addralign=1,
                 info=None, link=None, entsize=None):
        UnpreparedElfSection.__init__(self, elffile, name, section_type,
                                      address, data, flags, addralign,
                                      info, link, entsize)
        BaseElfStringTable.__init__(self)

    get_size = BaseElfStringTable.get_size
    get_file_data = BaseElfStringTable.get_file_data

    def add_string(self, string):
        """Add a new string to the table. Return the data offset."""
        if not string:
            return 0
        if string in self.offsets:
            return self.offsets[string]
        if len(self.offsets) > 1:
            offset = self.offsets[self.strings[-1][:-1]] + len(self.strings[-1])
        else:
            offset = 1
        self.strings.append(string + '\x00')
        self.offsets[string] = offset
        self.string_dict[offset] = string
        return offset



class BaseElfSymbolTable(BaseElfSection):
    """
    Represents an ELF symbol table (Ref 1-17). This is a subclass of
    the BaseElfSection object and all those methods are available here.
    """

    def __init__(self):
        self.symbols = None

    def _get_entsize(self):
        return ELF_SYMBOL_STRUCT_CLASSES[self.wordsize].size()
    entsize = property(_get_entsize, lambda self, val: None)

    def get_num_symbols(self):
        """Return the number of symbols"""
        return len(self.symbols)

    def get_size(self):
        """Return the size of the string table in bytes"""
        return len(self.symbols) * self.entsize

    def _unpack(self):
        """
        Convert out internal raw data into more useful data structures.
        """
        int_syms = []
        data = self.get_data()
        if not data:
            return []

        sym_cls = ELF_SYMBOL_STRUCT_CLASSES[self.wordsize]
        endianess = self.endianess # Optimize these property accesses to speed
        entsize = self.entsize     # up the following loop significantly
        for i in range(entsize, len(data), entsize):
            sym_struct = sym_cls(endianess)
            sym_struct.fromdata(data[i:i+entsize])
            int_syms.append(sym_struct)
        self._data = None # Only unpack once
        return int_syms

class PreparedElfSymbolTable(PreparedElfSection, BaseElfSymbolTable):

    def __init__(self, elffile, index, name_offset=None, offset=None, address=0,
                 section_type=SHT_SYMTAB, name=None, data=None, flags=0,
                 addralign=0, info=None, link=None, entsize=None,
                 int_syms=None):
        PreparedElfSection.__init__(self, elffile, index, name_offset, offset,
                                    address, section_type, name, data, flags,
                                    addralign, info, link, entsize)
        BaseElfSymbolTable.__init__(self)

        self.symbols = int_syms
        self.int_syms = self._symbols_to_int_syms(self.symbols)

    get_size = BaseElfSymbolTable.get_size

    def _symbols_to_int_syms(self, symbols):
        """
        Convert a list of ElfSymbols into actual internal elf structures
        which can be written to a file.
        """
        sym_cls = ELF_SYMBOL_STRUCT_CLASSES[self.wordsize]
        new_syms = [sym_cls(self.endianess)] # start with a null

        endianess = self.endianess # These local variables are created for
        link = self.link           # performance reasons
        for sym in symbols[1:]: # and skip this guy's null

            symstruct = sym_cls(endianess)
            symstruct.st_size = sym.size
            symstruct.st_value = sym.value
            symstruct.st_other = sym.other
            symstruct.st_bind = sym.bind
            symstruct.st_type = sym.type
            if sym.section:
                if isinstance(sym.section, PreparedElfSection):
                    symstruct.st_shndx = sym.section.index
                elif sym.section.prepared_to:
                    symstruct.st_shndx = sym.section.prepared_to.index
                else:
                    if sym.shndx == SHN_ABS:
                        symstruct.st_shndx = SHN_ABS
                    else:
                        symstruct.st_shndx = 0
            else:
                if sym.shndx == SHN_ABS:
                    symstruct.st_shndx = SHN_ABS
                else:
                    symstruct.st_shndx = 0
            index = link.offset_of(sym.name)
            if index is None:
                raise Exception, "%s not found in %s" % \
                      (sym.name, link.offsets.keys())
            symstruct.st_name = index
            new_syms.append(symstruct)
        return new_syms

    def get_symbol_structs(self):
        """
        Return a list of symbol structures. This is only available when the
        section is prepared.
        """
        return self.int_syms

    def get_file_data(self, clean=False):
        """Return an array of bytes representing the data."""
        return ByteArray("".join([sym.todata(as_bytearray=False) for
                                  sym in self.int_syms]))


class UnpreparedElfSymbolTable(UnpreparedElfSection, BaseElfSymbolTable):

    prepares_to = PreparedElfSymbolTable

    def __init__(self, elffile, name=None, section_type=SHT_SYMTAB, address=0,
                 data=None, flags=0, addralign=0, info=None, link=None,
                 entsize=None):
        UnpreparedElfSection.__init__(self, elffile, name, section_type,
                                      address, data, flags, addralign,
                                      info, link, entsize)
        BaseElfSymbolTable.__init__(self)

        # The code below is highly optimized, since we reading in the symbol
        # table is one of the slow parts of loading an elf file.
        # We create local versions of all objects, classes and methods
        # we would otherwise have to repeated be looked up in a deeper scope.
        int_syms = self._unpack() # int_syms are now structures
        symbols = self.symbols = [ElfSymbol(None, None)]

        strtab = self.link = self._get_strtab(link)
        sections = self.elffile.sections
        num_sections = len(sections)
        get_string_ofs = strtab.get_string_ofs
        append = symbols.append
        ES = ElfSymbol
        shn_lo = SHN_LORESERVE
        for sym_struct in int_syms:
            name = get_string_ofs(sym_struct.st_name)
            shndx = sym_struct.st_shndx
            if shndx < num_sections:
                section = sections[shndx]
            elif shndx < shn_lo:
                raise IndexError, \
                      "Invalid section index %d for symbol %s." \
                      "Only %d sections in file." \
                      % (shndx, name, num_sections)
            else:
                section = None
            append(ES(name, section, sym_struct))

    get_size = BaseElfSymbolTable.get_size

    def _get_strtab(self, link):
        """
        We *have* to have a string table. If no specific string table
        is given,  we look for the default (called ".strtab"). If that
        doesn't exist we look for any SH_STRTAB section and use that.
        if *THAT* doesn't work, we go ahead and create a string table
        called ".strtab" and use that.
        """
        if link is None:
            strtab = self.elffile._get_strtable()
            if strtab is None:
                strtab = UnpreparedElfStringTable(self.elffile, ".strtab")
        else:
            strtab = link
        return strtab

    def get_file_data(self, clean=False):
        """Return an array of bytes representing the data."""
        raise NotImplementedError

    def add_symbol(self, symbol):
        """
        Add a new symbol to the section. Only possible when the
        section is unprepared.
        """
        self.symbols.append(symbol)
        self.link.add_string(symbol.name)

    def get_symbol(self, sym_name):
        """
        Return the symbol with the given name if it exists, None otherwise.
        """
        for sym in self.symbols:
            if sym.name == sym_name:
                return sym

        return None

    def allocate_symbols(self):
        """
        Go through all symbols in this table and do any allocation that
        may be required.
        """
        for sym in self.symbols:
            sym.allocate(self.elffile)

    def update_symbols(self):
        """
        Go through all the symbols and update them e.g. bring their value fields
        in line with the section addresses
        """
        for sym in self.symbols:
            sym.update(self.elffile)

    def update_merged_sections(self, merged_sects):
        """
        Update any symbols that poiont to old_sect to point at new_sect using
        offset as the base offsect where old_sect's data starts.
        """
        for sym in self.symbols:
            sym.update_merged_sections(merged_sects)

    def update_discarded_sections(self, discarded_sects):
        """
        Drop any symbols that refer to a section that has been discarded.
        To update any relocations that rely on symbol index's we return a list
        of the index's that have been removed.
        """
        removed_sym_ind = []

        for sym in self.symbols:
            if sym.section in discarded_sects:
                removed_sym_ind.append(self.symbols.index(sym))

        self.symbols = [sym for sym in self.symbols
                        if not sym.section in discarded_sects]

        return removed_sym_ind

    def resolve(self, sym_tab):
        """
        Resolve any undefined global symbols in this symbol table by looking
        through the provided symbol table for a match.  Return a mapping of
        resolved symbols in the form (old_index, new_index)
        """
        resolved_syms = []
        for index in range(1, len(self.symbols)):
            base_sym = self.symbols[index]

            if base_sym.shndx != SHN_UNDEF or base_sym.bind != STB_GLOBAL:
                continue

            for sym in sym_tab.symbols:
                if sym.name == base_sym.name and sym.shndx != SHN_UNDEF and \
                        sym.bind == STB_GLOBAL:
                    resolved_syms.append((sym_tab.symbols.index(sym), index))
                    self.symbols[index] = sym

        for old_ind, _ in resolved_syms:
            sym_tab.symbols[old_ind] = None

        return resolved_syms

    def merge(self, sym_tab):
        """
        Merge the symbols in sym_tab into self.  If the symbol is undefined try
        to resolve it to one already in our symbol table.  Returns mappings in
        the form (old_index, new_index).
        """
        merged_syms = []
        for sym in sym_tab.symbols[1:]:
            if sym == None:
                continue

            if sym.shndx == SHN_UNDEF and sym.bind == STB_GLOBAL:
                # It is an undefined global, try to find a match in our symbol
                # table.  If so we just use that one, otherwise add it as a new
                # symbol.
                for base_sym in self.symbols[1:]:
                    if base_sym.name == sym.name and \
                            base_sym.bind == STB_GLOBAL:
                        merged_syms.append((sym_tab.symbols.index(sym),
                                           self.symbols.index(base_sym)))
                        break
                else:
                    self.add_symbol(sym)
                    merged_syms.append((sym_tab.symbols.index(sym),
                                       self.symbols.index(sym)))
            else:
                self.add_symbol(sym)
                merged_syms.append((sym_tab.symbols.index(sym),
                                   self.symbols.index(sym)))

        return merged_syms

    def prepare(self, offset, index, name_offset):
        """
        Prepare this ElfSection by setting its offset into
        the file, its index and name offset.
        """
        return UnpreparedElfSection.prepare(self, offset, index, name_offset,
                                            int_syms=self.symbols)


class BaseElfNote(BaseElfSection):
    """
    Represents an ELF note section (Ref 2-4). This is a subclass of
    the BaseElfSection object and all those methods are available here.
    """

    bytes_per_word = 4 # This is defined in Ref 2-4

    def __init__(self):
        self.note_name  = None
        self.note_type  = None
        self._desc_data  = None
        self._format_chr =  "I"

    def get_desc_data(self):
        """Return the note descriptpr data."""
        return self._desc_data

    def _unpack(self):
        """
        Convert our internal raw data into more useful data structures.
        """
        if len(self._data) == 0:
            return

        name_start = self.bytes_per_word * 3
        namesz, descsz, _type = \
                struct.unpack(self.endianess + self._format_chr * 3,
                              self._data[:name_start])

        name = struct.unpack(self.endianess + str(namesz) + 's',
                             self._data[name_start:name_start + namesz])

        # Remove the terminating 0.
        self.note_name = name[0].split('\x00')[:-1][0]
        self.note_type = _type

        desc_start = name_start + align_up(namesz, self.bytes_per_word)

        self._desc_data = self._data[desc_start: desc_start + descsz]

    def get_file_data(self, clean=False):
        """
        Return the file data. This overloads the parent class implementation.
        """
        self._update_data()
        return BaseElfSection.get_file_data(self, clean)

    def _update_data(self):
        """Update the note data based on our changes."""

        if not (self.note_name is None or self.note_type is None or \
                self._desc_data is None):
            # Round the data up to a multiple of word size.
            desc_data = self._desc_data

            if (len(desc_data) % self.bytes_per_word) != 0:
                desc_data += ByteArray('\0' * (len(desc_data) % self.bytes_per_word))

            data = ByteArray(struct.pack(self.endianess + self._format_chr * 3,
                                         len(self.note_name) + 1,
                                         len(desc_data),
                                         self.note_type))

            data += ByteArray(struct.pack(self.endianess +
                                          str(align_up(len(self.note_name) + 1,
                                                       self.bytes_per_word)) +
                                          's',
                                          self.note_name + '\0'))

            data += desc_data
            self._data = data


class PreparedElfNote(PreparedElfSection, BaseElfNote):

    def __init__(self, elffile, index, name_offset=None, offset=None, address=0,
                 section_type=SHT_NOTE, name=None, data=None, flags=0,
                 addralign=0, info=None, link=None, entsize=None):
        assert section_type == SHT_NOTE
        PreparedElfSection.__init__(self, elffile, index, name_offset, offset,
                                    address, section_type, name, data, flags,
                                    addralign, info, link, entsize)
        BaseElfNote.__init__(self)
        self._unpack()

    get_file_data = BaseElfNote.get_file_data


class UnpreparedElfNote(UnpreparedElfSection, BaseElfNote):

    prepares_to = PreparedElfNote

    def __init__(self, elffile, name=None, section_type=SHT_NOTE, address=0,
                 data=ByteArray('\x00'), flags=0, addralign=0,
                 info=None, link=None, entsize=None):
        assert section_type == SHT_NOTE
        UnpreparedElfSection.__init__(self, elffile, name, section_type,
                                      address, data, flags, addralign,
                                      info, link, entsize)
        BaseElfNote.__init__(self)

    get_file_data = BaseElfNote.get_file_data


class BaseElfReloc(BaseElfSection):

    def __init__(self):
        self.entsize = self._class().size()

    def _class(self):
        """ Return the struct class associated with this class. """
        return ELF_RELOC_STRUCT_CLASSES[self.wordsize]

    def _unpack(self, user_reloc_cls):
        """
        Convert our internal raw data into more useful data structures.
        """
        relocs = []
        reloc_cls = self._class()
        data = self.get_data()
        for i in range(0, len(data), self.entsize):
            reloc_struct = reloc_cls(self.endianess, self.elffile.machine)
            reloc_struct.fromdata(data[i:i+self.entsize])
            relocs.append(user_reloc_cls(reloc_struct=reloc_struct))
        self._data = None # Only unpack once
        return relocs

    def _relocs_to_int_relocs(self, relocs):
        reloc_cls = self._class()
        if relocs is None:
            relocs = []
        return [reloc.to_struct(reloc_cls, self.endianess, self.elffile.machine)
                for reloc in relocs]


class PreparedElfReloc(PreparedElfSection, BaseElfReloc):
    def __init__(self, elffile, index, name_offset=None, offset=None,
                 address=0, section_type=SHT_REL, name=None, data=None,
                 flags=0, addralign=0, info=None, link=None, entsize=None,
                 int_relocs=None):
        assert section_type == SHT_REL
        PreparedElfSection.__init__(self, elffile, index, name_offset, offset,
                                    address, section_type, name, data, flags,
                                    addralign, info, link, entsize)
        BaseElfReloc.__init__(self)

        self.relocs = int_relocs
        self.int_relocs = self._relocs_to_int_relocs(self.relocs)

    def get_reloc_structs(self):
        """
        Return a list of reloc structures. This is only available when the
        section is prepared.
        """
        return self.int_relocs

    def get_file_data(self, clean=False):
        """Return an array of bytes representing the data."""
        #return ByteArray(chain(*(reloc.todata() for reloc in self.int_relocs)))
        data = ByteArray()
        for reloc in self.int_relocs:
            data.extend(reloc.todata())
        return data

    def get_size(self):
        """Return the size of the relocation table in bytes"""
        return len(self.int_relocs) * self.entsize


class UnpreparedElfReloc(UnpreparedElfSection, BaseElfReloc):
    prepares_to = PreparedElfReloc

    def __init__(self, elffile, name=None, section_type=SHT_REL, address=0,
                 data=None, flags=0, addralign=0, info=None, link=None,
                 entsize=None):
        assert section_type == SHT_REL
        UnpreparedElfSection.__init__(self, elffile, name, section_type,
                                      address, data, flags, addralign,
                                      info, link, entsize)
        BaseElfReloc.__init__(self)
        self.relocs = self._unpack(ElfReloc)
        self.verbose = False

    def get_file_data(self, clean=False):
        """Return an array of bytes representing the data."""
        raise NotImplementedError

    def add_reloc(self, reloc):
        """
        Add a new reloc to the section. Only possible when the
        section is unprepared.
        """
        self.relocs.append(reloc)

    def prepare(self, offset, index, name_offset):
        """
        Prepare this ElfSection by setting its offset into
        the file, its index and name offset.
        """
        return UnpreparedElfSection.prepare(self, offset, index, name_offset,
                                            int_relocs=self.relocs)

    def get_size(self):
        """Return the size of the relocation table in bytes"""
        return len(self.relocs) * self.entsize

    def apply(self):
        """
        Apply the relocations in this section to the linked section.
        """
        if self.verbose:
            print "Applying relocs in section", self.name

        self.relocs = [reloc for reloc in self.relocs if not
                       reloc.apply(self.elffile, self.info, self.link)]

        return len(self.relocs) == 0

    def allocate_relocs(self):
        """
        Allocate any space or data that the relocations in this section require.
        """
        if self.verbose:
            print "Allocating relocs in section", self.name

        for reloc in self.relocs:
            reloc.allocate(self.elffile, self.info, self.link)

    def update_merged_sections(self, merged_sects):
        """
        If this relocation points at a merged section update it and then modify
        the offset of all relocations in this section to have the correct offset
        into the new section.
        """
        for old_sect, new_sect, offset in merged_sects:
            if self.info == old_sect:
                self.info = new_sect
                for reloc in self.relocs:
                    reloc.update_merged_sections(offset)

                return

    def update_discarded_symbols(self, discarded_syms):
        """
        Update symbol index's if any symbols have been removed.
        """
        if len(discarded_syms) == 0:
            return

        for reloc in self.relocs:
            reloc.update_discarded_symbols(discarded_syms)

    def update_discarded_sections(self, discarded_sects):
        """
        Delete any relocations that point to the new discarded sections.
        """
        for sect in discarded_sects:
            if self.info == sect:
                # This section will get removed when we apply the relocs and
                # its length is 0
                self.relocs = []

    def update_merged_symbols(self, sym_tab, merged_syms):
        """
        Update the relocation to use the new symbol table and update the index
        of relocations to use the new symbol ID's provided.
        """
        self.link = sym_tab

        for reloc in self.relocs:
            reloc.update_merged_symbols(merged_syms)

    def append_relocs(self, relocs):
        """
        Append relocs to the list of relocs in this relocation.
        """
        for reloc in relocs:
            self.relocs.append(reloc)

    def get_relocs(self):
        """
        Return the relocs in this relocation.
        """
        return self.relocs

    def set_verbose(self, verbose):
        self.verbose = verbose

        for reloc in self.relocs:
            reloc.set_verbose(verbose)

class BaseElfReloca(BaseElfReloc):

    def _class(self):
        """ Return the struct class associated with this class. """
        return ELF_RELOCA_STRUCT_CLASSES[self.wordsize]


class PreparedElfReloca(PreparedElfReloc, BaseElfReloca):

    def __init__(self, elffile, index, name_offset=None, offset=None,
                 address=0, section_type=SHT_RELA, name=None, data=None,
                 flags=0, addralign=0, info=None, link=None, entsize=None,
                 int_relocs=None):
        assert section_type == SHT_RELA
        PreparedElfSection.__init__(self, elffile, index, name_offset, offset,
                                    address, section_type, name, data, flags,
                                    addralign, info, link, entsize)
        BaseElfReloca.__init__(self)
        self.relocs = int_relocs
        self.int_relocs = self._relocs_to_int_relocs(self.relocs)

class UnpreparedElfReloca(UnpreparedElfReloc, BaseElfReloca):

    prepares_to = PreparedElfReloca

    def __init__(self, elffile, name=None, section_type=SHT_RELA, address=0,
                 data=None, flags=0, addralign=0, info=None, link=None,
                 entsize=None):
        assert section_type == SHT_RELA
        UnpreparedElfSection.__init__(self, elffile, name, section_type,
                                      address, data, flags, addralign,
                                      info, link, entsize)
        BaseElfReloca.__init__(self)
        self.relocs = self._unpack(ElfReloca)

ELF_SECTION_TYPES = {
    SHT_STRTAB : UnpreparedElfStringTable,
    SHT_SYMTAB : UnpreparedElfSymbolTable,
    SHT_DYNSYM : UnpreparedElfSymbolTable,
    SHT_NOTE   : UnpreparedElfNote,
    SHT_REL    : UnpreparedElfReloc,
    SHT_RELA   : UnpreparedElfReloca,
    }
