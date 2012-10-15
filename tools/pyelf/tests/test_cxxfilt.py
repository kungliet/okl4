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

from parsers.cxxfilt.strings import MangledString
import parsers.cxxfilt.cxxfilt
import unittest

modules_under_test = [parsers.cxxfilt.cxxfilt, parsers.cxxfilt.demangle, parsers.cxxfilt.demangled_types, parsers.cxxfilt.strings]

class TestCXXFilt(unittest.TestCase):
    def setUp(self):
        self.infiles = ["data/symbols_kernel", "data/symbols_libstdc++", "data/symbols_libQtCore"]

    def tearDown(self):
        pass

    def test_strings(self):
        test_str = MangledString("Some random test string")
        self.assertEquals(test_str.peek(), "S")
        test_str.advance(3)
        self.assertEquals(test_str.peek(), "e")
        self.assertEquals(test_str.pos, 3)
        self.assertEquals(test_str.get(4), "e ra")
        self.assertEquals(test_str.check("n"), True)
        self.assertEquals(test_str.peek(1), "o")
        self.assertEquals(test_str.check("o"), False)
        self.assertEquals(test_str.peek(2, 3), "m t")
        test_str.advance(14)
        self.assertEquals(test_str.peek(), "g")
        self.assertEquals(test_str.get(), "g")
        test_str.pos -= 1
        self.assertEquals(test_str.check("g"), True)
        self.assertEquals(test_str.peek(), "")
        self.assertEquals(test_str.peek(1), "")
        self.assertEquals(test_str.peek(2, 6), "")
        self.assertEquals(test_str.get(), "")
        self.assertEquals(test_str.get(5), "")
        self.assertEquals(test_str.check("a"), False)

    def test_symbols(self):
        for file in self.infiles:
            input_file = open(file, "r")
            match_file = open(file + "_demangled", "r")

            for data in input_file:
                match = match_file.readline()
                data = data.strip()
                match = match.strip()

                str = parsers.cxxfilt.cxxfilt.demangle(data)

                self.assertEquals(str, match)

            input_file.close()
            match_file.close()

