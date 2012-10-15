###############################################################################
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

"""
This module provides the interface to printing the elfweaver-initialised
nano structures.
"""

from sys import stdout
import struct
from elf.constants import EM_ARM
from weaver.util import arch_import, get_symbol_value
from StringIO import StringIO
from weaver.memstats_model import NanoHeap

class NanoStructures(object):
    """Class to print the contents of nano's kernel heap."""

    MAX_PRIORITY = 31

    elf_machine_lookup = {EM_ARM: "arm"}

    def tcb_addr_to_thread_num(self, thread_addr):
        """Get TID of a thread specified by its TCB address."""
        return int((thread_addr - self.tcb_base_addr) / self.tcb.tcb_size())

    def get_thread_by_addr(self, addr):
        """Get thread n-tuple of a thread specified by its TCB address."""
        tid = self.tcb_addr_to_thread_num(addr)
        for thread in self.thread_table:
            if thread[0] == tid:
                return thread
        return None

    def decode_threads(self, data):
        """
        Store an n-tuple describing each thread in the thread_table.
        Keep a count of the spare threads while we are here.
        """
        size = self.tcb.tcb_size()
        start = 0
        end = size
        for _ in range(self.num_threads):
            thread = data[start:end]
            halted = self.tcb.get_tcb_thread_state(thread)
            if halted:
                self.spare_threads += 1
            else:
                thread = (self.tcb.get_tcb_tid(thread),
                          self.tcb.get_tcb_priority(thread),
                          self.tcb.get_tcb_pc(thread),
                          self.tcb.get_tcb_sp(thread),
                          self.tcb.get_tcb_r0(thread),
                          self.tcb.get_tcb_prev(thread),
                          self.tcb.get_tcb_next(thread))
                self.thread_table.append(thread)
            start += size
            end += size

        return self.num_threads * size

    def print_live_threads(self):
        """
        Print out details of all live threads in the system.
        """
        f = StringIO()
        f.write("\tTID   PRIO    IP           SP            r0\n")

        for thread in self.thread_table:
            f.write("\t %d     %d     %#-10x   %#-10x    %#-10x\n" % \
                    (thread[0], thread[1],
                     thread[2],
                     thread[3],
                     thread[4]))
        return f


    def decode_priority_table(self, data):
        """
        prio_list is indexed by priority level.
        Each level stores a list of TIDs.
        """

        for prio in range(self.MAX_PRIORITY+1):
            index = prio * (self.wordsize/8)
            addr = struct.unpack(self.format_str,
                                 data[index:index+(self.wordsize/8)])[0]
            thread = self.get_thread_by_addr(addr)
            if thread:
                first_tid = thread[0]
                queue = [first_tid]
                # Follow the 'next' pointer until we get back to the start
                while self.tcb_addr_to_thread_num(thread[6]) != first_tid:
                    thread = self.get_thread_by_addr(thread[6])
                    queue.append(thread[0])
                self.prio_list[prio] = queue

        return (self.wordsize/8) + (self.MAX_PRIORITY+1)

    def print_priority_table(self):
        """Print the priority table in descending order."""
        f = StringIO()

        # We want to print priorities in descending order
        i = self.MAX_PRIORITY
        for entry in reversed(self.prio_list):
            if entry:
                f.write("\tPRIORITY %2d: " % (i-1))
                f.write("\t%d" % entry[0])
                for tid in entry[1:]:
                    f.write(" --> %d" % tid)
                f.write("\n")
            i -= 1

        return f

    def __init__(self, elf, section):
        self.section = section
        self.elf = elf
        self.wordsize = elf.wordsize
        self.format_str = "%s%s" % \
                (elf.endianess, ["I", "Q"][elf.wordsize==64])

        self.tcb = arch_import(self.elf_machine_lookup, "nano_tcb", elf.machine)
        self.map = arch_import(self.elf_machine_lookup, "nano_mappings", elf.machine)
        self.mappings = self.map.Mappings()

        self.kpagetable_phys = get_symbol_value(self.elf, "kpagetable_phys")
        self.pgtable_size = 0

        self.num_threads = get_symbol_value(self.elf, "max_tcbs")
        self.tcb_base_addr = get_symbol_value(self.elf, "tcbs")
        self.tcb_size = 0
        self.spare_threads = 0
        self.thread_table = []

        self.prio_list = [[] for _ in range(self.MAX_PRIORITY+1)]
        self.priority_table_size = 0

        self.futex_hash_slots = get_symbol_value(self.elf, "futex_hash_slots")
        self.futex_hash = get_symbol_value(self.elf, "futex_hash")
        self.futex_pending_tags = get_symbol_value(self.elf,
                                                   "futex_pending_tags")

    def decode(self, memstats = None):
        offset = 0
        data = self.section._data

        self.pgtable_size = self.mappings.decode(data, self.kpagetable_phys)
        offset += self.pgtable_size

        self.tcb_size = self.decode_threads(data[offset:])
        offset += self.tcb_size

        self.priority_table_size = self.decode_priority_table(data[offset:])
        offset += self.priority_table_size

        if memstats is not None:
            heap = NanoHeap(memstats,
                            self.kpagetable_phys,
                            self.tcb_size,
                            self.pgtable_size,
                            self.priority_table_size,
                            self.futex_hash_slots * (self.elf.wordsize / 8))
            # using self.kpagetable_phys as the start of the heap
            # to map to phys section.
            heap.process_mappings(self.mappings.decoded_mappings)
            heap.process_threads(self.thread_table, self.spare_threads)
            heap.process_futexes(self.futex_hash_slots)

    def output(self, f=stdout):
        """Print contents of nano's kernel heap."""

        # Decode everything before we start printing
        self.decode()

        print >> f, "Nano Initialised Structures:"

        pgtable = self.mappings.print_decoded_mappings()
        print >> f, "\nPage Table:"
        print >> f, "\tSize: %#x Bytes" % self.pgtable_size
        print >> f, "\tkpagetable_phys: %#x\n" % self.kpagetable_phys
        print >> f, pgtable.getvalue()
        pgtable.close()

        print >> f, "\nTCB:"
        print >> f, "\tSize: %#x Bytes\n" % self.tcb_size
        print >> f, "\tLive Threads:"
        live_threads = self.print_live_threads()
        print >> f, live_threads.getvalue()
        print >> f, "\tSpare Threads: %d" % self.spare_threads
        print >> f, "\tTotal Threads: %d\n" % self.num_threads
        live_threads.close()

        priority_table = self.print_priority_table()
        print >> f, "Scheduler Queue:"
        print >> f, "\tSize: %#x Bytes" % self.priority_table_size
        print >> f, priority_table.getvalue()
        priority_table.close()

        print >> f, "Futex Structures:"
        print >> f, "\tFutex Hash Slots: %d" % self.futex_hash_slots
        print >> f, "\tFutex Hash Address: %#x" % self.futex_hash
        print >> f, "\tFutex Pending Tags Address: %#x" % \
                     self.futex_pending_tags

def find_nano_heap(elf):
    """
    Find the kernel heap and return an object
    capable of describing its contents.
    """
    structures = elf.find_section_named("kernel.heap")

    if structures:

        structures = NanoStructures(elf, structures)

    return structures
