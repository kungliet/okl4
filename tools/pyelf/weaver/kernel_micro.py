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

"""Collection of items that will initialise a Micro kernel."""

from math import log
from StringIO import StringIO
from elf.constants import PF_R
from elf.util import align_up, align_down
from elf.ByteArray import ByteArray
from weaver import MergeError
from weaver.kernel_micro_elf import InitScriptHeader, InitScriptCreateHeap, \
        InitScriptInitIds, InitScriptCreateThreadHandles, \
        InitScriptCreateClist, InitScriptCreateSpace, InitScriptCreateThread, \
        InitScriptMapMemory, InitScriptCreateSegment, InitScriptCreateMutex, \
        InitScriptAssignIrq, InitScriptAllowPlatformControl, \
        InitScriptCreateMutexCap, InitScriptCreateIpcCap
from weaver.kernel_micro_elf import KernelConfigurationSection, find_elfweaver_info
from weaver.kernel_xml import get_symbol
from weaver.util import _0

MICRO_KERNEL_API_VERSION = 0x2

class MicroKernel(object):
    """
    Class which contains kernel specific methods for the Micro kernel.
    """

    # These objects will default to these sizes if not specified
    DEFAULT_KERNEL_HEAP_SIZE = 4 * 1024 * 1024
    DEFAULT_KERNEL_MAX_THREADS = 1024

    # These objects cannot exceed these sizes
    ABSOLUTE_MAX_THREADS = 4092
    MAX_PRIORITY = 255
    DEFAULT_PRIORITY = 100

    # MicroKernel __init__
    def __init__(self, namespace):
        self.init_ms       = None
        self.namespace = namespace
        self.segment_drops = []
        self.devices = []

    def layout_cells_pre(self, kernel):
        # This is called before dynamic elements of cells as we should
        # be able to decide the number of cells, clists and mutexes at that
        # time
        pass


    def layout_cells_post(self, kernel, image):
        # This must be called after image.layout() and before the cell
        # init scripts are written out.
        #
        # Do any required layout here

        heap_phys_base = kernel.heap_attrs.phys_addr

        for cell in kernel.cells.values():
            cell.heap_phys_base = heap_phys_base
            heap_phys_base += cell.heap_size

            assert heap_phys_base <= kernel.heap_attrs.phys_addr + \
                    kernel.heap_attrs.size

            for space in cell.get_static_spaces():
                utcb_base = space.utcb.virt_addr
                utcb_size = image.utcb_size
                index = 0
                for thread in space.get_static_threads():
                    thread.utcb.virt_addr = utcb_base + index * utcb_size
                    index += 1

        kernel.thread_array_base = heap_phys_base

        assert heap_phys_base + kernel.thread_array_size <= \
               kernel.heap_attrs.phys_addr + kernel.heap_attrs.size

    def init_kernel_pool(self, machine, pools):
        pass
    def add_device_mem(self, name, ranges, image, machine, pools):
        pass

    def create_dynamic_segments(self, kernel, namespace, image, machine,
                                pools, base_segment):
        """ Create a segment to store the init script. """

        kernel.calc_thread_array_sizes()

        f = self.create_ops(kernel, image, machine)

        attrs = image.new_attrs(namespace.add_namespace("initscript"))
        attrs.attach = PF_R
        attrs.pager  = None
        attrs.size   = align_up(len(f.getvalue()), machine.min_page_size())
        attrs.data   = ByteArray()

        self.init_ms = image.add_memsection(attrs, machine, pools)
        image.add_group(machine.kernel_heap_proximity, (base_segment, self.init_ms))

        f.close()

    def create_ops(self, kernel, image, machine):
        """ Create in init script for Micro kernel initialisation. """

        op_list = []
        offset  = [0]

        def add_op(op_func, *args):
            op = op_func(None, None, None, (args), image, machine)
            op_list.append(op)

            my_offset  = offset[0]
            offset[0] += op.sizeof()

            return my_offset

        f = StringIO()

        # We just use the cells in order, hopefully the first cell has a
        # large enough heap for soc/kernel.  No longer do sorting
        cells = kernel.cells.values()

        ## PHASE ONE ##
        add_op(InitScriptHeader, [])

        add_op(InitScriptCreateHeap, _0(cells[0].heap_phys_base),
               cells[0].heap_size)

        # Declare total sizes.  The must be a minimum of 1.
        add_op(InitScriptInitIds, max(kernel.total_spaces, 1),
               max(kernel.total_clists, 1),
               max(kernel.total_mutexes, 1))

        needs_heap = False

        add_op(InitScriptCreateThreadHandles, _0(kernel.thread_array_base),
               kernel.thread_array_count)
        op_list[-1].set_eop()

        ## PHASE TWO ##
        for cell in cells:
            # No need to encode the heap of the first cell.
            if needs_heap:
                add_op(InitScriptCreateHeap, _0(cell.heap_phys_base),
                       cell.heap_size)

            else:
                needs_heap = True

            cell.clist_offset = \
                              add_op(InitScriptCreateClist,
                                     cell.clist_id, cell.max_caps)

            for space in cell.get_static_spaces():
                utcb_base = 0xdeadbeef # something obvious if we ever use it!
                utcb_size = 0x11

                if space.utcb is not None:
                    utcb_base = space.utcb.virt_addr
                    if utcb_base is None:
                        utcb_base = 0
                        utcb_size = 0
                    else:
                        utcb_size = int(log(space.utcb.size, 2))

                add_op(InitScriptCreateSpace, space.id, space.space_id_base,
                        _0(space.max_spaces), space.clist_id_base,
                        _0(space.max_clists), space.mutex_id_base,
                        _0(space.max_mutexes), space.max_phys_segs,
                        utcb_base, utcb_size, space.is_privileged,
                        #XXX: A space's max priority is currently hardcoded!
                        #XXX: For now, use the kernel's max priority instead.
                        self.MAX_PRIORITY)
                        #space.max_priority)

                # Grant the space access to the platform control
                # system call.
                if space.plat_control:
                    add_op(InitScriptAllowPlatformControl, [])

                # Assign any irqs to the space.
                for irq in space.irqs:
                    add_op(InitScriptAssignIrq, irq)

                for thread in space.get_static_threads():
                    # FIXME: Need to deal with entry and user_start
                    thread.offset = \
                                  add_op(InitScriptCreateThread,
                                         thread.cap_slot,
                                         thread.priority,
                                         thread.entry, thread.get_sp(),
                                         utcb_base, cell.get_mr1())

                for mutex in space.get_static_mutexes():
                    mutex.offset = \
                                 add_op(InitScriptCreateMutex, mutex.id)

                for (num, name, attrs) in space.mappings:
                    map_pg_sz = machine.min_page_size()
                    map_pg_sz_log2 = int(log(map_pg_sz, 2))

                    phys_addr = align_down(_0(attrs.phys_addr), map_pg_sz)
                    virt_addr = align_down(_0(attrs.virt_addr), map_pg_sz)

                    # Calculate the shift cause by aligning the phys_addr
                    alignment_diff = 0

                    if attrs.has_phys_addr():
                        alignment_diff = attrs.phys_addr - phys_addr

                    size = 0
                    num_pages = 0
                    if attrs.size != None:
                        # In certain cases, phys alignments can leave us a
                        # page short. To account for this we add alignment
                        # differences to the size.
                        size = align_up(attrs.size + alignment_diff, map_pg_sz)
                        num_pages = size / map_pg_sz

                    # Attributes are 0xff => All cache policies are valid!
                    if attrs.has_phys_addr():
                        add_op(InitScriptCreateSegment, num, phys_addr,
                               0xff, size, attrs.attach)

                    if attrs.need_mapping():
                        add_op(InitScriptMapMemory, num, 0,
                                attrs.attach, map_pg_sz_log2, num_pages,
                                attrs.cache_policy, virt_addr)


        # Dump any caps
        for cell in cells:
            for space in cell.get_static_spaces():
                for cap in space.ipc_caps:
                    add_op(InitScriptCreateIpcCap, _0(cap.clist.clist_offset),
                           _0(cap.cap_slot), cap.obj.offset)

                for cap in space.mutex_caps:
                    add_op(InitScriptCreateMutexCap, _0(cap.clist.clist_offset),
                           _0(cap.cap_slot), cap.obj.offset)


        op_list[-1].set_eop()

        f.write(''.join([op.encode() for op in op_list]))
        return f

    def generate_init_script(self, kernel, elf, image, machine):
        f = self.create_ops(kernel, image, machine)

        dlen = len(f.getvalue())
        assert(dlen <= self.init_ms.get_attrs().size)

        self.init_ms.segment.sections[0]._data[:dlen] = ByteArray(f.getvalue())
        f.close()

    def update_elf(self, kernel, elf, image, machine):
        """Update the kernel configuration page in the ELF file."""
        kconfig_sect = elf.find_section_named(kernel.section)

        if kconfig_sect is None:
            raise MergeError, "Couldn't find roinit section"

        kconfig_sect = KernelConfigurationSection(elf, kconfig_sect)
        kconfig_sect.set_cellinit_addr(self.init_ms.segment.paddr)
        kconfig_sect.update_data()

    def get_base_segment(self, elf, segs, image):
        """Return the base segment, check for XIP and patch accordingly."""

        if get_symbol(elf, '__phys_addr_rom', True) != None:
            (addr, size) = get_symbol(elf, '__phys_addr_ram')
            image.patch(addr, size, segs[1])
            base_segment = segs[1]

            (addr, size) = get_symbol(elf, '__phys_addr_rom')
            image.patch(addr, size, segs[0])

            if get_symbol(elf, '__phys_addr_stack', True) != None:
                stack_sym = elf.find_symbol('__stack')
                if stack_sym is not None:
                    offset = stack_sym.value - segs[1].segment.vaddr
                    (addr, size) = get_symbol(elf, '__phys_addr_stack')
                    image.patch(addr, size, segs[1], offset)
        else:
            sdata = get_symbol(elf, '__phys_addr_ram', may_not_exist=True)

            if sdata is not None:
                (addr, size) = sdata
                image.patch(addr, size, segs[0])

            if get_symbol(elf, '__phys_addr_stack', True) != None:
                stack_sym = elf.find_symbol('__stack')
                if stack_sym is not None:
                    offset = stack_sym.value - segs[0].segment.vaddr
                    (addr, size) = get_symbol(elf, '__phys_addr_stack')
                    image.patch(addr, size, segs[0], offset)

            base_segment = segs[0]

        return base_segment

    def get_utcb_size(self, elf, image):
        """
        Return the size, in bytes, of a UTCB entry.

        This value is fetched from the kernel image.
        """
        elfweaverinfo = find_elfweaver_info(elf)
        return elfweaverinfo.utcb_size
