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
Test file for elf.py
"""

import unittest
import os
from elf.ByteArray import ByteArray
import elf.core
from elf.File import File
from elf.core import UnpreparedElfSection, UnpreparedElfFile, \
     InvalidArgument, PreparedElfFile, BaseElfFile
from elf.constants import SHT_NOBITS, SHT_PROGBITS
from elf.segment import SectionedElfSegment, DataElfSegment, HeaderElfSegment
from elf.user_structures import ElfSymbol

modules_under_test = [elf.core]

elf_address = {
    "data/null_elf" : (0, None, 0, None, 0, 0),
    "data/arm_exec" : (0xf0028000L, 0xa0128000L, 0xf0000000L, 0xa0100000L, 20, 1),
    "data/arm_exec_nosect" : (0xf0028000L, 0x43d3000, 0x4330000, 0x04100000, 0, 15),
    "data/arm_stripped_exec" : (0x80110000L, 0x80110000L, 0x80100000L, 0x80100000L, 0, 2),
    "data/arm_object" : (0x1bc, None, 0, None, 9, 0),
    "data/arm_scatter_load" : (0x0010245CL, 0x0010245CL, 0x00100000L, 0x00100000L, 17, 1),
    "data/amd64_exec" : (0x600000L, 0x600000L, 0x400000L, 0x400000L, 26, 8),
    "data/ia32_exec" : (0xF011C000L, 0x147000L, 0x13c000L, 0x00100200L, 22, 4),
    "data/ia64_exec" : (0x6000000000010000L, 0x6000000000010000L, 0x4000000000000000L, 0x4000000000000000L, 31, 7),
    "data/mips64_exec" : (0xffffffff80076000L, 0x76000, 0xffffffff80050000L, 0x50000, 11, 2)
    }

            
class TestElfFile(unittest.TestCase):
    def test__addr_type_is_virt(self):
        self.assertEquals(BaseElfFile._addr_type_is_virt("virtual"), True)
        self.assertEquals(BaseElfFile._addr_type_is_virt("physical"), False)
        self.assertRaises(InvalidArgument, BaseElfFile._addr_type_is_virt, None)
        self.assertRaises(InvalidArgument, BaseElfFile._addr_type_is_virt, 37)
        self.assertRaises(InvalidArgument, BaseElfFile._addr_type_is_virt, "asdf")

    def test_null_elf(self):
        for endianess, wordsize, expectedsize in \
            [ ('<', 32, 52), ('>', 32, 52),
              ('<', 64, 64), ('>', 64, 64) ]:
            ef = UnpreparedElfFile()
            self.assertEqual(len(ef.sections), 1)
            self.assertEqual(ef.has_segments(), False)
            ef = ef.prepare(wordsize, endianess)
            f = File("test.elf", "wb")
            data = ef.todata()
            for offset, _dat in data:
                f.seek(offset)
                _dat.tofile(f)
            f.flush()
            self.assertEqual(f.size(), expectedsize)
            f.close()
            os.remove("test.elf")

    def test_add_section(self):
        ef = UnpreparedElfFile()
        sect = UnpreparedElfSection(None)
        ef.add_section(sect)
        self.assertEqual(ef.sections[-1], sect)

    def test_remove_section(self):
        ef = UnpreparedElfFile()
        sect = UnpreparedElfSection(None)
        self.assertRaises(InvalidArgument, ef.remove_section, sect)
        ef.add_section(sect)
        self.assertEqual(sect in ef.sections, True)
        ef.remove_section(sect)
        self.assertEqual(sect in ef.sections, False)

        seg = SectionedElfSegment(None)
        ef.add_segment(seg)
        ef.add_section(sect)
        seg.add_section(sect)
        self.assertEqual(sect in ef.sections, True)
        self.assertEqual(sect in seg.sections, True)  
        ef.remove_section(sect)
        self.assertEqual(sect in ef.sections, False)
        self.assertEqual(sect in seg.sections, False)  

    def test_remove_nobits(self):
        for file_name in ["data/null_elf",
                          "data/arm_exec",
                          "data/arm_stripped_exec",
                          "data/arm_exec_nosect",
                          "data/arm_object",
                          "data/mips64_exec",
                          "data/mips32_exec",
                          "data/ia32_exec",
                          "data/amd64_exec",
                          "data/ia64_exec",
                          ]:
            ef = PreparedElfFile(filename=file_name)
            if ef.segments:
                seg_sizes = []
                for segment in ef.segments:
                    seg_sizes.append((segment.get_memsz(), segment.get_filesz()))
                ef.remove_nobits()
                for segment, seg_size in zip(ef.segments, seg_sizes):
                    self.assertEqual(segment.get_memsz(), segment.get_filesz())
                    self.assertEqual(segment.get_memsz(), seg_size[0])

        ef = UnpreparedElfFile()
        sec = UnpreparedElfSection(None, section_type=SHT_NOBITS, data=10)
        self.assertEqual(sec.get_size(), 10)
        ef.add_section(sec)
        ef.remove_nobits()
        sec = ef.sections[1]
        self.assertEqual(sec.type, SHT_PROGBITS)
        self.assertEqual(sec.get_size(), 10)
        self.assertEqual(sec.get_file_data(), ByteArray('\0' * 10))

    def test_add_segment(self):
        ef = UnpreparedElfFile()
        seg = DataElfSegment(None, data=ByteArray("pants"))
        ef.add_segment(seg)
        self.assertEqual(ef.segments[-1], seg)
        
    """
    def test_malformed_stringtable(self):
        ef = ElfFile()
        self.assertRaises(elf.core.ElfFormatError, ef.init_and_prepare_from_filename,
                          "data/malformed_stringtable")
        """

    def test_prepare(self):
        ef = UnpreparedElfFile()
        segment = SectionedElfSegment(None, align=0x1)
        new_sect = UnpreparedElfSection(None, "pants")
        ef.add_section(new_sect)
        segment.add_section(new_sect)
        ef.add_segment(segment)
        ef.add_segment(HeaderElfSegment(None))
        ef = ef.prepare(32, '<')

    def test_round_trip(self):
        for file_name in ["data/null_elf",
                          "data/arm_exec",
                          "data/arm_stripped_exec",
                          "data/arm_exec_nosect",
                          "data/arm_object",
                          "data/arm_scatter_load",
                          "data/mips64_exec",
                          "data/mips32_exec",
                          "data/ia32_exec",
                          "data/amd64_exec",
                          "data/ia64_exec",
                          ]:
            ef = PreparedElfFile(filename=file_name)
            ef.to_filename("elf.tmp")
            # FIXME
            # self.assertEqual(open("elf.tmp", "rb").read(), open(file_name, "rb").read(), "%s: failed to read back correctly" % file_name)
            os.remove("elf.tmp")

    def test_long_round_trip(self):
        for file_name in ["data/null_elf",
                          "data/arm_exec",
                          "data/arm_stripped_exec",
                          "data/arm_exec_nosect",
                          "data/arm_object",
                          "data/arm_scatter_load",
                          "data/mips64_exec",
                          "data/mips32_exec",
                          "data/ia32_exec",
                          "data/amd64_exec",
                          "data/ia64_exec",
                          ]:
            ef = UnpreparedElfFile(filename=file_name)
            ef = ef.prepare(ef.wordsize, ef.endianess)
            ef.to_filename("elf.tmp")
            # FIXME: We can't be sure that the files produced will be byte for
            # byte equal at this point. We need to come up with a test for
            # equivalance independant of things such as section ordering.
            # self.assertEqual(open("elf.tmp", "rb").read(), open(file_name, "rb").read(), "%s: failed to read back correctly" % file_name)
            os.remove("elf.tmp")

    def test_from_file(self):
        for filename in elf_address:
            ef = PreparedElfFile(filename=filename)
            # self.assertEqual(len(ef.sections), elf_address[filename][4])
            self.assertEqual(len(ef.segments), elf_address[filename][5])

    def test_vtop(self):
        ef = PreparedElfFile(filename="data/arm_exec")
        seg = ef.find_segment_by_vaddr(0xf0001000L)
        self.assertEqual(seg.vtop(0xf0001000L), 0xa0101000L)

        self.assertRaises(InvalidArgument, seg.vtop, 0xa0001000L)

    def test_find_segment_by_vaddr(self):
        ef = PreparedElfFile(filename="data/arm_exec")
        seg = ef.find_segment_by_vaddr(0xf0001000L)
        self.assertEqual(0xf0001000L in seg.get_span(), True)

        seg = ef.find_segment_by_vaddr(0xa0001000L)
        self.assertEqual(seg, None)

    def test_contains_vaddr(self):
        ef = PreparedElfFile(filename="data/arm_exec")
        seg = ef.find_segment_by_vaddr(0xf0001000L)
        self.assertEqual(seg.contains_vaddr(0xf0001000L), True)

    def test_find_section_named(self):
        ef = PreparedElfFile(filename="data/arm_exec")
        sect = ef.find_section_named(".kip")
        self.assertEqual(sect.name, ".kip")

        sect = ef.find_section_named("notexist")
        self.assertEqual(sect, None)

    def test_get_first_addr(self):
        for filename in elf_address:
            ef = PreparedElfFile(filename=filename)
            self.assertEqual(ef.get_first_addr(), elf_address[filename][2])


    def test_get_last_addr(self):
        for filename in elf_address:
            ef = PreparedElfFile(filename=filename)
            self.assertEqual(ef.get_last_addr(), elf_address[filename][0],
                             "%s 0x%x != 0x%x" % (filename, ef.get_last_addr(),
                                                  elf_address[filename][0]))

    def test_get_address_span(self):
        for filename in elf_address:
            ef = PreparedElfFile(filename=filename)
            self.assertEqual(ef.get_address_span(), elf_address[filename][0] - elf_address[filename][2],
                             "%s 0x%x != 0x%x" % (filename, ef.get_address_span(),
                                                  elf_address[filename][0] - elf_address[filename][2]))

    def test_get_first_paddr(self):
        for filename in elf_address:
            ef = PreparedElfFile(filename=filename)
            if elf_address[filename][3] is None:
                self.assertRaises(InvalidArgument, ef.get_first_addr, "physical")
            else:
                self.assertEqual(ef.get_first_addr("physical"), elf_address[filename][3])

    def test_get_last_paddr(self):
        for filename in elf_address:
            ef = PreparedElfFile(filename=filename)
            if elf_address[filename][1] is None:
                self.assertRaises(InvalidArgument, ef.get_last_addr, "physical")
            else:
                self.assertEqual(ef.get_last_addr("physical"), elf_address[filename][1],
                                 "%s 0x%x != 0x%x" % (filename, ef.get_last_addr("physical"),
                                                      elf_address[filename][1]))

    def test_get_address_span_phys(self):
        for filename in elf_address:
            ef = PreparedElfFile(filename=filename)
            if elf_address[filename][1] is None:
                self.assertRaises(InvalidArgument, ef.get_last_addr, "physical")
            else:
                self.assertEqual(ef.get_address_span("physical"), elf_address[filename][1] - elf_address[filename][3],
                                 "%s 0x%x != 0x%x" % (filename, ef.get_address_span(),
                                                      elf_address[filename][1] - elf_address[filename][3]))

    def test_set_ph_offset(self):
        ef = UnpreparedElfFile()
        ef.set_ph_offset(100, True)
        self.assertEqual(ef._ph_offset, 100)
        self.assertRaises(InvalidArgument, ef.set_ph_offset, 1, True)
        ef = ef.prepare(32, "<")
        self.assertEqual(hasattr(ef, "set_ph_offset"), False)

    def test_et_ph_offset(self):
        ef = UnpreparedElfFile()
        ef.set_ph_offset(100, True)
        self.assertEqual(ef.get_ph_offset(), 100)
        ef = ef.prepare(32, "<")
        self.assertEqual(ef.get_ph_offset(), 100)

    def test_find_symbol(self):
        ef = PreparedElfFile(filename="data/arm_exec")
        self.assertEquals(ef.find_symbol("notexistant"), None)
        sym = ef.find_symbol("do_ipc_copy")
        self.assertEqual(sym.name, "do_ipc_copy")

        sym = ef.find_symbol("_end_kip")
        self.assertEqual(sym.name, "_end_kip")

    def test_get_paddr(self):
        ef = PreparedElfFile(filename="data/arm_exec")
        self.assertEqual(ef.get_paddr(), 0xa0100000)
        ef = PreparedElfFile(filename="data/arm_object")
        self.assertRaises(InvalidArgument, ef.get_paddr)

    def test_add_symbols(self):

        # Adding symbols to a file with no symbol table
        # should create a new symbol table
        symbols = []
        ef = UnpreparedElfFile()
        self.assertEqual(ef.get_symbol_table(), None)
        self.assertEqual(ef.get_symbols(), [])
        ef.add_symbols(symbols)
        self.assertNotEqual(ef.get_symbol_table(), None)
        # Apart from the opening null symbol, we should have nothing
        self.assertEqual(ef.get_symbols()[1:], [])

        sect_a = UnpreparedElfSection(ef)
        symbols = [ElfSymbol("a", sect_a), ElfSymbol("b", sect_a)]
        ef.add_symbols(symbols)
        self.assertEqual(ef.get_symbols()[1:], symbols)

        sect_b = UnpreparedElfSection(ef)
        symbols_b = [ElfSymbol("c", sect_b), ElfSymbol("d", sect_b)]
        ef.add_symbols(symbols_b)
        self.assertEqual(ef.section_symbols(sect_a), symbols)
        self.assertEqual(ef.section_symbols(sect_b), symbols_b)

        symbol_dict = {}
        for name in ["foo", "bar"]:
            symbol = ElfSymbol(name, sect_a)
            symbol_dict[name] = symbol
        ef.add_symbols(symbol_dict.values())

        for name in symbol_dict.keys():
            #full string match
            self.assertEqual(ef.find_symbol(name), symbol_dict[name])

            # partial suffix match
            self.assertEqual(ef.find_symbol(name[1:]), symbol_dict[name])

        self.assertEqual(ef.find_symbol("missing"), None)
        
