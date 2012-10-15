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
Test file for machine.py
"""

import unittest
import weaver
from weaver import MergeError
from weaver.machine import PhysicalDevice, Machine

modules_under_test = [weaver.machine]

class TestMachine(unittest.TestCase):
    def test_init(self):
        pass

    def test_word_size(self):
        machine = Machine()
        self.assertEqual(machine.word_size, 32)
        self.assertEqual(machine.sizeof_word, 4)
        self.assertEqual(machine.sizeof_pointer, 4)

        machine.word_size = 64
        self.assertEqual(machine.word_size, 64)
        self.assertEqual(machine.sizeof_word, 8)
        self.assertEqual(machine.sizeof_pointer, 8)

        # We don't accept non 8-byte word sizes
        try:
            machine.word_size = 63
        except AssertionError:
            pass
        else:
            self.assertEqual(0, 1)

    def test_kernel_heap_proximity(self):
        machine = Machine()
        # By default it should be 64MB as is the arm requirement.
        self.assertEqual(machine.kernel_heap_proximity, 64*1024*1024)

        # We can set it to a different value
        machine.kernel_heap_proximity = 32*1024*1024
        self.assertEqual(machine.kernel_heap_proximity, 32*1024*1024)

        # Assigning a value of None should keep it the same
        machine.kernel_heap_proximity = None
        self.assertEqual(machine.kernel_heap_proximity, 32*1024*1024)


    def test_page_size(self):
        machine = Machine()
        # An arbitrary set of pages, in arbitrary order
        sizes = [1024, 64*1024, 1024*1024, 4*1024]

        machine.set_page_sizes(sizes)
        self.assertEqual(machine.min_page_size(), min(sizes))
        self.assertEqual(machine.max_page_size(), max(sizes))

        for n in range(1, 32):
            # Natural alignment with powers of two.
            size = 2**n
            align = machine.natural_alignment(size)
            if size <= 1024:
                self.assertEqual(align, 1024)
            elif size > 1024*1024:
                self.assertEqual(align, 1024*1024)
            else:
                self.assertEqual(align, size)

            # Natural alignment with powers of two plus 1.
            size = 2**n + 1
            align = machine.natural_alignment(size)
            if size <= 1024:
                self.assertEqual(align, 1024)
            elif size > 1024*1024:
                self.assertEqual(align, 1024*1024)
            else:
                self.assertEqual(align, size - 1)

            # Natural alignment with powers of two minus 1.
            size = 2**n - 1
            align = machine.natural_alignment(size)
            if size <= 1024:
                self.assertEqual(align, 1024)
            elif size > 1024*1024:
                self.assertEqual(align, 1024*1024)
            else:
                # We're rounding down, so...
                self.assertEqual(align, (size + 1)/2)

        for n in range(1, 32):
            for size in [2**n - 1, 2**n, 2**n + 1]:
                # superpage alignment with powers of two.
                size = 2**n
                align = machine.superpage_alignment(size)
                if size <= 1024:
                    # min page boundary
                    self.assertEqual(align, 1024)
                elif size < 4*1024:
                    self.assertEqual(align, 1024)
                elif size < 64*1024:
                    self.assertEqual(align, 4*1024)
                elif size < 1024*1024:
                    self.assertEqual(align, 64*1024)
                else:
                    self.assertEqual(align, 1024*1024)

    def test_cache_policies(self):
        machine = Machine()

        policies = [("test_1", 100),
                    ("test_2", 200),
                    ("test_3", 300)]
        machine.add_cache_policies(policies)
        for name, val in policies:
            self.assertEqual(machine.get_cache_policy(name), val)
        self.assertRaises(MergeError, machine.get_cache_policy, "not_exist")

    def test_virtual_mem(self):
        machine = Machine()

        mem = [(0, 0x1000, None),
               (0x1000, 0x2000, None),
               (0x4000, 0x8000, None)]
        machine.add_virtual_mem("test_mem", mem)

        overlap = [(0x800, 0x1800, None)]
        self.assertRaises(MergeError, machine.add_virtual_mem, "overlap", overlap)

        self.assertEqual(machine.get_virtual_memory("test_mem"), mem)
        self.assertRaises(MergeError, machine.get_virtual_memory, "not_exists")

    def test_physical_mem(self):
        machine = Machine()

        mem = [(0, 0x1000, None),
               (0x1000, 0x2000, None),
               (0x4000, 0x8000, None)]
        machine.add_physical_mem("test_mem", mem)

        overlap = [(0x800, 0x1800, None)]
        self.assertRaises(MergeError, machine.add_physical_mem, "overlap", overlap)

        self.assertEqual(machine.get_physical_memory("test_mem"), mem)
        self.assertRaises(MergeError, machine.get_physical_memory, "not_exists")

    def test_physical_dev(self):
        machine = Machine()

        mem = [(0, 0x1000, None),
               (0x1000, 0x2000, None),
               (0x4000, 0x8000, None)]
        machine.add_physical_mem("test_mem", mem)


        dev = machine.add_phys_device("test_device")
        dev_mem = [(0x10000, 0x11000, None),
                   (0x11000, 0x12000, None),
                   (0x14000, 0x18000, None)]
        dev.add_physical_mem("dev_mem", dev_mem)
        self.assertEquals(machine.get_physical_memory("dev_mem"), dev_mem)
                
class TestPhysicalDevice(unittest.TestCase):

    def test_init(self):
        name = "test_device"
        dev = PhysicalDevice(name)
        self.assertEqual(dev.name, name)
        self.assertEqual(dev.physical_mem, {})
        self.assertEqual(dev.interrupt, {})

    def test_physical_mem(self):
        name = "test_device"
        dev = PhysicalDevice(name)

        self.assertEqual(dev.physical_mem, {})

        dev.add_physical_mem("test_mem", 10)
        self.assertEqual(dev.get_physical_mem("test_mem"), 10)
        self.assertEqual(dev.physical_mem, {"test_mem": 10})

        self.assertEqual(dev.get_physical_mem("not_exist"), None)

    def test_interrupts(self):
        name = "test_device"
        dev = PhysicalDevice(name)

        self.assertEqual(dev.interrupt, {})

        dev.add_interrupt("test_int", 10)
        self.assertEqual(dev.get_interrupt("test_int"), 10)
        self.assertEqual(dev.interrupt, {"test_int": 10})

        self.assertRaises(MergeError, dev.get_interrupt, "not_exist")
