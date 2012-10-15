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

#pylint: disable-msg=R0902,R0903,R0904,R0913
# We disable the warning about a class having too many public methods
# because for this class we are sure it is a good thing. We also
# don't care about the classes that have too few methods. It would be
# nice to disable these per class.
# Also we don't really care about having too many instance attributes
"""
ELF parsing library.
"""
__revision__ = 1.0

from elf.ByteArray import ByteArray
from elf.constants import PT_PHDR, PT_LOAD, SHT_STRTAB, SHT_NOBITS, \
     ELFOSABI_NONE, EM_NONE, ET_NONE, ElfFormatError, \
     ElfMachine, EF_MIPS_ABI_O64, SHT_PROGBITS, SHF_WRITE, SHF_ALLOC
from elf.structures import InvalidArgument, ElfIdentification, \
     ELF_PH_CLASSES, ELF_SH_CLASSES, ELF_HEADER_CLASSES
from elf.File import File
from elf.util import align_up, align_down
from elf.section import ELF_SECTION_TYPES, UnpreparedElfSection, \
     UnpreparedElfStringTable, UnpreparedElfSymbolTable, PreparedElfSection, \
     UnpreparedElfReloc, UnpreparedElfReloca
from elf.segment import DataElfSegment, HeaderElfSegment, SectionedElfSegment


class BaseElfFile(object):
    """
    Class for creating and manipulating ELF files.

    Elf file classes can exist in two distinct states, prepared and unprepared.
    A prepared file has all its file offsets, name offsets and such setup and
    correct. In this state it is only possible to perform changes that won't
    affect the layout of the final ELF file.

    For example, it would be possible to change the virtual address of a
    segment, but not add new data to a segment, or add new segments.

    A file in the prepared state can be converted to raw data, and/or written
    out to a file.

    When an ELF file is created using init_and_prepare_from_filename, it is set
    in the prepared state. A new elf file created with the constructor is in
    the unprepared state.

    An unprepared ELF file can be converted into a prepared ELF file by calling
    the prepare() method.
    """

    def __init__(self):
        self._ph_offset = None
        self._ph_fixed = False

        self.machine = EM_NONE
        self.elf_type = ET_NONE
        self.entry_point = 0
        self.osabi = ELFOSABI_NONE
        self.abiversion = 0
        self.flags = 0

        # A file doesn't necessarily have these defined at all times!
        # Be careful when using unprepared elf files in particular
        self.wordsize = NotImplemented
        self.endianess = NotImplemented

    def _initialise_header_information(self, hdr):
        """Initialise information from the ELF header"""
        self.machine = hdr.e_machine
        self.elf_type = hdr.e_type
        self.entry_point = hdr.e_entry
        self.osabi = hdr.ident.ei_osabi
        self.abiversion = hdr.ident.ei_abiversion
        self.flags = hdr.e_flags

    def _get_headers(self, hdr, file_, classes, off, entsize, num):
        """Create a list of section or program headers."""
        # Create all the objects...
        h_class = classes[self.wordsize]
        headers = [h_class(self.endianess) for _ in range(num)]
        # Then populate them...
        for idx, header in enumerate(headers):
            header.fromdata(file_.get_data(off + entsize * idx, entsize), hdr)
        return headers

    def _get_section_headers(self, hdr, file_):
        """Return a list of section headers, given the elf header and file."""
        return self._get_headers(hdr, file_, ELF_SH_CLASSES, hdr.e_shoff, \
                                 hdr.e_shentsize, hdr.e_shnum)

    def _initialise_sections(self, hdr, file_, prepare):
        """Initialise information from section headers."""
        self._sh_offset = hdr.e_shoff
        self._sh_strndx = hdr.e_shstrndx
        self._sheaders = self._get_section_headers(hdr, file_)
        self.sections = []

        if self._sh_offset == 0:
            return

        # Now create section objects for all of them
        for i, header in enumerate(self._sheaders):
            sect_cls = ELF_SECTION_TYPES.get(header.sh_type,
                                             UnpreparedElfSection)
            if header.sh_type != SHT_NOBITS:
                data = file_.get_data(header.sh_offset, header.sh_size)
            else:
                data = header.sh_size
            if sect_cls in [UnpreparedElfSymbolTable, UnpreparedElfReloc,
                            UnpreparedElfReloca]:
                section = None # We handle symbol tables and relocs later
            else:
                section = sect_cls(self, None, header.sh_type, header.sh_addr,
                                   data, header.sh_flags, header.sh_addralign,
                                   header.sh_info)
                if prepare:
                    section = section.prepare(header.sh_offset, i,
                                              header.sh_name)
            self.sections.append(section)

        # Now set up any links between sections and convert the info
        # field to the appropriate type.
        for section, header in zip(self.sections, self._sheaders):
            if section:
                if header.sh_link:
                    section.link = self.sections[header.sh_link]
                if section.info_is_section():
                    section.info = self.sections[header.sh_info]
                else:
                    section.info = header.sh_info

    def _get_strtable(self):
        """
        Return the string table for this file. Returns None if the table does
        not exist.
        """
        strtable = None
        if self._sh_strndx:
            strtable = self.sections[self._sh_strndx]
            if strtable.type != SHT_STRTAB:
                raise ElfFormatError, \
                      "The section string table is malformed. %s %s" \
                      % (strtable, self.sections)
        return strtable

    def _initialise_section_names(self):
        """
        Use this file's string table to assign to assign names to all of its
        sections.
        """
        strtable = self._get_strtable()
        if strtable:
            for section, header in zip(self.sections, self._sheaders):
                section._name = strtable.get_string_ofs(header.sh_name)

    def _init_cls_sections(self, file_, prepare, classes):
        """
        Initialise the sections which are of a given list of classes
        """
        for i, header in enumerate(self._sheaders):
            sect_cls = ELF_SECTION_TYPES.get(header.sh_type,
                                             UnpreparedElfSection)
            if sect_cls in classes:
                data = file_.get_data(header.sh_offset, header.sh_size)
                strtable = self._get_strtable()
                name = strtable.get_string_ofs(header.sh_name)
                sect = sect_cls(self, name, header.sh_type, header.sh_addr,
                                data, header.sh_flags, header.sh_addralign,
                                header.sh_info, self.sections[header.sh_link])
                if prepare:
                    sect = sect.prepare(header.sh_offset, i, header.sh_name)
                self.sections[i] = sect

        # Now set up any links between sections and convert the info
        # field to the appropriate type.
        for section, header in zip(self.sections, self._sheaders):
            if section:
                if header.sh_link:
                    section.link = self.sections[header.sh_link]
                if section.info_is_section():
                    section.info = self.sections[header.sh_info]
                else:
                    section.info = header.sh_info

    def _initialise_symbol_tables(self, file_, prepare):
        """Initialise symbol information."""
        self._init_cls_sections(file_, prepare, [UnpreparedElfSymbolTable])

    def _initialise_relocs(self, file_, prepare):
        """Initialise reloc information."""
        self._init_cls_sections(file_, prepare, [UnpreparedElfReloc,
                                                 UnpreparedElfReloca])

    def _initialise_segments(self, hdr, file_, prepare):
        """Initialise instance from program headers."""
        # Now load all the program headers
        #
        # "The time has come," the developer said, "to talk of many
        # things: Of section to segment mapping and scatter load, of
        # cabbages and kings, and why the RVCT linker is on drugs and
        # whether pigs have wings."
        #
        # There are two ways of determining which sections belong in a
        # segment:
        #
        # 1) In most cases a section is in a segment if its virtual
        #    address range falls in the virtual address range of the
        #    segment.  These segments tend to contain multiple
        #    PROGBITS sections and may end with a NOBITS section.
        #
        # 2) In the case of segments build with RVCT's scatter load
        #    support the virtual address of the included sections can
        #    be anywhere in memory.  In this case a section is in the
        #    segment if its data file offset falls in the offset range
        #    of the segment.  These segments can contain multiple
        #    PROGBITS and NOBITS sections in any order.  The file size
        #    of the segment is the filesize of all of the PROGBITS
        #    sections.  The memory size of the segment is the sum of
        #    the size of all of the sections, not the difference
        #    between the lowest and highest virtual addresses.
        #
        # Some other things about ELF files that you need to know:
        #
        # a) Segments can overlap, and sections can appear in multiple
        #    segments.
        #
        # b) Some sections do not appear in any segments.  These tend
        #    to be the symbol table and other debugging data.
        #
        # c) The offsets of NOBITS sections tend to be the same as the
        #    offset for the next PROGBITS section, even if it is in
        #    the next segment.  However, sometimes NOBITS sections
        #    appear at the beginning of a segment.
        #
        # d) The address offset from the start of a segment and the
        #    file offset from the start of the segment are not
        #    necessarily the same.
        #
        # Therefore, to map sections to segments involves two passes:
        #
        # 1) Find all of the sections that fall in the segment virtual
        #    address range.
        #
        # 2) Find all of the remaining sections that fall in the file
        #    offset range.  If any are found then mark the segment as
        #    a scatter-load segment.
        #
        # The file size and memory size of a segment depends on
        # whether or not it is a scatter load segment.  For scatter
        # load files, the section that has the latest offset from the
        # beginning of the section is used, while for non-scatter-load
        # segments, the section with the largest virtual address is
        # used.
        #
        # File offsets cannot be used for both types of segments
        # because broken linkers can produce sections with
        # inconsistant virtual address to file offset mappings and
        # this will make elfweaver produce segments with incorrect
        # file sizes or memory sizes.
        #
        # "O Readers," said the developer, "You've had a pleasant run!
        # Shall we start the code again?" But answer came there none--
        # And this was scarcely odd, because no-one would read this far
        # for fun.

        self._ph_offset = hdr.e_phoff
        self.segments = []

        # Short circuit the return.
        if hdr.e_phoff == 0:
            return

        ph_list = self._get_headers(hdr, file_, ELF_PH_CLASSES, hdr.e_phoff,
                                    hdr.e_phentsize, hdr.e_phnum)

        # Map segments to sections, pass 1.
        for ph in ph_list:
            if ph.p_type != PT_PHDR:
                low = ph.p_vaddr
                high = low + ph.p_memsz
                sects = []
                for section in self.sections:
                    # See if the section's virtual address range falls within
                    # the virtual address range of the segment.
                    addr = section.address
                    if low <= addr < high and addr + section.get_size() <= high:
                        sects.append(section)

                if sects:
                    seg = SectionedElfSegment(self, prog_header=ph,
                                              sections=sects)
                else:
                    data = file_.get_data(ph.p_offset, ph.p_filesz)
                    seg = DataElfSegment(self, prog_header=ph, data=data,
                                         memsz=ph.p_memsz)
            else:
                seg = HeaderElfSegment(self, prog_header=ph)
            self.segments.append(seg)

        # Map segments to sections, pass 2
        sheaders = self._get_section_headers(hdr, file_)
        sect_offsets = [header.sh_offset for header in sheaders]
        for ph, seg in zip(ph_list, self.segments):
            if seg.has_sections():
                low = ph.p_offset
                high = low + ph.p_filesz
                for (section, offset) in zip(self.sections, sect_offsets):
                    # Scatter-load sections do not appear in multiple segments,
                    # so skip sections that are already in some segment.
                    if section.in_segment:
                        continue
                    # See if the section's file offset range falls within the
                    # file offset range of the segment.  Remember that the
                    # trailing BSS section can have the offset of the next
                    # section
                    if low <= offset <= high and \
                           ((section.type == SHT_NOBITS and offset != low) or \
                            (section.type != SHT_NOBITS and section.get_size() > 0 and \
                             offset + section.get_size() <= high)):
                        seg.add_section(section)
                        seg.set_scatter_load()

                # If we're a scatter-load segment we need to work out in segment
                # offsets for our sections
                if seg.is_scatter_load():
                    for section in seg.sections:
                        idx = self.sections.index(section)
                        offset = sect_offsets[idx]
                        section.set_in_segment_offset(offset - ph.p_offset)

        # If need be, we now prepare the segments
        if prepare:
            for ph, seg in zip(ph_list, self.segments):
                if ph.p_type == PT_PHDR:
                    prog_header_size = hdr.e_phentsize * hdr.e_phnum
                else:
                    prog_header_size = None
                seg.prepare(ph.p_offset, prog_header_size)

    def init_and_prepare_from_file(self, file_):
        """This method initialise an ElfFile from a given File object."""
        ident = ElfIdentification()
        ident.fromdata(file_.get_data(0, ident.get_size()))
        if file_.size() < ident.get_size():
            self.wordsize = 0
            self.sections = []
            self.segments = []
            return
        self.wordsize = ident.wordsize
        self.endianess = ident.endianess

        header_data = file_.get_data(0, ELF_HEADER_CLASSES[self.wordsize].size())
        hdr = ELF_HEADER_CLASSES[self.wordsize]()
        hdr.fromdata(header_data)

        self._initialise_header_information(hdr)
        self._initialise_sections(hdr, file_)
        self._initialise_symbol_tables(file_)
        self._initialise_relocs(file_)
        self._initialise_section_names()
        self._initialise_segments(hdr, file_)

        # FIXME: Mothra bug2591
        #  This code removes any zero-sized sections.
        #  Perhaps not the right place to do it, but works around linkder
        #  scripts that generate zero-sized sections. Note this may only
        #  be a problem due to stupid scatter-load handling.
        self.sections = [s for s in self.sections if s.get_size() or not s.name]
        for segment in self.segments:
            if segment.has_sections():
                segment.sections = [s for s in segment.sections if
                                    s.get_size() or not s.name]

    def init_and_prepare_from_filename(self, filename):
        """
        This method returns a new Elf file object initialised from a file.
        """
        return self.init_and_prepare_from_file(File(filename, "rb"))

    def get_ph_offset(self):
        """Get the program header offset."""
        return self._ph_offset

    def has_segments(self):
        """Return true if this ELF file has segments."""
        return bool(self.segments)

    def get_paddr(self):
        """Return the physical address of the first loadable segment"""
        for seg in self.segments:
            if seg.type == PT_LOAD:
                return seg.paddr
        raise InvalidArgument, "Elf file has no segments of type PT_LOAD?"

    def add_symbols(self, symbols):
        """
        Add a list of symbols to this files symbol table. If the symbol table
        doesn't exit yet, it will be created.

        symbols: a list of ElfSymbol objects
        """
        symtab = self.get_symbol_table()
        if symtab is None:
            # we need to create a symbol table
            string_table = UnpreparedElfStringTable(self, ".strtab")
            symtab = UnpreparedElfSymbolTable(self, ".symtab",
                                              link=string_table)
            self.sections.append(string_table)
            self.sections.append(symtab)

        for sym in symbols:
            symtab.add_symbol(sym)

    def get_symbol_table(self):
        """ Return the main symbol table for this file. """
        return self.find_section_named(".symtab")

    def get_symbols(self):
        """ Return a list of symbols (ElfSymbol) in this file. """
        symtab = self.get_symbol_table()
        if symtab:
            return symtab.symbols
        else:
            return []

    def section_symbols(self, section):
        """Find the symbols for the given section"""
        return [sym for sym in self.get_symbols() if sym.section == section]

    def find_symbol(self, name):
        """
        Find the named symbol in the file. Returns None if it can't be found.
        Only returns the first one found.
        """
        for symbol in self.get_symbols():
            if symbol.name == name:
                return symbol

        for symbol in self.get_symbols():
            if symbol.name is not None and symbol.name.endswith(name):
                return symbol

        return None

    def get_value(self, address, size, endianess = None):
        """get a value from an elf file."""
        if self.machine == ElfMachine(8):
            if self.flags & EF_MIPS_ABI_O64:
                if address & 0x80000000:
                    address |= 0xffffffff00000000L

        for segment in self.segments:
            if not segment.has_sections():
                continue
            for section in segment.get_sections():
                if address >= section.address and \
                       address < (section.address + section.get_size()):
                    offset = address - section.address
                    if endianess is None:
                        endianess = self.endianess
                    return section.get_data().get_data(offset, size, endianess)

        return None

    def find_section_named(self, name):
        """Find a section named `name'. Return None if no section is found."""
        return dict([(sect.name, sect) for sect in self.sections]).get(name)

    def remove_nobits(self):
        """
        Remove any NOBITS sections from the ELF file by converting them to
        PROGBITS sections. This will ensure that segments have memsz and filesz
        equal.
        """
        if self.has_segments():
            for segment in self.segments:
                segment.remove_nobits()
        else:
            for section in self.sections:
                section.remove_nobits()

    def _addr_type_is_virt(cls, addr_type):
        """
        Parse the address space type, returning true if virtual, false if
        physical. May raise InvalidArgument.
        """
        try:
            return {"virtual": True, "physical": False}[addr_type]
        except KeyError:
            raise InvalidArgument("addr_type must be 'virtual' or 'physical'")
    _addr_type_is_virt = classmethod(_addr_type_is_virt)

    def _get_ptload_segments(self):
        """
        Return a list of those segments in the file which are of type PT_LOAD.
        """
        return [seg for seg in self.segments if seg.type == PT_LOAD]

    def get_first_addr(self, addr_type = "virtual"):
        """
        Return the first memory address used in the file. By default it uses
        the virtual memory address. This can be changed to use the physical
        memory address if physical is set to True. Getting last physical can only
        be used if the file has segments.
        """
        virtual = self._addr_type_is_virt(addr_type)

        if self.has_segments():
            segments = self._get_ptload_segments()
            if virtual:
                addrs = [segment.vaddr for segment in segments]
            else:
                addrs = [segment.paddr for segment in segments]
        else:
            # Base it on sections instead
            if not virtual:
                raise InvalidArgument, "Can't find first physical \
                address in an ElfFile without segments."
            addrs = [section.address for section in self.sections]
        if addrs:
            return min(addrs)
        else:
            return 0

    def get_last_addr(self, addr_type = "virtual"):
        """
        Return the last memory address used in the file. By default it uses the
        virtual memory address. This can be changed to use the physical memory
        address if physical is set to True. Getting last physical can only be
        used if the file has segments.
        """
        virtual = self._addr_type_is_virt(addr_type)

        if self.has_segments():
            segments = self._get_ptload_segments()
            if virtual:
                addrs = [(segment.vaddr + segment.get_memsz(), segment.align)
                         for segment in segments]
            else:
                addrs = [(segment.paddr + segment.get_memsz(), segment.align)
                         for segment in segments]
        else:
            # Base it on sections instead
            if not virtual:
                raise InvalidArgument, "Can't find last physical \
                address in an ElfFile without segments."
            addrs = [(section.address + section.get_size(), section.addralign)
                     for section in self.sections]
        if addrs:
            last_addr, last_align = max(addrs)
        else:
            last_addr, last_align = 0, 0
        return align_up(last_addr, last_align)

    def get_address_span(self, addr_type = "virtual"):
        """Return the address span of a given file."""
        return self.get_last_addr(addr_type) - self.get_first_addr(addr_type)

    def find_segment_by_vaddr(self, vaddr):
        """Given a virtual address return the segment in which it is found."""
        for segment in self.segments:
            if vaddr in segment.get_span():
                return segment


class PreparedElfFile(BaseElfFile):
    """
    Prepared elf files have a set layout, and cannot be added to/removed from.
    They have all the details of their layout fixed and well defined, ready to
    be written to disk.
    """

    def __init__(self, wordsize=None, endianess=None, sections=None,
                 segments=None, filename=None):
        """Create a new ELF file."""
        BaseElfFile.__init__(self)

        if filename is not None:
            self.init_and_prepare_from_filename(filename)
        else:
            if sections is None:
                sections = [PreparedElfSection(self)]
            if segments is None:
                segments = []
            self.wordsize = wordsize
            self.endianess = endianess
            self.sections = sections
            self.segments = segments

    def get_elf_header(self):
        """Return the ELF header for this ElfFile."""

        elfheader = ELF_HEADER_CLASSES[self.wordsize](self.endianess)

        elfheader.e_machine = self.machine
        elfheader.e_type = self.elf_type
        elfheader.e_entry = self.entry_point
        elfheader.ident.ei_osabi = self.osabi
        elfheader.ident.ei_abiversion = self.abiversion
        elfheader.e_flags = self.flags

        # Create any section headers
        if self.sections:
            elfheader.e_shentsize = ELF_SH_CLASSES[self.wordsize].size()
            elfheader.e_shoff = self._sh_offset
            elfheader.e_shnum = len(self.sections)
            elfheader.e_shstrndx = self._sh_strndx

        if self.segments:
            elfheader.e_phentsize = ELF_PH_CLASSES[self.wordsize].size()
            elfheader.e_phoff = self._ph_offset
            elfheader.e_phnum = len(self.segments)

        return elfheader

    def get_section_headers(self):
        """Return a list of section headers"""
        return [sect.get_section_header() for sect in self.sections]

    def todata(self):
        """Return a list of (array, offset) tuples"""
        data = [(0, self.get_elf_header().todata())]

        # Create any section headers
        if self.sections[1:]:
            sect_data = ByteArray()
            for header in self.get_section_headers():
                sect_data.extend(header.todata())
            data.append( (self._sh_offset, sect_data) )

        # Create any section headers
        if self.segments:
            ph_data = ByteArray()
            for segment in self.segments:
                ph_data.extend(
                    segment.get_program_header(self.endianess,
                                               self.wordsize).todata())
            data.append( (self._ph_offset, ph_data) )

        if self.sections[1:]:
            data += [(sec.offset, sec.get_file_data()) for sec in self.sections]
        elif self.segments:
            data += [(seg.offset, seg.get_file_data()) for seg in self.segments]

        return data

    def to_file(self, file_):
        """Write the ELF out to a real file. file_ must be a File() instance."""
        data = self.todata()
        for offset, _dat in data:
            file_.seek(offset)
            _dat.tofile(file_)
        file_.close()

    def to_filename(self, filename):
        """This method outputs the ELf file to a given filename."""
        self.to_file(File(filename, "wb"))

    # We override the methods below to call the base method, passing
    # in prepare=True, so indicate that we want to get prepared objects
    # out of them.

    def _initialise_sections(self, hdr, file_):
        BaseElfFile._initialise_sections(self, hdr, file_, True)

    def _initialise_segments(self, hdr, file_):
        BaseElfFile._initialise_segments(self, hdr, file_, True)

    def _initialise_symbol_tables(self, file_):
        BaseElfFile._initialise_symbol_tables(self, file_, True)

    def _initialise_relocs(self, file_):
        BaseElfFile._initialise_relocs(self, file_, True)


class UnpreparedElfFile(BaseElfFile):
    """
    Unprepared elf files can have their contents modified however properties
    such as section header offsets, etc are not necessarily defined.
    """

    def __init__(self, filename=None):
        BaseElfFile.__init__(self)
        if filename is not None:
            self.init_and_prepare_from_filename(filename)
        else:
            self.sections = [UnpreparedElfSection(self)]
            self.segments = []
            self._sh_offset = None
            self._sh_strndx = 0

        # Hold a list of the symbols in the got table
        self.got_table = []

    def set_ph_offset(self, offset, fixed=False):
        """Set the program header offset."""

        if offset is not None:
            min_offset = min([header_class.size() for header_class in \
                              ELF_HEADER_CLASSES.values()])
            if offset < min_offset:
                raise InvalidArgument, \
                      "Program header offset must be at least %d (%d)." % \
                      (min_offset, offset)

        if not self._ph_fixed:
            self._ph_offset = offset
            self._ph_fixed = fixed

    def add_segment(self, segment):
        """Add a new segment to the ELF file."""
        self.segments.append(segment)

    def add_section(self, section):
        """Adding a new section to the ELF file."""
        self.sections.append(section)

    def clone_section(self, section, address):
        """
        Returns a new section based on the provided section.  This avoids a
        circular dependency in the elfweaver linker's linker script code.
        """
        return UnpreparedElfSection(self, section.name, section.type,
                                    address, None, section.flags,
                                    section.addralign, section.info,
                                    section.link, section.entsize)

    def allocate_section(self, sect_name):
        """
        Returns the section requested, creating a new one if required.
        """
        sect = self.find_section_named(sect_name)
        if sect == None:
            if sect_name.find('bss') != -1:
                sect = UnpreparedElfSection(self, sect_name, SHT_NOBITS, 0, 0,
                                            SHF_WRITE | SHF_ALLOC, 8)
            elif sect_name == '.got':
                sect = UnpreparedElfSection(self, sect_name, SHT_PROGBITS, 0,
                                            ByteArray(), SHF_WRITE | SHF_ALLOC,
                                            4, entsize=4)
                got_plt = UnpreparedElfSection(self, '.got.plt', SHT_PROGBITS,
                                               0, ByteArray('\0' * 4 * 3),
                                               SHF_WRITE | SHF_ALLOC, 4,
                                               entsize=4)
                self.add_section(got_plt)
            else:
                sect = UnpreparedElfSection(self, sect_name, SHT_PROGBITS, 0,
                                            ByteArray(), SHF_WRITE | SHF_ALLOC,
                                            4, entsize=4)

            self.add_section(sect)

        return sect

    def add_got_entry(self, sym):
        """Add sym to the got table."""
        if not sym in self.got_table:
            got = self.allocate_section('.got')

            # Allocate this symbol at the end of the current .got
            offset = got.get_size()
            got.append_word(0)

            # Now set the got value on the symbol so that it is able to update
            # the .got section when we know the actual address it refers to
            sym.got_offset = offset

            self.got_table.append(sym)

    def remove_section(self, section):
        """Remove section"""
        if section not in self.sections:
            raise InvalidArgument, "Can't remove a section that doesn't exist"
        for segment in self.segments:
            if segment.has_sections() and section in segment.sections:
                segment.remove_section(section)
        self.sections.remove(section)

    def replace_section(self, old_section, new_section):
        """Replace section"""
        if old_section not in self.sections:
            raise InvalidArgument, "Can't remove a section that doesn't exist"
        for segment in self.segments:
            if segment.has_sections() and old_section in segment.sections:
                segment.replace_section(old_section, new_section)
        self.sections[self.sections.index(old_section)] = new_section

    def create_and_replace(self, cls, name):
        """
        This method creates a new section of a given class with a given name.
        If another section of the same name exists, it is replaced by the new
        section, otherwise, the new section is appended to the file.
        """
        old_sect = self.find_section_named(name)
        new_sect = cls(self, name)
        if old_sect:
            self.replace_section(old_sect, new_sect)
        else:
            self.add_section(new_sect)
        return new_sect

    def prepare(self, wordsize, endianess):
        """Prepare file for writing"""
        self.wordsize = wordsize
        self.endianess = endianess
        # First we prepare the symbols, as this might in fact add
        # new sections.
        sh_string_table = None

        # Sort the sections so they appear as you'd expect from ld
        old_sections = self.sections[1:]
        if len(self.sections) > 1:
            self.sections = [self.sections[0]]
        else:
            self.sections = []
        for sect in old_sections:
            if sect.address > 0:
                self.sections.append(sect)

        old_sections = [sect for sect in old_sections if sect.address == 0]
        old_sections.sort(key = lambda x: x.name)
        self.sections.sort(key = lambda x: x.address)
        self.sections.extend(old_sections)

        if len(self.sections) > 1:
            # Create section header string table
            sh_string_table = self.create_and_replace(UnpreparedElfStringTable,
                                                      ".shstrtab")

        # We need to make sure the symbol table is the last thing in the file,
        symtab = self.find_section_named(".symtab")
        if symtab:
            i = self.sections.index(symtab)
            self.sections[-1], self.sections[i] = symtab, self.sections[-1]

        # First find where our segments should go.
        if self._ph_offset is None or self._ph_offset == 0:
            self.set_ph_offset(ELF_HEADER_CLASSES[wordsize].size())

        ofs = self._ph_offset
        ###########
        sections = self.sections[:]
        if self.segments:
            ofs = self._prepare_segments(ofs, sh_string_table, sections)

        ofs, sections = self._prepare_sections(ofs, sh_string_table, sections)

        new_file = PreparedElfFile(wordsize, endianess, sections, self.segments)
        new_file._sh_offset = align_up(ofs, new_file.wordsize / 8)
        new_file._sh_strndx = self._sh_strndx
        new_file._ph_offset = self._ph_offset

        new_file.machine = self.machine
        new_file.elf_type = self.elf_type
        new_file.entry_point = self.entry_point
        new_file.osabi = self.osabi
        new_file.abiversion = self.abiversion
        new_file.flags = self.flags

        for section in new_file.sections:
            section.elffile = new_file

        return new_file

    def _prepare_sections(self, ofs, sh_string_table, sections):
        """
        Prepare sections in the file. Passed the current ofs value, and returns
        any updated value.
        """
        # We now add all the section that are not part of the segment.
        if sh_string_table:
            self._sh_strndx = self.sections.index(sh_string_table)

            # we need to add all the section names to the sh_string_table before
            # we go prepring things, since once sh_string_table is prepared, we
            # can't add to it any more.
            for section in sections:
                if isinstance(section, UnpreparedElfSection):
                    sh_string_table.add_string(section.name)

        new_sections = []
        for idx, section in enumerate(sections):
            if isinstance(section, UnpreparedElfSection) and section.name:
                if sh_string_table:
                    name_offset = sh_string_table.offset_of(section.name)
                else:
                    name_offset = None
                offset = align_up(ofs, section.addralign)
                # Note: must prepare before getting size
                section = section.prepare(offset, idx, name_offset)
                ofs = offset + section.get_size()
            elif isinstance(section, UnpreparedElfSection):
                # NULL sections
                section = section.prepare(0, 0, 0)

            new_sections.append(section)
        # Check that all sections have a valid section index
        for section in new_sections[1:]:
            assert section.index < len(sections)

        return ofs, new_sections

    def _prepare_segments(self, ofs, sh_string_table, all_sections):
        """Prepare the segments in the file for writing."""

        ph_class = ELF_PH_CLASSES[self.wordsize]
        ph_header_size = ph_class.size() * len(self.segments)
        ofs += ph_header_size
        for segment in self.segments:
            #round down
            new_ofs = align_down(ofs, segment.align)
            if segment.align != 0:
                new_ofs += segment.vaddr % segment.align
            # add extra
            if new_ofs <= ofs:
                new_ofs += segment.align

            assert new_ofs >= ofs, "New offset must be greater " \
                   "than the old offset"
            # check + add pagesize
            ofs = new_ofs
            if segment.align != 0:
                assert ofs % segment.align == segment.vaddr % segment.align, \
                       "Must be congruent"

            if segment.type == PT_PHDR:
                segment.prepare(ofs, ph_header_size)
            else:
                segment.prepare(ofs)
                # Now can prepare the sections inside it if it has any
                if segment.has_sections():
                    segment.prepare_sections(all_sections, sh_string_table)
            ofs += segment.get_filesz()

        return ofs

    def _initialise_sections(self, hdr, file_):
        BaseElfFile._initialise_sections(self, hdr, file_, False)

    def _initialise_segments(self, hdr, file_):
        BaseElfFile._initialise_segments(self, hdr, file_, False)

    def _initialise_symbol_tables(self, file_):
        BaseElfFile._initialise_symbol_tables(self, file_, False)

    def _initialise_relocs(self, file_):
        BaseElfFile._initialise_relocs(self, file_, False)

    def init_and_prepare_from_file(self, file_):
        """This method initialise an ElfFile from a given File object."""
        BaseElfFile.init_and_prepare_from_file(self, file_)

    def init_and_prepare_from_filename(self, filename):
        """
        This method returns a new Elf file object initialised from a file.
        """
        return self.init_and_prepare_from_file(File(filename, "rb"))
