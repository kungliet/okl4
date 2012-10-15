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
import StringIO

import elf
from elf.user_structures import ElfSymbol, ElfReloc, ElfReloca
from elf.core import UnpreparedElfFile
from elf.section import UnpreparedElfSection
from elf.structures import ElfSymbolStruct, ElfRelocStruct, ElfRelocaStruct
from elf.constants import EM_ARM

modules_under_test = [elf.user_structures]

class TestElfSymbol(unittest.TestCase):

    def _build_test_symbol(self, name="test_symbol"):
        ef = UnpreparedElfFile()
        sect = UnpreparedElfSection(ef)
        symbol = ElfSymbol(name, sect)
        return ef, sect, symbol

    def _set_symbol_values(self, symbol):
        """ Stick some arbitrary values into a symbol """
        symbol.value = 1
        symbol.size = 2
        symbol.type = 3
        symbol.bind = 4
        symbol.other = 5
        symbol.shndx = 6

    def test_init(self):

        name = "test_symbol"
        ef, sect, symbol = self._build_test_symbol(name)

        self.assertEqual(symbol.name, name)
        self.assertEqual(symbol.section, sect)
        self.assertEqual(symbol.value, 0)
        self.assertEqual(symbol.size, 0)
        self.assertEqual(symbol.type, 0)
        self.assertEqual(symbol.bind, 0)
        self.assertEqual(symbol.other, 0)
        self.assertEqual(symbol.shndx, 0)

        self._set_symbol_values(symbol)

        new_name = "new_symbol"
        new_symbol = ElfSymbol(new_name, sect, old_sym=symbol)

        self.assertEqual(new_symbol.name, new_name)
        self.assertEqual(new_symbol.section, sect)
        self.assertEqual(new_symbol.value, 1)
        self.assertEqual(new_symbol.size, 2)
        self.assertEqual(new_symbol.type, 3)
        self.assertEqual(new_symbol.bind, 4)
        self.assertEqual(new_symbol.other, 5)
        self.assertEqual(new_symbol.shndx, 6)

        sym_struct = ElfSymbolStruct("<")
        sym_struct.st_value = 1
        sym_struct.st_size = 2
        sym_struct.st_type = 3
        sym_struct.st_bind = 4
        sym_struct.st_other = 5
        sym_struct._st_shndx = 6
        symbol = ElfSymbol("struct_symbol", sect, sym_struct=sym_struct)

        self.assertEqual(symbol.name, "struct_symbol")
        self.assertEqual(symbol.section, sect)
        self.assertEqual(symbol.value, 1)
        self.assertEqual(symbol.size, 2)
        self.assertEqual(symbol.type, 3)
        self.assertEqual(symbol.bind, 4)
        self.assertEqual(symbol.other, 5)
        self.assertEqual(symbol.shndx, 6)


                                                
    def test_repr(self):
        ef, sect, symbol = self._build_test_symbol()
        s1 = str(symbol)

        # We expect a differnt result when we have actual values
        symbol.value = 37
        symbol.size = 100
        s2 = str(symbol)
        self.assertNotEqual(s1, s2)

        # Same symbol, different section, should give differnt string
        sect = UnpreparedElfSection(ef, "test_section")
        symbol2 = ElfSymbol("test_symbol", sect, old_sym=symbol)
        s3 = str(symbol2)
        self.assertNotEqual(s2, s3)

    def test_output(self):
        ef, sect, symbol = self._build_test_symbol()

        # Our symbol has no values, so this should raise a value error
        # FIXME:
        #self.assertRaises(ValueError, symbol.output, 0)

        self._set_symbol_values(symbol)
        # Should be able to write out to a file-like object now
        f = StringIO.StringIO()
        symbol.output(0, f)
        
    def test_copy_into(self):
        ef, sect, symbol = self._build_test_symbol()
        self._set_symbol_values(symbol)

        sect = UnpreparedElfSection(ef, "new_section")
        new_symbol = symbol.copy_into(sect)
        self.assertEqual(symbol.value, new_symbol.value)
        self.assertEqual(symbol.size, new_symbol.size)
        self.assertEqual(symbol.type, new_symbol.type)
        self.assertEqual(symbol.bind, new_symbol.bind)
        self.assertEqual(symbol.other, new_symbol.other)
        self.assertEqual(symbol.shndx, new_symbol.shndx)
        self.assertEqual(symbol.name, new_symbol.name)
        self.assertEqual(new_symbol.section, sect)


class TestElfReloc(unittest.TestCase):

    def test_init(self):
        reloc = ElfReloc()
        self.assertEqual(reloc.offset, 0)
        self.assertEqual(reloc.type, 0)
        self.assertEqual(reloc.symdx, 0)

        reloc = ElfReloc(1, 2, 3)
        self.assertEqual(reloc.offset, 1)
        self.assertEqual(reloc.type, 2)
        self.assertEqual(reloc.symdx, 3)

        reloc_struct = ElfRelocStruct("<", EM_ARM)
        reloc_struct.r_offset = 1
        reloc_struct.r_type = 2
        reloc_struct.r_symdx = 3

        reloc = ElfReloc(reloc_struct=reloc_struct)
        self.assertEqual(reloc.offset, 1)
        self.assertEqual(reloc.type, 2)
        self.assertEqual(reloc.symdx, 3)


    def test_output(self):
        reloc = ElfReloc(1, 2, 3)
        f = StringIO.StringIO()
        reloc.output(f)


class TestElfReloca(unittest.TestCase):

    def test_init(self):
        reloc = ElfReloca()
        self.assertEqual(reloc.offset, 0)
        self.assertEqual(reloc.type, 0)
        self.assertEqual(reloc.symdx, 0)
        self.assertEqual(reloc.addend, 0)

        reloc = ElfReloca(1, 2, 3, 4)
        self.assertEqual(reloc.offset, 1)
        self.assertEqual(reloc.type, 2)
        self.assertEqual(reloc.symdx, 3)
        self.assertEqual(reloc.addend, 4)

        reloc_struct = ElfRelocaStruct("<", EM_ARM)
        reloc_struct.r_offset = 1
        reloc_struct.r_type = 2
        reloc_struct.r_symdx = 3
        reloc_struct.r_addend = 4
        reloc = ElfReloca(reloc_struct=reloc_struct)
        self.assertEqual(reloc.offset, 1)
        self.assertEqual(reloc.type, 2)
        self.assertEqual(reloc.symdx, 3)
        self.assertEqual(reloc.addend, 4)


    def test_output(self):
        reloc = ElfReloca(1, 2, 3, 4)
        f = StringIO.StringIO()
        reloc.output(f)

