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

"""Collection of items that will initialise a Nano kernel."""

from math import log
import struct
from copy import copy
from elf.constants import PF_R, PF_W
from elf.util import align_up, align_down
from elf.ByteArray import ByteArray
import weaver.machine
from weaver import MergeError
from weaver.util import _0, next_power_of_2, get_symbol, arch_import

NANO_KERNEL_API_VERSION = 0x80000000

class NanoKernel(object):
    """
    Class which contains kernel specific methods for the Nano kernel.
    """
    # These objects will default to these sizes if not specified
    DEFAULT_KERNEL_HEAP_SIZE = 32 * 1024
    DEFAULT_KERNEL_MAX_THREADS = 32

    # These objects cannot exceed these sizes
    ABSOLUTE_MAX_THREADS = DEFAULT_KERNEL_MAX_THREADS
    MAX_PRIORITY = 31
    DEFAULT_PRIORITY = 16

    THREAD_STATE_HALTED = 0xffffffff
    THREAD_STATE_RUNNABLE = 0

    arch_lookup = {"xscale" : "arm",
                   "arm926ejs" : "arm",
                   "arm1136js" : "arm",
                   }
    heap_proximity_map = {"arm": 64 * 1024}

    def allocate_memory(self, memory_array, base_addr, size):
        """Allocate 'size' bytes in the given memory array. Return
        the address allocated and the updated base address."""
        memory_array += ByteArray('\0' * size)
        return base_addr, base_addr + size

    def thread_num_to_tcb_addr(self, base_addr, idx):
        ret = base_addr + (idx * self.tcb.tcb_size())
        assert ((ret >> 3) & 1) == 0
        return ret

    def init_empty_thread(self, id, base_addr, prev, next):
        # Initialise a new, empty thread.
        tcb = ByteArray([0] * self.tcb.tcb_size())
        self.tcb.set_tcb_tid(tcb, id)

        # Setup thread state to halted
        self.tcb.set_tcb_thread_state(tcb, self.THREAD_STATE_HALTED)

        if self.arch_lookup[self.cpu] == 'arm':
            self.tcb.set_tcb_cpsr(tcb, 0x10)

        self.tcb.set_tcb_next(tcb, self.thread_num_to_tcb_addr(base_addr, next))
        self.tcb.set_tcb_prev(tcb, self.thread_num_to_tcb_addr(base_addr, prev))

        # Not initially registered for any interrupts
        self.tcb.set_tcb_registered_interrupt(tcb, 0xffffffff)

        return tcb

    def init_thread(self, id, thread, base_addr, prev, next):
        # Create a blank thread
        tcb = self.init_empty_thread(id, base_addr, prev, next)

        # Setup thread state to runnable
        self.tcb.set_tcb_thread_state(tcb, self.THREAD_STATE_RUNNABLE)

        # Setup default registers
        self.tcb.set_tcb_priority(tcb, thread[1])
        self.tcb.set_tcb_pc(tcb, thread[2])
        self.tcb.set_tcb_sp(tcb, thread[3])
        self.tcb.set_tcb_r0(tcb, thread[4])

        return tcb

    def get_thread_data(self, base_addr, live_threads, num_spare_threads):
        # Sort the threads into reverse priority order.
        live_threads.sort(lambda x, y: cmp(y[1], x[1]))

        # Setup empty scheduling queue data structures.
        priority_bitfield = 0L
        plist = [0 for i in range(self.MAX_PRIORITY+1)]
        threads_at_priority = [[] for i in range(self.MAX_PRIORITY+1)]

        # Iterate through and create scheduler priority list and bitmap.
        for i, thread in enumerate(live_threads):
            priority = thread[1]
            threads_at_priority[priority].append((i, priority))
            if plist[priority] == 0L:
                priority_bitfield |= (1<<priority)
                plist[priority] = self.thread_num_to_tcb_addr(base_addr, i)

        # For each thread, determine who is next on the scheduler queue.
        tcb_prev_next = {}
        for plevel in threads_at_priority:
            i = 0
            for entry in plevel:
                prev = plevel[(i - 1) % len(plevel)][0]
                next = plevel[(i + 1) % len(plevel)][0]
                tcb_prev_next[entry[0]] = (prev, next)
                i += 1

        # Iterate through and create the TCBs.
        thread_data = ByteArray()
        num_threads = 0
        for i, thread in enumerate(live_threads):
            prev, next = tcb_prev_next[i]
            thread_data += self.init_thread(i, thread, base_addr, prev, next)
            num_threads += 1

        # Pack scheduling data
        priority_table = ByteArray()
        for pdata in plist:
            priority_table += ByteArray(struct.pack(self.format_str, pdata))

        # Create the spare threads.
        first_spare_thread_num = num_threads
        for id in range(num_spare_threads):
            prev = first_spare_thread_num + ((id - 1) % num_spare_threads)
            next = first_spare_thread_num + ((id + 1) % num_spare_threads)
            thread_data += self.init_empty_thread(num_threads, base_addr, prev, next)
            num_threads += 1
        first_spare_thread_addr = self.thread_num_to_tcb_addr(base_addr, first_spare_thread_num)

        return thread_data, priority_table, priority_bitfield, first_spare_thread_addr

    def create_mapping(self, attrs, remaining_size, page_size, is_minimum):
        """
        Map as many pages for the specified page size.
        Return the mapping n-tuple and the size of the mapping.
        If page size is the minimum page size, we need to worry about
        some extra checks.
        """

        # If minimum page size, we need to consider some extra conditions
        if is_minimum:
            phys_addr = align_down(_0(attrs.phys_addr), page_size)
            virt_addr = align_down(_0(attrs.virt_addr), page_size)

            # Calculate the shift cause by aligning the phys_addr
            alignment_diff = 0
            if attrs.phys_addr is not None:
                alignment_diff = attrs.phys_addr - phys_addr

            size = 0
            num_pages = 0
            if attrs.size != None:
                # In certain cases, phys alignments can leave us a
                # page short. To account for this we add alignment
                # differences to the size.
                size = align_up(remaining_size + alignment_diff, page_size)
                num_pages = size / page_size

        # for all other pages, we map as many as we can
        else:
            phys_addr = _0(attrs.phys_addr)
            virt_addr = _0(attrs.virt_addr)
            size = 0

            if attrs.size != None:
                num_pages = remaining_size / page_size
                size = num_pages * page_size

        #print "ceating mapping: size %x, pages %x" % (size, num_pages)
        mapping = (virt_addr, phys_addr, page_size, num_pages,
                   attrs.attach, attrs.cache_policy)
        return mapping, size

    def get_mappings(self, _attrs, page_sizes, minimum):
        """
        We run through all valid page sizes and try to map with a
        page size as large as possible.
        We return a list of mapping n-tuples.
        """

        # We are going to be changing attrs, so copy it first
        attrs = copy(_attrs)
        mappings = []
        # Find biggest page size we can use
        attrs.size = _0(attrs.size)
        for page_size in page_sizes:
            if attrs.size == 0:
                break
            if page_size > attrs.size and page_size != minimum:
                continue
            else:
                #print "phys_addr %x mapping size %x, page size %x" % \
                #        (_0(attrs.phys_addr), attrs.size, page_size)
                mapping, ret_size = self.create_mapping(attrs, attrs.size,
                                                        page_size,
                                                        page_size==minimum)
                # Ensure phys addr is aligned to page size
                assert(mapping[1] % page_size == 0)
                mappings.append(mapping)
                attrs.size -= ret_size
                if attrs.phys_addr:
                    attrs.phys_addr += ret_size
                if attrs.virt_addr:
                    attrs.virt_addr += ret_size

        return mappings

    def largest_page(self, vaddr, size, page_sizes):
        valid_sizes = [x for x in page_sizes if x >= size and vaddr+x <= 2**self.wordsize]
        assert(len(valid_sizes) > 0)
        return valid_sizes[0]

    # NanoKernel __init__
    def __init__(self, elf, namespace, machine):
        self.kernel_data = ByteArray([])
        self.kobjects_ms = None
        self.namespace = namespace
        self.wordsize = elf.wordsize
        self.format_str = "%s%s" % \
                (elf.endianess, ["I", "Q"][elf.wordsize/8==64])
        self.machine_init(machine)
        self.tcb = arch_import(self.arch_lookup, "nano_tcb", self.cpu)
        self.map = arch_import(self.arch_lookup, "nano_mappings", self.cpu)
        self.patches = []
        self.segment_drops = ["elfweaver"]
        self.devices = []

    def machine_init(self, machine):
        self.cpu = machine.cpu
        if self.cpu not in self.arch_lookup:
            raise MergeError("Cpu %s not supported by nano kernel" % self.cpu)

        machine.set_kernel_heap_proximity(self.heap_proximity_map[self.arch_lookup[self.cpu]])

    def layout_cells_pre(self, kernel):
        pass
    def layout_cells_post(self, kernel, image):
        pass

    def init_kernel_pool(self, machine, pools):
        """
        Went want to reserve top 1MB of virtual for the kernel,
        but the top 68KB is comprised of 64KB kernel segments and
        4KB helper page.
        """
        pools.new_virtual_pool("kernel", machine)
        pools.add_virtual_memory("kernel", machine,
                base = 0xFFF00000, size = 1024*1024 - 64*1024 - 4096)

    def add_device_mem(self, dev, image, machine, pools):
        this_device_mem = []
        # Naming format: <name>_dev
        name = dev.name.split('_')[0]

        # Ensure keys are sorted
        for key in sorted(dev.physical_mem.keys()):
            # XXX: Assume there is only one region in each key
            assert (len(dev.get_physical_mem(key)) == 1)
            (base, size, rights, cache_policy) = dev.get_physical_mem(key)[0]

            attrs = image.new_attrs(self.namespace.add_namespace("%s_mem%d" % (name, len(this_device_mem))))
            attrs.attach = PF_R | PF_W
            attrs.virtpool = "kernel"
            attrs.phys_addr = base
            attrs.size = size
            attrs.cache_policy = machine.get_cache_policy(cache_policy)

            ms = image.add_memsection(attrs, machine, pools)
            # index <N> into array corresponds to "device_mem<N>"
            this_device_mem.append(ms)

        self.devices.append((name, this_device_mem))

    def create_dynamic_segments(self, kernel, namespace, image, machine,
                                pools, base_segment):
        data = self.create_ops(kernel, None, machine)

        kernel.heap_attrs.size   = align_up(len(data),
                                            machine.min_page_size())

    def create_ops(self, kernel, elf, machine):
        self.patches = []
        kernel_data = ByteArray()
        data_vaddr = kernel.base_segment.segment.vaddr + \
                (_0(kernel.heap_attrs.phys_addr) - kernel.base_segment.segment.paddr)
        start_data_vaddr = data_vaddr
        mappings = self.map.Mappings()

        total_kernel_size = (_0(kernel.heap_attrs.phys_addr) + \
                             kernel.heap_attrs.size) - \
                             kernel.base_segment.segment.paddr
        page_size = self.largest_page(kernel.base_segment.segment.vaddr,
                                      total_kernel_size,
                                      machine.page_sizes)
        mappings.add_mapping(kernel.base_segment.segment.vaddr,
                kernel.base_segment.segment.paddr,
                page_size, 1, 0x0, weaver.machine.CACHE_POLICIES['default'])

        # hacked for arm at the moment
        if self.arch_lookup[self.cpu] == 'arm':
            # user helpers page
            if elf:
                user_helpers_initial_vaddr = elf.find_symbol("user_atomic_cmp_set").value
            else:
                user_helpers_initial_vaddr = 0
            user_paddr = user_helpers_initial_vaddr - kernel.base_segment.segment.vaddr + \
                kernel.base_segment.segment.paddr
            #print "user_helpers %x -> %x" % (kernel.base_segment.segment.vaddr-4096, user_paddr)
            mappings.add_mapping(kernel.base_segment.segment.vaddr - 4096,
                    user_paddr, 4096, 1, 0x1, weaver.machine.CACHE_POLICIES['default'])

        # Format: (name, prio, entry, sp, r0)
        # TODO: Give main thread cell_environment in r0
        threads = []

        # Collect live thread info
        for (name, cell) in kernel.cells.items():
            for space in cell.spaces:
                for thread in space.threads:
                    threads.append(("nanotest",
                                    thread.priority,
                                    thread.entry,
                                    thread.get_sp(),
                                    cell.get_mr1())) # cell env pointer
                for num, name, attrs in space.mappings:
                    if not attrs.need_mapping():
                        continue
                    #print "Mapping for (%d) %s:" % (num, name)
                    for m in self.get_mappings(attrs, machine.page_sizes,
                                               machine.min_page_size()):
                        mappings.add_mapping(*m)

        # Add initial mapping
        if kernel.base_segment.segment.paddr != kernel.base_segment.segment.vaddr:
            mappings.add_mapping(kernel.base_segment.segment.paddr,
                    kernel.base_segment.segment.paddr,
                1024 * 1024, 1, 0x0, weaver.machine.CACHE_POLICIES['default'])

        # Kernel driver mappings
        for (name, mem) in self.devices:
            # Patch variables
            if elf:
                # Ensure enough kernel pointers are defined
                base_name = "%s_mem" % name
                driver_max_index = len(mem) - 1
                i = 0
                while elf.find_symbol("%s%d" % (base_name, i)):
                    i += 1
                kernel_max_index = i - 1

                if driver_max_index != kernel_max_index:
                    raise MergeError, "%s driver: "\
                            "Kernel expected %d memory range%s, "\
                            "driver supplied %d" % \
                            (name, kernel_max_index+1,
                             ["s", ""][kernel_max_index==0],
                             driver_max_index+1)

            # Add mappings
            for (i, ms) in enumerate(mem):
                for m in self.get_mappings(ms.attrs, machine.page_sizes,
                                           machine.min_page_size()):
                    mappings.add_mapping(*m)
                self.patches.append(("%s_mem%d" % (name, i),
                                    _0(ms.attrs.virt_addr)))

        # Generate pagetable
        pagetable = mappings.to_data(_0(kernel.heap_attrs.phys_addr))

        # pagetable needs to be 16k aligned, so it must be the first thing in the kernel_data
        kernel_data += pagetable
        data_vaddr += len(pagetable)

        # Number of threads defaults to the defined constant, if less are
        # specifed, the difference will be allocated as spare threads
        assert(kernel.total_threads <= self.ABSOLUTE_MAX_THREADS)
        num_spare_threads = self.DEFAULT_KERNEL_MAX_THREADS - len(threads)
        num_threads = self.DEFAULT_KERNEL_MAX_THREADS

        thread_data, priority_table, priority_bitfield, free_thread_addr = \
                self.get_thread_data(data_vaddr, threads, num_spare_threads)
        tcb_data_vaddr = data_vaddr
        kernel_data += thread_data
        data_vaddr += len(thread_data)

        priority_table_addr = data_vaddr
        kernel_data += priority_table
        data_vaddr += len(priority_table)

        # Futexes
        futex_base_addr = data_vaddr
        futex_hash_slots = next_power_of_2((num_threads * 3) / 2)

        futex_hash_addr, futex_base_addr = \
                self.allocate_memory(kernel_data, futex_base_addr,
                                futex_hash_slots * 8)
        futex_pending_tags_addr, futex_base_addr = \
                self.allocate_memory(kernel_data, futex_base_addr, num_threads * 4)

        self.patches.extend(
                [("tcbs", tcb_data_vaddr),
                 ("max_tcbs", num_threads),
                 ("tcb_free_head", free_thread_addr),
                 ("priority_bitmap", priority_bitfield),
                 ("priority_heads", priority_table_addr),
                 ("futex_hash_slots", futex_hash_slots),
                 ("futex_hash_slots_lg2", int(log(futex_hash_slots, 2))),
                 ("futex_hash", futex_hash_addr),
                 ("futex_pending_tags", futex_pending_tags_addr),
                 ("kpagetable_phys", _0(kernel.heap_attrs.phys_addr)),
                 ])

        return kernel_data

    def generate_init_script(self, kernel, elf, image, machine):
        data = self.create_ops(kernel, elf, machine)

        # All these symbols *must* exist
        may_not_exist = False
        for p in self.patches:
            (addr, size) = get_symbol(elf, p[0], may_not_exist)
            image.patch(addr, size, p[1])

        dlen = len(data)
        assert(dlen <= kernel.heap_attrs.size)

        kernel.heap.segment.sections[0]._data[:dlen] = data

    def update_elf(self, kernel, elf, image, machine):
        pass

    def get_base_segment(self, elf, segs, image):
        """Return the base segment."""
        return segs[0]


from weaver import *
