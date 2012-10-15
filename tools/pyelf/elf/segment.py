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

#pylint: disable-msg=R0902,R0913,W0142
# We dont care about too many arguments, we need them all (0913)
# Also we don't really care about having too many instance attributes (0902)
# We also don't mind using **magic in the one place it's used. (0142)
"""
ELF parsing library.
"""
__revision__ = 1.0

from elf.ByteArray import ByteArray
from elf.constants import PT_NULL, PT_PHDR, SHT_NOBITS
from elf.structures import InvalidArgument, ELF_PH_CLASSES
from elf.util import Span, Unprepared, Prepared

class ElfSegment(object):
    """
    ElfSegment instances represent segments in an ELF file. This differs from
    ElfProgramHeader, which just describes the header and not the data itself.
    """

    def __init__(self, elffile, program_header_type=PT_NULL, vaddr=0, paddr=0,
                 flags=0, align=0, prog_header=None):
        """Create a new ElfSegment, with a given set of initial parameters."""
        self.elffile = elffile
        if prog_header:
            program_header_type = prog_header.p_type
            vaddr = prog_header.p_vaddr
            paddr = prog_header.p_paddr
            flags = prog_header.p_flags
            align = prog_header.p_align
        self.type = program_header_type
        self.vaddr = vaddr
        self.paddr = paddr
        self.flags = flags
        self.align = align

        if self.vaddr is None:
            self.vaddr = 0
        if self.paddr is None:
            self.paddr = 0

        self.prepared = False
        self._offset = None

        self._prog_header_size = None # This is only used by HeaderElfSements

    def remove_nobits(self):
        """Remove any NOBITS sections from the segment."""
        raise NotImplementedError

    def prepare(self, offset, prog_header_size = None):
        """Prepare this segment ready for writing."""
        if self.prepared:
            raise Prepared, "This segment is already prepared"
        if not prog_header_size and self.type == PT_PHDR:
            raise InvalidArgument, "Must set the program header size on PHDR "\
                  "segments when preparing"
        if prog_header_size and self.type != PT_PHDR:
            raise InvalidArgument, "Program header size should only " \
                  "be set on phdr sections."

        self._offset = offset
        self.prepared = True
        self._prog_header_size = prog_header_size

    def get_memsz(self):
        """Return the size this segment occupies in memory."""
        raise NotImplementedError

    def get_filesz(self):
        """Return the size this segment occupies in memory."""
        raise NotImplementedError

    def get_offset(self):
        """Return the offset of this segment is the physical file."""
        if self.prepared:
            return self._offset
        else:
            raise Unprepared, "Can only get the offset once the" \
                  " segment has been prepared."
    offset = property(get_offset)

    def has_sections(self):
        """Return true if the segment has internal sections."""
        raise NotImplementedError

    def copy_into(self, elffile):
        """Copy this segment into the given elf file."""
        raise NotImplementedError

    def _do_copy_into(self, elffile, **kwargs):
        """
        Implements the guts of the copy_into process, once the kwargs for the
        particular class have been collected.
        """
        new_segment = self.__class__(elffile, program_header_type=self.type,
                                     vaddr=self.vaddr,
                                     paddr=self.paddr,
                                     flags=self.flags,
                                     align=self.align, **kwargs)
        new_segment._prog_header_size = self._prog_header_size
        new_segment._offset = self._offset
        new_segment.prepared = self.prepared
        return new_segment

    def contains_vaddr(self, vaddr):
        """Return true if this segment contains the given virtual address."""
        return vaddr in self.get_span()

    def get_span(self):
        """Return the span of memory that this segment holds."""
        return Span(self.vaddr, self.vaddr + self.get_memsz())

    def vtop(self, vaddr):
        """Convert a virtual address to a physical address"""
        if vaddr not in self.get_span():
            raise InvalidArgument, "Vaddr must be in segment's range"
        return self.paddr + (vaddr - self.vaddr)

    def get_program_header(self, endianess, wordsize):
        """
        Return a program header for this segment with a given endianess and
        wordsize.
        """
        if not self.prepared:
            raise Unprepared, "get_program_header can't be called if the " \
                  " segment is unprepared."
        try:
            prog_header = ELF_PH_CLASSES[wordsize](endianess)
        except KeyError:
            raise InvalidArgument, "wordsize %s is not valid. " \
                  "Only %s are valid" %  \
                  (wordsize, ELF_PH_CLASSES.keys())

        prog_header.p_type = self.type
        prog_header.p_vaddr = self.vaddr
        prog_header.p_paddr = self.paddr
        prog_header.p_flags = self.flags
        prog_header.p_align = self.align
        prog_header.p_offset = self.offset
        prog_header.p_memsz = self.get_memsz()
        prog_header.p_filesz = self.get_filesz()

        return prog_header

    def __str__(self):
        return "<ElfSegment %s>" % self.type


class SectionedElfSegment(ElfSegment):
    """
    A SectionElfSegment is a segment which contains sections, but no explicit
    data.
    """

    def __init__(self, elffile, program_header_type=PT_NULL, vaddr=0, paddr=0,
                 flags=0, align=0, prog_header=None, sections=None):
        """
        Create a new SectionedElfSegment, with a given set of initial
        parameters.
        """
        ElfSegment.__init__(self, elffile, program_header_type, vaddr, paddr,
                            flags, align, prog_header)

        self.sections = []
        if sections:
            for section in sections:
                self.add_section(section)

        self._is_scatter_load = False

    def set_scatter_load(self):
        """Mark the segment as a scatter-load segment."""
        self._is_scatter_load = True

    def is_scatter_load(self):
        """Return whether or not the segment is a scatter-load one."""
        return self._is_scatter_load

    def copy_into(self, elffile):
        """Copy this segment into the given elf file."""

        sections = []

        for sect in self.sections:
            if sect.link is None:
                sections.append(sect.copy_into(elffile))

        for sect in sections:
            elffile.sections.append(sect)

        new_segment = self._do_copy_into(elffile, sections=sections)
        new_segment._is_scatter_load = self._is_scatter_load
        return new_segment

    def has_sections(self):
        """Return true if the segment has internal sections."""
        return True

    def get_sections(self):
        """Return a list of sections associated with this segment."""
        return self.sections

    def remove_section(self, section):
        """
        Remove a section from the segment.

        Raises InvalidArgument if the section is not valid.
        """
        if section not in self.sections:
            raise InvalidArgument, "Section must be in segment to remove it."
        self.sections.remove(section)
        section.in_segment = False
        # FIXME: needs to undo what got done in add_section

    def replace_section(self, old_section, new_section):
        """
        Replace old_section with new_section in this segments list of sections.

        Raises InvalidArgument if the old_section is not valid.
        """
        if old_section not in self.sections:
            raise InvalidArgument, "Section must be in segment to replace it."
        self.remove_section(old_section)
        self.add_section(new_section)

    def add_section(self, section):
        """Add a new section to a segment."""
        # Mark the section as added to the segment.
        # HACK:  Should detect if adding a section causes a segment to
        # turn into a scatter load segment.
        section.in_segment = True
        self.sections.append(section)
        self.sections.sort(key=lambda x: x.address)

    def get_filesz(self):
        """Return the size this segment occupies in memory."""
        return self._calc_section_size()

    def _calc_section_size(self):
        """Calculate the size of the segment when it has sections."""
        # NOBITS sections are not included in the calculations.
        sects = [sect for sect in self.sections if sect.type != SHT_NOBITS]
        return self._calc_size(sects)

    def get_memsz(self):
        """Return the size this segment occupies in memory."""
        return self._calc_size(self.sections)

    def _calc_size(self, sects):
        """
        Return the size of memory occupied by the given list of sections.
        """
        if not sects:
            return 0
        off, size = max([(sect.calc_in_segment_offset(self), sect.get_size())
                         for sect in sects])
        return off + size

    def remove_nobits(self):
        """Remove any NOBITS sections from the segment."""
        for section in self.sections:
            section.remove_nobits()

    def prepare_sections(self, all_sections, sh_string_table):
        """
        This prepares all sections within this segment, given the context of
        all_sections, which are all the sections in the elf file and
        sh_string_table, which is the string table containing the section names.
        """
        for i, section in enumerate(self.sections):
            if section not in all_sections:
                section = section.prepared_to
            else:
                name_offset = sh_string_table.add_string(section.name)
                sh_index = all_sections.index(section)
                offset = self.offset + section.calc_in_segment_offset(self)
                section = section.prepare(offset, sh_index, name_offset)
                all_sections[sh_index] = section

            self.sections[i] = section


class DataElfSegment(ElfSegment):
    """
    A DataElfSegment is a segment which contains data, but no sections.
    """

    def __init__(self, elffile, data, program_header_type=PT_NULL, vaddr=0,
                 paddr=0, flags=0, align=0, prog_header=None, memsz=None):
        """
        Create a new DataElfSegment, with a given set of initial parameters.
        """
        ElfSegment.__init__(self, elffile, program_header_type, vaddr, paddr,
                            flags, align, prog_header)
        self._data = data
        if memsz is not None:
            self._memsz = memsz
        else:
            self._memsz = len(data)

    def copy_into(self, elffile):
        """Copy this segment into the given elf file."""
        # Make a copy of the data
        data = self._data.copy()
        return self._do_copy_into(elffile, data=data, memsz=self._memsz)

    def has_sections(self):
        """Return true if the segment has internal sections."""
        return False

    def get_file_data(self):
        """Return the data that will go into the file."""
        return self._data

    def get_memsz(self):
        """Return the size this segment occupies in memory."""
        return self._memsz

    def get_filesz(self):
        """Return the size this segment occupies in memory."""
        return len(self._data)

    def remove_nobits(self):
        """Remove any NOBITS sections from the segment."""
        zeros = ByteArray('\0' * (self.get_memsz() - self.get_filesz()))
        self._data.extend(zeros)


class HeaderElfSegment(ElfSegment):
    """
    This segment represents the program header itself.
    """

    def __init__(self, elffile, vaddr=0, paddr=0, flags=0, align=0,
                 prog_header=None, program_header_type=PT_PHDR):
        """
        Create a new HeaderElfSegment, with a given set of initial parameters.
        """
        assert program_header_type == PT_PHDR
        ElfSegment.__init__(self, elffile, PT_PHDR, vaddr, paddr, flags,
                            align, prog_header)

    def copy_into(self, elffile):
        """Copy this segment into the given elf file."""
        return self._do_copy_into(elffile)

    def get_memsz(self):
        """Return the size this segment occupies in memory."""
        if not self.prepared:
            raise Unprepared, "Phdr segments must be prepared before " \
                  "it is possible to get their size."
        return self._prog_header_size

    def get_filesz(self):
        """Return the size this segment occupies in memory."""
        if not self.prepared:
            raise Unprepared, "Phdr segments must be prepared before " \
                  "it is possible to get their size."
        return self._prog_header_size

    def has_sections(self):
        """Return true if the segment has internal sections."""
        return False

    def remove_nobits(self):
        """Remove any NOBITS sections from the segment."""
        pass
