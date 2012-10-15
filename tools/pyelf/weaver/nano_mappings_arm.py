##############################################################################
# Copyright (c) 2008 Open Kernel Labs, Inc. (Copyright Holder).
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

from elf.ByteArray import ByteArray
from StringIO import StringIO
import struct

class Mappings(object):
    """ Mappings class to generate pagetable for ARM. """
    def __init__(self):
        self._mappings = []
        self.decoded_mappings = []

    def add_mapping(self, virt, phys, pgsize, npages, rwx, cache):
        rwx_lookup = {
                0x0 : 0x1, # rwx
                0x1 : 0x2, # rwX
                0x2 : 0x3, # rWx
                0x3 : 0x3, # rWX
                0x4 : 0x2, # Rwx
                0x5 : 0x2, # RwX
                0x6 : 0x3, # RWx
                0x7 : 0x3, # RWX
                }
        self._mappings.append((virt, phys, pgsize, npages,
                               rwx_lookup[rwx], cache))
        #print "map %x -> %x (%x bytes)" % (virt, phys, pgsize*npages)

    def decode_level1_entry(self, entry, section):
        """
        Decode a first level ARM pagetable entry.
        These can either be 1M mappings or subtree entries.
        Return a flag indicating if it is a subtree or not and
        an n-tuple describing it.
        """
        first_level_mask = 0x13
        entry_1m = 0x12
        entry_subtree = 0x11

        virt = section * 1024 * 1024
        if entry & first_level_mask == entry_1m:
            phys = (((entry & 0xfff00000) >> 20)) * 1024 * 1024
            pgsize = 0x100000
            rwx = (entry & 0x1c00) >> 10 # XXX: rwx needs reverse lookup?
            cache = (entry & 0xc) >> 2

            is_subtree = False
            ret = (phys, virt, pgsize, rwx, cache)

        elif entry & first_level_mask == entry_subtree:
            phys = entry & ~0x3ff

            is_subtree = True
            ret = (phys, virt, None, None, None)
        else:
            return False, None

        return is_subtree, ret

    def decode_subtree(self, data, base_section):
        """
        Decode an ARM pagetable subtree.
        These can either be 4K or 64K entries.
        Return a list of mapping n-tuples.
        """
        mappings = []
        section_size = 1024
        second_level_mask = 0x3
        entry_4k = 0x2
        entry_64k = 0x1

        index = 0
        while index < section_size:
            section = index / 4
            entry = struct.unpack("<I", data[index:index+4])[0]

            index += 4
            if not entry:
                continue

            if entry & second_level_mask == entry_4k:
                pgsize = 0x1000
            elif entry & second_level_mask == entry_64k:
                pgsize = 0x10000
                index += 15 * 4 # 64k entry is 16 identical entries, skip them

            phys = ((entry & 0xfffff000) >> 12) * 4 * 1024
            virt = ((section * 4096) + (base_section * 1024 * 1024))
            rwx = (entry & 0x70) >> 4 # XXX: rwx needs reverse lookup?
            cache = (entry & 0xc) >> 2

            mappings.append((phys, virt, pgsize, rwx, cache))

        return mappings

    def decode(self, data, base_addr, entries = None):
        max_offset = 0
        section_size = 16 * 1024

        for index in range(0, section_size, 4):
            section = index / 4
            entry = struct.unpack("<I", data[index:index+4])[0]
            if not entry:
                continue

            is_subtree, mapping = self.decode_level1_entry(entry, section)

            if not is_subtree: # 1MB mapping
                self.decoded_mappings.append(mapping)
            else: # Subtree
                offset = mapping[0] - base_addr
                if offset > max_offset:
                    max_offset = offset

                sub_mappings = self.decode_subtree(data[offset:], section)
                if sub_mappings:
                    self.decoded_mappings.extend(sub_mappings)

        return max_offset + 1024

    def print_decoded_mappings(self):
        f = StringIO()
        f.write("\tPHYS         VIRT         PGSIZE       RWX  CACHE\n")

        for m in self.decoded_mappings:
            f.write("\t%#-10x   %#-10x   %#-10x   %#x   %#x\n" % \
                    (m[0], # phys
                     m[1], # virt
                     m[2], # pgsize
                     m[3], m[4])) # rwx, cache

        return f

    def to_data(self, phys_addr):
        # Map a 1M section
        def set_section(array, index, pfn, rwx, cache):
            entry = (pfn << 20) | (rwx << 10) | (cache << 2) | 0x12
            #print "set  1M entry %x = %x (%x, %x, %x)" % (index, entry, pfn, rwx, cache)
            assert array.get_data(index*4, 4, '<') == 0
            array.set_data(index*4, entry, 4, '<')

        # Map a 4K page
        def set_page(array, offset, index, pfn, rwx, cache):
            entry = (pfn << 12) | (rwx << 10) | (rwx << 8) | (rwx << 6) | \
                    (rwx << 4) | (cache << 2) | 0x2
            #print "set  4K entry %x = %x (%x, %x, %x)" % (index, entry, pfn, rwx, cache)
            assert array.get_data(offset+index*4, 4, '<') == 0
            array.set_data(offset+index*4, entry, 4, '<')

        # Map a 64K page
        def set_large_page(array, offset, index, pfn, rwx, cache):
            entry = (pfn << 12) | (rwx << 10) | (rwx << 8) | (rwx << 6) | \
                    (rwx << 4) | (cache << 2) | 0x1
            for i in range(index, index+16):
                #print "set 64K entry %x = %x (%x, %x, %x)" % (index, entry, pfn, rwx, cache)
                assert array.get_data(offset+i*4, 4, '<') == 0
                array.set_data(offset+i*4, entry, 4, '<')

        # Create a subtree to support 4k pages
        # will extend pagetable array as neccesary, returns an offset into pagetable array
        # which is start of 2nd level pagetable
        # will return old subtree if one already exists here
        def create_subtree(array, index, phys_addr):
            existing = array.get_data(index*4, 4, '<')
            # if we have already created a subtree here
            if existing & 0x3 == 1:
                return (existing & 0xfffffc00) - phys_addr
            assert (existing & 0x3 == 0)
            start = len(pagetable)
            array.extend(1024*[0])
            entry = ((phys_addr + start) & ~0x3ff) | 0x11
            #print "create subtree %x = %x (%x)" % (index, entry, phys_addr + start)
            array.set_data(index*4, entry, 4, '<')
            return start

        pagetable = ByteArray(16*1024*"\0")
        # Sort by virtual
        self._mappings.sort(key=lambda x: x[0])

        for virt, phys, pgsize, npages, rwx, cache in self._mappings:
            if virt == 0:
                #print "Warning mapping at 0 - reserving 1K of space which may not be needed"
                pagetable.extend(1024*[0])
                continue
            #print "map %x -> %x (%x bytes)" % (virt, phys, pgsize*npages)
            section_index = virt / 1024 / 1024
            num_sections = ((npages*pgsize - 1) / 1024 / 1024) + 1
            for section in range(section_index, section_index+num_sections):
                if pgsize == 0x00100000: # 1MB
                    set_section(pagetable, section,
                                phys / 1024 / 1024 + section-section_index,
                                rwx, cache)
                elif pgsize == 0x00010000: # 64KB
                    subtree = create_subtree(pagetable, section, phys_addr)
                    page_index = (virt - (section * 1024 * 1024)) / 4096
                    for page in range(page_index, page_index + npages):
                        set_large_page(pagetable, subtree, page,
                                 phys / 4096 + page - page_index, rwx, cache)
                else: # 4KB
                    subtree = create_subtree(pagetable, section, phys_addr)
                    page_index = (virt - (section * 1024 * 1024)) / 4096
                    for page in range(page_index, page_index + npages):
                        set_page(pagetable, subtree, page,
                                 phys / 4096 + page - page_index, rwx, cache)

        return pagetable
