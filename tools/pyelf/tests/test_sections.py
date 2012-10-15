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
import array

import elf
from elf.core import UnpreparedElfFile, InvalidArgument
from elf.section import UnpreparedElfSection, BaseElfSection, UnpreparedElfStringTable, UnpreparedElfReloc, UnpreparedElfReloca, UnpreparedElfNote, UnpreparedElfSymbolTable
from elf.user_structures import ElfReloc, ElfSymbol
from elf.ByteArray import ByteArray
from elf.constants import SHT_NOBITS, SHT_PROGBITS, SHT_NULL

modules_under_test = [elf.section]

class TestElfSection(unittest.TestCase):
    def test_init(self):
        UnpreparedElfSection(None, "test")
        UnpreparedElfSection(None, "test", SHT_PROGBITS)
        UnpreparedElfSection(None, "test", section_type = SHT_PROGBITS)
        sec = UnpreparedElfSection(None, "test", address = None)
        self.assertEquals(sec.address, 0)
    
    def test_copy_into(self):
        elf_from = UnpreparedElfFile()
        elf_to = UnpreparedElfFile()

        sect = BaseElfSection(elf_from, "test")
        # FIXME
        #self.assertRaises(NotImplementedError, sect.copy_into, elf_to)

        sect = UnpreparedElfSection(elf_from, "test")
        new_sect = sect.copy_into(elf_to)
        self.assertEquals(sect.name, new_sect.name)
        
        prep_sect = sect.prepare(0, 0, 0)
        # FIXME
        #self.assertRaises(NotImplementedError, prep_sect.copy_into, elf_to)

        sect = UnpreparedElfSection(elf_from, "test", SHT_NOBITS)
        new_sect = sect.copy_into(elf_to)
        self.assertEquals(sect.name, new_sect.name)

        sect = UnpreparedElfStringTable(elf_from, "string")
        strings = ["foo", "bar", "baz"]
        for string in strings:
            sect.add_string(string)
        new_sect = sect.copy_into(elf_to)
        for i in range(len(strings)):
            self.assertEquals(sect.get_string_idx(i), new_sect.get_string_idx(i))
            

    def test_gettype(self):
        sect = UnpreparedElfSection(None, "test", SHT_PROGBITS)
        self.assertEqual(sect.type, SHT_PROGBITS)

    def test_settype(self):
        sect = UnpreparedElfSection(None, "test")
        self.assertEqual(sect.type, SHT_NULL)
        sect.type = SHT_PROGBITS
        self.assertEqual(sect.type, SHT_PROGBITS)

    def test_getname(self):
        sect = UnpreparedElfSection(None, "test")
        self.assertEqual(sect.name, "test")

    def test_setname(self):
        sect = UnpreparedElfSection(None, "test")
        self.assertEqual(sect.name, "test")
        sect.name = "pants"
        self.assertEqual(sect.name, "pants")

    def test_setname_prepared(self):
        sect = UnpreparedElfSection(None, "test")
        self.assertEqual(sect.name, "test")
        sect = sect.prepare(0, 0, 0)
        self.assertEqual(hasattr(sect, "set_name"), False) # FIXME: this isn't the right thing to check

    def test_get_name_offset(self):
        sect = UnpreparedElfSection(None, "test")
        self.assertEqual(hasattr(sect, "name_offset"), False) # FIMXE: is this really what we want to check?
        sect = sect.prepare(0, 0, 5)
        self.assertEqual(sect.name_offset, 5)

    def test_set_name_offset(self):
        sect = UnpreparedElfSection(None, "test")
        self.assertEqual(hasattr(sect, "name_offset"), False) # FIMXE: is this really what we want to check
        sect = sect.prepare(0, 0, 5)
        self.assertEqual(sect.name_offset, 5)
        sect.name_offset = 25
        self.assertEqual(sect.name_offset, 25)

    def test_get_address(self):
        sect = UnpreparedElfSection(None, "test")
        self.assertEqual(sect.address, 0)
        sect = UnpreparedElfSection(None, "test", address=25)
        self.assertEqual(sect.address, 25)

    def test_set_address(self):
        sect = UnpreparedElfSection(None, "test")
        self.assertEqual(sect.address, 0)
        sect.address = 25
        self.assertEqual(sect.address, 25)

    def test_get_size(self):
        sect = UnpreparedElfSection(None, "test")
        self.assertEqual(sect.get_size(), 0)

        sect = UnpreparedElfSection(None, "test", data = ByteArray("pants"))
        self.assertEqual(sect.get_size(), 5)

    def test_get_flags(self):
        sect = UnpreparedElfSection(None, "test")
        self.assertEqual(sect.flags, 0)

        sect = UnpreparedElfSection(None, "test", flags = 0xf)
        self.assertEqual(sect.flags, 0xf)

    def test_set_flags(self):
        sect = UnpreparedElfSection(None, "test")
        self.assertEqual(sect.flags, 0)
        sect.flags = 0xff
        self.assertEqual(sect.flags, 0xff)

    def test_get_link(self):
        sect = UnpreparedElfSection(None, "test")
        self.assertEqual(sect.link, None)

        sect2 = UnpreparedElfSection(None, "test_link", link = sect)
        self.assertEqual(sect2.link, sect)

        sect3 = self.assertRaises(InvalidArgument, UnpreparedElfSection, "test_link", link = 80)

    def test_get_info(self):
        sect = UnpreparedElfSection(None, "test")
        self.assertEqual(sect.info, None)
        sect = UnpreparedElfSection(None, "test", info = 13)
        self.assertEqual(sect.info, 13)

    def test_set_info(self):
        sect = UnpreparedElfSection(None, "test")
        self.assertEqual(sect.info, None)
        sect.info = 13
        self.assertEqual(sect.info, 13)
        
    def test_get_entsize(self):
        # FIXME:
        #sect = UnpreparedElfSection(None, "test")
        #self.assertEqual(sect.entsize, 0)
        pass

    def test_set_entsize(self):
        # FIXME:
        #sect = UnpreparedElfSection(None, "test")
        #self.assertEqual(sect.entsize, 0)
        #sect.entsize = 13
        #self.assertEqual(sect.entsize, 13)

        pass

    def test_get_addralign(self):
        sect = UnpreparedElfSection(None, "test")
        self.assertEqual(sect.addralign, 0)
        sect = UnpreparedElfSection(None, "test", addralign = 13)
        self.assertEqual(sect.addralign, 13)

    def test_set_addralign(self):
        sect = UnpreparedElfSection(None, "test")
        self.assertEqual(sect.addralign, 0)
        sect.addralign = 13
        self.assertEqual(sect.addralign, 13)

    def test_get_offset(self):
        sect = UnpreparedElfSection(None, "test")
        self.assertEqual(hasattr(sect, "get_offset"), False)
        sect = sect.prepare(15, 0, 0)
        self.assertEqual(sect.offset, 15)

    def test_get_index(self):
        sect = UnpreparedElfSection(None, "test")
        self.assertEqual(hasattr(sect, "get_index"), False)
        sect = sect.prepare(0, 15, 0)
        self.assertEqual(sect.index, 15)

    def test_get_file_data(self):
        data = ByteArray("pants")
        sect = UnpreparedElfSection(None, "test", data = data)
        self.assertEquals(sect.get_file_data(), data)
        
    def test_get_data(self):
        data = ByteArray("pants")
        sect = UnpreparedElfSection(None, "test", data = data)
        self.assertEquals(sect.get_data(), data)

    def test_data_append(self):
        data = ByteArray("pants")
        sect = UnpreparedElfSection(None, "test", data = data)
        self.assertEquals(sect.get_data(), data)
        sect.append_data(ByteArray("foo"))
        self.assertEquals(sect.get_data(), ByteArray("pantsfoo"))
        sect = sect.prepare(0, 0, 0)
        self.assertEqual(hasattr(sect, "data_append"), False)

    def test_get_section_header(self):
        # FIXME: This test uses broken semantics now that elf sections can't
        # live on their own in the wild

        sect = UnpreparedElfSection(None, "test")
        self.assertEqual(hasattr(sect, "get_section_header"), False)

        """
        sect = sect.prepare(0, 0, 0)

        sh = sect.get_section_header()
        self.assertEquals(sh.todata(), Elf32SectionHeader('<').todata())

        sect = UnpreparedElfSection(None, "test")
        sect2 = UnpreparedElfSection(None, "test")
        sect.link = sect2
        sect2 = sect2.prepare(0, 2, 0)
        sect = sect.prepare(0, 0, 0)
        sh = sect.get_section_header()
        self.assertEquals(sh.sh_link, 2)
        """

    def test_get_num_entries(self): # FIXME: need to fix how we handle entries...
        # FIXME
        #sect = UnpreparedElfSection(None, "test", entsize=4)
        #sect.data_append(ByteArray("12345678"))
        #self.assertEquals(sect.get_num_entries(), 2)
        pass

    def test_get_entry(self):
        # FIXME
        #sect = UnpreparedElfSection(None, "test", entsize=4)
        #sect.data_append(ByteArray("12345678"))
        #self.assertEquals(sect.get_entry(0), ByteArray("1234"))
        #self.assertEquals(sect.get_entry(1), ByteArray("5678"))
        pass

    def test_get_entries(self):
        # FIXME
        #sect = UnpreparedElfSection(None, "test", entsize=4)
        #sect.data_append(ByteArray("12345678"))
        #self.assertEquals(list(sect.get_entries()), [ByteArray("1234"), ByteArray("5678")])
        pass

    def test_repr(self):
        sect = UnpreparedElfSection(None, "test")
        self.assertEquals(repr(sect), "<UnpreparedElfSection NULL test>")
        


class TestStringTable(unittest.TestCase):

    def _get_string_tables(self):
        empty_st = UnpreparedElfStringTable(None)
        added_st = UnpreparedElfStringTable(None)
        added_st.add_string("name.")
        added_st.add_string("Variable")
        added_st.add_string("able")
        added_st.add_string("xx")
        data = ByteArray('\0name.\0Variable\0able\0\0xx\0')
        data_st = UnpreparedElfStringTable(None, data = data)

        prep_empty_st = UnpreparedElfStringTable(None)
        prep_added_st = UnpreparedElfStringTable(None)
        prep_added_st.add_string("name.")
        prep_added_st.add_string("Variable")
        prep_added_st.add_string("able")
        prep_added_st.add_string("xx")
        data = ByteArray('\0name.\0Variable\0able\0xx\0')
        prep_data_st = UnpreparedElfStringTable(None, data = data)
        prep_empty_st = prep_empty_st.prepare(1, 2, 3)
        prep_added_st = prep_added_st.prepare(1, 2, 3)
        prep_data_st = prep_data_st.prepare(1, 2, 3)
        return empty_st, added_st, data_st, prep_empty_st, prep_added_st, prep_data_st

    def test_get_file_data(self):
        tables = self._get_string_tables()
        for table in tables:
            data = table.get_file_data()
            self.assertEquals(len(data), table.get_size())
            for string in ["name.", "Variable", "able", "xx"]:
                if table.offset_of(string) is not None:
                    self.assertEquals(string in data.tostring(), True)
            for string in table.get_strings():
                self.assertEquals(string in data.tostring(), True)
                    
    def test_offset_of(self):
        empty_st, added_st, data_st, prep_empty_st, prep_added_st, prep_data_st = self._get_string_tables()
        for string in ["name.", "Variable", "able", "xx", "me.", "", "foo"]:
            for st in [empty_st, prep_empty_st]:
                if string:
                    self.assertEqual(st.offset_of(string), None)
                else:
                    self.assertEqual(st.offset_of(string), 0)
            for st in [added_st, data_st, prep_added_st, prep_data_st]:
                offset = st.offset_of(string)
                if offset:
                    self.assertEqual(st.get_string_ofs(offset), string)

    def test_index_of(self):
        empty_st, added_st, data_st, prep_empty_st, prep_added_st, prep_data_st = self._get_string_tables()
        for string in ["name.", "Variable", "able", "xx", ".me", "", "foo"]:
            for st in [empty_st, prep_empty_st]:
                if string:
                    self.assertEqual(st.index_of(string), None)
                else:
                    self.assertEqual(st.index_of(string), 0)
            for st in [added_st, data_st, prep_added_st, prep_data_st]:
                index = st.index_of(string)
                if index:
                    self.assertEqual(st.get_string_idx(index), string)

    def test_add_string(self):
        empty_st, added_st, data_st, prep_empty_st, prep_added_st, prep_data_st = self._get_string_tables()
        for table in [empty_st, added_st, data_st]:
            for string in ["", "foo", "bar", "Variable", "able"]:
                offset = table.add_string(string)
                self.assertEqual(table.get_string_ofs(offset), string)
        for table in [prep_empty_st, prep_added_st, prep_data_st]:
            self.assertRaises(AttributeError, lambda x, y: x.add_string(y), table, "foo")
        
    def test_getstringidx_out_of_bounds(self):
        st = UnpreparedElfStringTable(None)
        self.assertEqual(st.get_string_idx(1), None)

    def test_nulltable(self):
        st = UnpreparedElfStringTable(None)
        self.assertEqual(st.get_string_idx(0), "")
        self.assertEqual(st.index_of(""), 0)
        self.assertEqual(st.index_of("anything"), None)
        self.assertEqual(st.offset_of(""), 0)
        self.assertEqual(st.offset_of("anything"), None)
        self.assertEqual(st.get_file_data(), array.array('B', '\x00'))

    def test_simpletable(self):
        st = UnpreparedElfStringTable(None)
        self.assertEqual(st.add_string("name."), 1)
        self.assertEqual(st.add_string("Variable"), 7)
        self.assertEqual(st.add_string("able"), 16) # this isn't a good test. we don't know it will be at 16, it'd just be obvious. it could also be at 11 or so
        self.assertEqual(st.add_string("xx"), 21)
        self.assertEqual(len(st.get_strings()), 5)
        self.assertEqual(st.get_string_idx(1), "name.")
        self.assertEqual(st.get_string_idx(2), "Variable")
        self.assertEqual(st.get_string_idx(3), "able")
        self.assertEqual(st.get_string_idx(4), "xx")
        self.assertEqual(st.get_string_ofs(0), "")
        self.assertEqual(st.get_string_ofs(1), "name.")
        self.assertEqual(st.get_string_ofs(7), "Variable")
        self.assertEqual(st.get_string_ofs(11), "able")
        self.assertEqual(st.get_string_ofs(16), "able")                                
        self.assertEqual(st.get_file_data(), array.array('B', '\0name.\0Variable\0able\0xx\0'))

    def test_fromdata(self):
        data = ByteArray('\0name.\0Variable\0able\0xx\0')
        st = UnpreparedElfStringTable(None, data = data)
        st = st.prepare(0, 0, 0)
        self.assertEqual(len(st.get_strings()), 5)
        self.assertEqual(st.get_string_idx(1), "name.")
        self.assertEqual(st.get_string_idx(2), "Variable")
        self.assertEqual(st.get_string_idx(3), "able")
        self.assertEqual(st.get_string_idx(4), "xx")
        self.assertEqual(st.get_string_ofs(0), "")
        self.assertEqual(st.get_string_ofs(1), "name.")
        self.assertEqual(st.get_string_ofs(7), "Variable")
        self.assertEqual(st.get_string_ofs(11), "able")
        self.assertEqual(st.get_string_ofs(16), "able")                                
        self.assertEqual(st.get_file_data(), data)

class TestNote(unittest.TestCase):
    """Test the NOTE elf section."""

    def test_init(self):
        data = ByteArray("\0\0\0\7" + "\0\0\0\7" + "\0\0\0\1" + "abcdef\0\0" + "ghijkl\0\0")        
        note = UnpreparedElfNote(None, data=data)
        #print note.note_name


class TestElfRelocaSection(unittest.TestCase):

    def test_empty_reloca(self):
        ef = UnpreparedElfFile()
        ef.wordsize = 32
        reloca_sect = UnpreparedElfReloca(ef)

        self.assertEqual(reloca_sect.get_size(), 0)

class TestElfRelocSection(unittest.TestCase):

    def test_empty_reloca(self):
        ef = UnpreparedElfFile()
        ef.wordsize = 32
        reloc_sect = UnpreparedElfReloc(ef)

        self.assertEqual(reloc_sect.get_size(), 0)

    def test_add_reloc(self):
        ef = UnpreparedElfFile()
        ef.wordsize = 32
        reloc_sect = UnpreparedElfReloc(ef)
        for offset in range(0, 0x10000, 0x1000):
            reloc = ElfReloc(offset)
            reloc_sect.add_reloc(reloc)

        # we expect to have the relocs we added
        self.assertEqual(len(reloc_sect.relocs), 16)

class TestElfSymbolTable(unittest.TestCase):

    def test_init(self):
        ef = UnpreparedElfFile()
        symtab = UnpreparedElfSymbolTable(ef, ".symtab")

    def test_add_symbols(self):
        ef = UnpreparedElfFile()
        sect = UnpreparedElfSection(ef, "test_sect")
        symtab = UnpreparedElfSymbolTable(ef, ".symtab")
        for name in ["foo", "bar"]:
            symtab.add_symbol(ElfSymbol(name, sect))
        
        ef = UnpreparedElfFile()
        sect = UnpreparedElfSection(ef, "test_ect")
        strtab = UnpreparedElfStringTable(ef, ".strtab")
        symtab = UnpreparedElfSymbolTable(ef, ".symtab", link=strtab)
        for name in ["foo", "bar"]:
            symtab.add_symbol(ElfSymbol(name, sect))
        
    def test_prepare(self):
        ef = UnpreparedElfFile()
        sect = UnpreparedElfSection(ef, "test_sect")
        symtab = UnpreparedElfSymbolTable(ef, ".symtab")
        for name in ["foo", "bar"]:
            symtab.add_symbol(ElfSymbol(name, sect))

        ef.wordsize = 32
        ef.endianess = "<"
        symtab = symtab.prepare(0x1000, 1, 0)

    def test_get_file_data(self):
        ef = UnpreparedElfFile()
        symtab = UnpreparedElfSymbolTable(ef, ".symtab")
        self.assertRaises(NotImplementedError, symtab.get_file_data)
