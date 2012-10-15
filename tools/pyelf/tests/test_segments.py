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

import unittest

import elf
from elf.segment import ElfSegment, DataElfSegment, SectionedElfSegment, HeaderElfSegment
from elf.section import UnpreparedElfSection, UnpreparedElfStringTable
from elf.core import UnpreparedElfFile, InvalidArgument
from elf.ByteArray import ByteArray
from elf.util import Prepared, Unprepared
from elf.constants import PT_NULL, PT_LOAD, PT_PHDR
from elf.structures import Elf32ProgramHeader, ElfProgramHeader

modules_under_test = [elf.segment]

PROG_HEADER_SIZE = 12
DATA_BASE = 0x1000
PHYS_BASE = 0x10000

class TestElfSegment(unittest.TestCase):
    def _get_segments(self):
        elf_file = UnpreparedElfFile()
        sections = [UnpreparedElfSection(elf_file, "test1", data=ByteArray("test1 data")),
                    UnpreparedElfSection(elf_file, "test2", data=ByteArray("test2 data"))]

        empty_sec_seg = SectionedElfSegment(None)
        full_sec_seg = SectionedElfSegment(elf_file, sections=sections)
        
        head_seg = HeaderElfSegment(None)
        prep_head_seg = HeaderElfSegment(None)
        prep_head_seg.prepare(37, PROG_HEADER_SIZE)
        
        data = ByteArray("pants")
        full_data_seg = DataElfSegment(None, vaddr=DATA_BASE, paddr=PHYS_BASE, data=data)
        nobits_data_seg = DataElfSegment(None, vaddr=DATA_BASE, paddr=PHYS_BASE, data=data, memsz=10)

        return empty_sec_seg, full_sec_seg, head_seg, prep_head_seg, full_data_seg, nobits_data_seg

    def test_init(self):
        ElfSegment(None)
        ElfSegment(None, PT_LOAD)
        ElfSegment(None, program_header_type = PT_LOAD)
        sections = [UnpreparedElfSection(None, "foo")]
        self.assertRaises(TypeError, ElfSegment,
                          data = ByteArray("pants"), sections = sections)
        seg = ElfSegment(None, program_header_type=PT_LOAD, vaddr=None, paddr=None)
        self.assertEquals(seg.vaddr, 0)
        self.assertEquals(seg.paddr, 0)

        prog_header = ElfProgramHeader("<")
        prog_header.p_type = PT_LOAD
        prog_header.p_vaddr = 0x1000
        prog_header.p_paddr = 0x10000
        prog_header.p_flags = 0x100
        prog_header.p_align = 0x4000
        seg = ElfSegment(None, prog_header=prog_header)
        self.assertEquals(prog_header.p_type, seg.type)
        self.assertEquals(prog_header.p_vaddr, seg.vaddr)
        self.assertEquals(prog_header.p_paddr, seg.paddr)
        self.assertEquals(prog_header.p_flags, seg.flags)
        self.assertEquals(prog_header.p_align, seg.align)

    def test_copy_into(self):
        elf_from = UnpreparedElfFile()
        elf_to = UnpreparedElfFile()
        seg = DataElfSegment(elf_from, ByteArray("pants"))
        new_seg = seg.copy_into(elf_to)

        seg = SectionedElfSegment(elf_from)
        seg.sections = [UnpreparedElfSection(elf_from, "test")]
        new_seg = seg.copy_into(elf_to)

        seg = DataElfSegment(elf_from, ByteArray("pants"))
        seg._data = ByteArray()
        new_seg = seg.copy_into(elf_to)

        seg = DataElfSegment(elf_from, ByteArray("pants"))
        seg.prepare(34)
        new_seg = seg.copy_into(elf_to)

        seg = HeaderElfSegment(elf_from)
        new_seg = seg.copy_into(elf_to)

    def test_get_file_data(self):
        empty_sec_seg, full_sec_seg, head_seg, prep_head_seg, full_data_seg, nobits_data_seg = self._get_segments()

        data = ByteArray("pants")
        self.assertEqual(full_data_seg.get_file_data(), data)
        self.assertEqual(nobits_data_seg.get_file_data(), data)

    def test_has_sections(self):
        empty_sec_seg, full_sec_seg, head_seg, prep_head_seg, full_data_seg, nobits_data_seg = self._get_segments()        

        self.assertEquals(full_data_seg.has_sections(), False)
        self.assertEquals(nobits_data_seg.has_sections(), False)
        self.assertEquals(full_sec_seg.has_sections(), True)
        self.assertEquals(empty_sec_seg.has_sections(), True)
        self.assertEquals(head_seg.has_sections(), False)
        self.assertEquals(prep_head_seg.has_sections(), False)
    
    def test_get_section(self):
        empty_sec_seg, full_sec_seg, head_seg, prep_head_seg, full_data_seg, nobits_data_seg = self._get_segments()        
        self.assertEquals(empty_sec_seg.get_sections(), [])
        self.assertEquals(len(full_sec_seg.get_sections()), 2)

    def test_replace_section(self):
        elf_from = UnpreparedElfFile()
        elf_to = UnpreparedElfFile()

        seg = SectionedElfSegment(elf_from, sections=[UnpreparedElfSection(elf_from, "test")])
        old_section = seg.get_sections()[0]
        new_section = UnpreparedElfSection(elf_to, "new")
        seg.replace_section(old_section, new_section)
        self.assertEqual(seg.get_sections(), [new_section])
        new_seg = seg.copy_into(elf_to)

        self.assertRaises(InvalidArgument, seg.replace_section, None, new_section)
    
    def test_prepare(self):
        empty_sec_seg, full_sec_seg, head_seg, prep_head_seg, full_data_seg, nobits_data_seg = self._get_segments()        
        self.assertRaises(InvalidArgument, full_data_seg.prepare, 37, PROG_HEADER_SIZE)
        full_data_seg.prepare(37)
        self.assertEqual(full_data_seg.prepared, True)
        self.assertEqual(full_data_seg.offset, 37)
        self.assertRaises(Prepared, full_data_seg.prepare, 37)

        self.assertRaises(InvalidArgument, nobits_data_seg.prepare, 37, PROG_HEADER_SIZE)
        nobits_data_seg.prepare(37)
        self.assertEqual(nobits_data_seg.prepared, True)
        self.assertEqual(nobits_data_seg.offset, 37)
        self.assertRaises(Prepared, nobits_data_seg.prepare, 37)

        self.assertRaises(InvalidArgument, head_seg.prepare, 37)
        head_seg.prepare(37, PROG_HEADER_SIZE)
        self.assertEqual(head_seg.get_memsz(), PROG_HEADER_SIZE)
        self.assertEqual(head_seg.offset, 37)
        self.assertEqual(head_seg.prepared, True)

        self.assertRaises(Prepared, prep_head_seg.prepare, 37, PROG_HEADER_SIZE) # FIXME: what happens if we prepare it to the wrong size??? A. We end up with a malformed program header entry!

        # FIMXE: what about the sec_segs?

    def test_gettype(self):
        empty_sec_seg, full_sec_seg, head_seg, prep_head_seg, full_data_seg, nobits_data_seg = self._get_segments()

        self.assertEquals(empty_sec_seg.type, PT_NULL)
        self.assertEquals(full_sec_seg.type, PT_NULL)
        self.assertEquals(head_seg.type, PT_PHDR)
        self.assertEquals(prep_head_seg.type, PT_PHDR)
        self.assertEquals(full_data_seg.type, PT_NULL)
        self.assertEquals(nobits_data_seg.type, PT_NULL)

    def test_getvaddr(self):
        seg = ElfSegment(None)
        self.assertEquals(seg.vaddr, 0)
        seg = ElfSegment(None, vaddr = 37)
        self.assertEquals(seg.vaddr, 37)

    def test_setvaddr(self):
        seg = ElfSegment(None)
        self.assertEquals(seg.vaddr, 0)
        seg.vaddr = 37
        self.assertEquals(seg.vaddr, 37)

    def test_getpaddr(self):
        seg = ElfSegment(None)
        self.assertEquals(seg.paddr, 0)
        seg = ElfSegment(None, paddr = 37)
        self.assertEquals(seg.paddr, 37)

    def test_setpaddr(self):
        seg = ElfSegment(None)
        self.assertEquals(seg.paddr, 0)
        seg.paddr = 37
        self.assertEquals(seg.paddr, 37)

    def test_getalign(self):
        seg = ElfSegment(None)
        self.assertEquals(seg.align, 0)
        seg = ElfSegment(None, align = 37)
        self.assertEquals(seg.align, 37)

    def test_setalign(self):
        seg = ElfSegment(None)
        self.assertEquals(seg.align, 0)
        seg.align = 37
        self.assertEquals(seg.align, 37)

    def test_getflags(self):
        seg = ElfSegment(None)
        self.assertEquals(seg.flags, 0)
        seg = ElfSegment(None, flags = 37)
        self.assertEquals(seg.flags, 37)

    def test_setflags(self):
        seg = ElfSegment(None)
        self.assertEquals(seg.flags, 0)
        seg.flags = 37
        self.assertEquals(seg.flags, 37)

    def test_getmemsz(self):

        empty_sec_seg, full_sec_seg, head_seg, prep_head_seg, full_data_seg, nobits_data_seg = self._get_segments()

        # the data "pants" is 5 bytes, but the memsz is 10 for nobits
        self.assertEqual(full_data_seg.get_memsz(), 5)
        self.assertEqual(nobits_data_seg.get_memsz(), 10)

        # An unprepared header section can call this yes
        self.assertRaises(Unprepared, head_seg.get_memsz)
        self.assertEqual(prep_head_seg.get_memsz(), PROG_HEADER_SIZE)

        # A segment with no sections has no size
        # What about one with sections?? section nobits??
        self.assertEqual(empty_sec_seg.get_memsz(), 0)
        self.assertEqual(full_sec_seg.get_memsz(), 10)

        # FIXME: do this prepared
        #self.assertEqual(empty_sec_seg.get_memsz(), 0)
        #self.assertNotEqual(full_sec_seg.get_memsz(), 0)
        #print "FIXME:", full_sec_seg.get_memsz()

    def test_getfilesz(self):
        empty_sec_seg, full_sec_seg, head_seg, prep_head_seg, full_data_seg, nobits_data_seg = self._get_segments()

        # the data "pants" is 5 bytes
        self.assertEqual(full_data_seg.get_filesz(), 5)
        self.assertEqual(nobits_data_seg.get_filesz(), 5)

        # An unprepared header section can call this yes
        self.assertRaises(Unprepared, head_seg.get_filesz)
        self.assertEqual(prep_head_seg.get_filesz(), PROG_HEADER_SIZE)

        # A segment with no sections has no size
        # What about one with sections?? nobits? FIXME
        self.assertEqual(empty_sec_seg.get_filesz(), 0)
        self.assertEqual(full_sec_seg.get_filesz(), 10)

        # FIXME: they need to be prepared
        #self.assertEqual(empty_sec_seg.get_filesz(), 0)
        #self.assertNotEqual(full_sec_seg.get_filesz(), 0)
        #print "FIXME:", full_sec_seg.get_filesz()

        # FIXME: scatter load segments calculate this differently
        # but we need soem real sections to check it out

        # Need to be prepared
        #empty_sec_seg.set_scatter_load()
        #full_sec_seg.set_scatter_load()
        #self.assertEqual(empty_sec_seg.get_filesz(), 0)
        #self.assertNotEqual(full_sec_seg.get_filesz(), 0)
        #print "FIXME:", full_sec_seg.get_filesz()


    def test_getoffset(self):

        seg = ElfSegment(None)
        self.assertRaises(Unprepared, seg.get_offset)
        seg.prepare(10)
        self.assertEquals(seg.offset, 10)

    def test_get_program_header(self):
        data = ByteArray()
        data.memsz = 0
        seg = DataElfSegment(None, data = data)
        self.assertRaises(Unprepared, seg.get_program_header, '<', 32)
        seg.prepare(0)
        self.assertRaises(InvalidArgument, seg.get_program_header, '<', 33)
        self.assertRaises(InvalidArgument, seg.get_program_header, 'ads', 32)

        ph = seg.get_program_header('<', 32)
        self.assertEquals(ph.todata(), Elf32ProgramHeader('<').todata())


    def test_remove_section(self):
        empty_sec_seg, full_sec_seg, head_seg, prep_head_seg, full_data_seg, nobits_data_seg = self._get_segments()

        section = UnpreparedElfSection(None)
        self.assertRaises(InvalidArgument, empty_sec_seg.remove_section, section)
        empty_sec_seg.add_section(section)
        self.assertEqual(section in empty_sec_seg.sections, True)
        empty_sec_seg.remove_section(section)
        self.assertEqual(section in empty_sec_seg.sections, False)
        self.assertRaises(InvalidArgument, empty_sec_seg.remove_section, section)

        section = UnpreparedElfSection(None)
        self.assertRaises(InvalidArgument, full_sec_seg.remove_section, section)
        full_sec_seg.add_section(section)
        self.assertEqual(section in full_sec_seg.sections, True)
        full_sec_seg.remove_section(section)
        self.assertEqual(section in full_sec_seg.sections, False)
        self.assertRaises(InvalidArgument, full_sec_seg.remove_section, section)  

    def test_add_section(self):
        empty_sec_seg, full_sec_seg, head_seg, prep_head_seg, full_data_seg, nobits_data_seg = self._get_segments()
        section = UnpreparedElfSection(None)        

        # Adding to a section segment should work
        empty_sec_seg.add_section(section)
        self.assertEqual(section in empty_sec_seg.sections, True)

        full_sec_seg.add_section(section)
        self.assertEqual(section in full_sec_seg.sections, True)

    def test_remove_nobits(self):
        empty_sec_seg, full_sec_seg, head_seg, prep_head_seg, full_data_seg, nobits_data_seg = self._get_segments()

        self.assertEqual(full_data_seg.remove_nobits(), None)
        self.assertEqual(full_data_seg.get_filesz(), full_data_seg.get_memsz())
        self.assertEqual(nobits_data_seg.remove_nobits(), None)
        self.assertEqual(nobits_data_seg.get_filesz(), nobits_data_seg.get_memsz())
        self.assertEqual(nobits_data_seg.get_filesz(), 10)

        # We ignore it o header segment
        self.assertEqual(head_seg.remove_nobits(), None)
        self.assertEqual(prep_head_seg.remove_nobits(), None)

        # FIXME: Need to test a segment which has NOBITS segments
        empty_sec_seg.remove_nobits()
        full_sec_seg.remove_nobits()

    def test___str__(self):
        seg = ElfSegment(None)
        self.assertEqual(str(seg), "<ElfSegment NULL>")
        seg = ElfSegment(None, program_header_type = PT_LOAD)
        self.assertEqual(str(seg), "<ElfSegment LOAD>")

    def test_base_class(self):
        seg = ElfSegment(None)
        self.assertRaises(NotImplementedError, seg.remove_nobits)
        self.assertRaises(NotImplementedError, seg.get_memsz)
        self.assertRaises(NotImplementedError, seg.get_filesz)
        self.assertRaises(NotImplementedError, seg.has_sections)
        self.assertRaises(NotImplementedError, seg.copy_into, None)
    
    def test_set_scatter_load(self):
        segments = self._get_segments()
        for segment in segments:
            if segment.__class__ == SectionedElfSegment:
                self.assertEqual(segment.is_scatter_load(), False)
                segment.set_scatter_load()
                self.assertEqual(segment.is_scatter_load(), True)
                segment.set_scatter_load()
                self.assertEqual(segment.is_scatter_load(), True)
            else:
                self.assertEqual(hasattr(segment, "is_scatter_load"), False)

    def test_prepare_sections(self):        
        empty_sec_seg, full_sec_seg, _, _, _, _ = self._get_segments()

        for seg in [empty_sec_seg, full_sec_seg]:
            all_sections = seg.sections
            sh_string_table = UnpreparedElfStringTable(None)
            ofs = 0x1000
            seg.prepare(ofs)
            seg.prepare_sections(all_sections, sh_string_table)
            #print [sect.offset for sect in seg.sections]

    def test_span(self):
        empty_sec_seg, full_sec_seg, head_seg, prep_head_seg, full_data_seg, nobits_data_seg = self._get_segments()

        # FIXME: check on prepared version
        # FIXME: this has changed
        #self.assertRaises(Unprepared, empty_sec_seg.get_span)
        #self.assertRaises(Unprepared, full_sec_seg.get_span)
        #self.assertRaises(Unprepared, head_seg.get_span)

        span = prep_head_seg.get_span()
        self.assertEquals(span.base, 0)
        self.assertEquals(span.end, PROG_HEADER_SIZE)

        span = full_data_seg.get_span()
        self.assertEquals(span.base, DATA_BASE)
        self.assertEquals(span.end, DATA_BASE + len("pants"))
        
        span = nobits_data_seg.get_span()
        self.assertEquals(span.base, DATA_BASE)
        self.assertEquals(span.end, DATA_BASE + 10)


    def test_contains_vaddr(self):
        empty_sec_seg, full_sec_seg, head_seg, prep_head_seg, full_data_seg, nobits_data_seg = self._get_segments()

        # FIXME: check on prepared version
        # FIXME: this has changed
        #self.assertRaises(Unprepared, empty_sec_seg.contains_vaddr, 0)
        #self.assertRaises(Unprepared, full_sec_seg.contains_vaddr, 0)
        #self.assertRaises(Unprepared, head_seg.contains_vaddr, 0)

        self.assertEquals(prep_head_seg.contains_vaddr(0), True)
        self.assertEquals(prep_head_seg.contains_vaddr(PROG_HEADER_SIZE-1), True)
        self.assertEquals(prep_head_seg.contains_vaddr(PROG_HEADER_SIZE), False)

        self.assertEquals(full_data_seg.contains_vaddr(0), False)
        self.assertEquals(full_data_seg.contains_vaddr(DATA_BASE-1), False)
        self.assertEquals(full_data_seg.contains_vaddr(DATA_BASE), True)
        self.assertEquals(full_data_seg.contains_vaddr(DATA_BASE + len("pants") - 1), True)
        self.assertEquals(full_data_seg.contains_vaddr(DATA_BASE + len("pants")), False)

        self.assertEquals(nobits_data_seg.contains_vaddr(0), False)
        self.assertEquals(nobits_data_seg.contains_vaddr(DATA_BASE-1), False)
        self.assertEquals(nobits_data_seg.contains_vaddr(DATA_BASE), True)
        self.assertEquals(nobits_data_seg.contains_vaddr(DATA_BASE + 10 - 1), True)
        self.assertEquals(nobits_data_seg.contains_vaddr(DATA_BASE + 10), False)


    def test_vtop(self):
        empty_sec_seg, full_sec_seg, head_seg, prep_head_seg, full_data_seg, nobits_data_seg = self._get_segments()

        # FIXME: check on prepared version
        #FIXME: this has changed
        #self.assertRaises(Unprepared, empty_sec_seg.vtop, 0)
        #self.assertRaises(Unprepared, full_sec_seg.vtop, 0)
        #self.assertRaises(Unprepared, head_seg.vtop, 0)

        self.assertRaises(InvalidArgument, prep_head_seg.vtop, PROG_HEADER_SIZE)
        self.assertEquals(prep_head_seg.vtop(0), 0)
        self.assertEquals(prep_head_seg.vtop(PROG_HEADER_SIZE - 1), PROG_HEADER_SIZE - 1)

        self.assertRaises(InvalidArgument, full_data_seg.vtop, DATA_BASE-1)
        self.assertEquals(full_data_seg.vtop(DATA_BASE), PHYS_BASE)
