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

"""
Support class for a cell that contains a single okl4 program.
"""
import os

from elf.core import UnpreparedElfFile
from elf.constants import ET_EXEC, PF_R, PF_W, PF_X
from elf.util import align_up

from weaver import MergeError
from weaver.cells import Cell
from weaver.memobjs_xml import Memsection_el, Stack_el, collect_memobj_attrs
from weaver.ezxml import Element, str_attr, bool_attr, long_attr, \
     size_attr, ParsedElement
from weaver.segments_xml import Patch_el, Heap_el, Segment_el, \
        collect_patches, collect_elf_segments, start_to_value, \
        make_pager_attr
from weaver.cell_environment import CellEnvironment
from weaver.cap import MutexCellCap, ThreadCellCap, PhysSegCellCap, \
     PhysAddrCellCap
from weaver.segments_xml import attach_to_elf_flags

UseDevice_el = Element("use_device",
                       name = (str_attr, "required"))

Thread_el = Element("thread", Stack_el,
                    name     = (str_attr, "required"),
                    start    = (str_attr, "required"),
                    priority = (long_attr, "optional"),
                    physpool = (str_attr, "optional"),
                    virtpool = (str_attr, "optional"))

Mutex_el = Element("mutex",
                   name = (str_attr, "required"))

Entry_el = Element("entry",
                        key    = (str_attr, "required"),
                        value  = (long_attr, "optional"),
                        cap    = (str_attr, "optional"),
                        attach = (str_attr, "optional"))

Environment_el = Element("environment", Entry_el)

IRQ_el = Element("irq",
                 value = (long_attr, "required"))

Space_el = Element("space", Mutex_el, Thread_el, Memsection_el, IRQ_el,
                   name     = (str_attr, "required"),
                   physpool = (str_attr, "optional"),
                   virtpool = (str_attr, "optional"),
                   pager    = (str_attr, "optional"),
                   max_priority = (long_attr, "optional"),
                   direct   = (bool_attr, "optional"))

Arg_el = Element("arg",
                 value      = (str_attr, "required"))

CommandLine_el = Element("commandline", Arg_el)

OKL4_el = Element("okl4", Patch_el, Memsection_el, Stack_el, Heap_el,
                  Segment_el, Space_el, IRQ_el, Environment_el, Mutex_el,
                  Thread_el, UseDevice_el, CommandLine_el,
                  name             = (str_attr, "required"),
                  file             = (str_attr, "required"),
                  physpool         = (str_attr, "optional"),
                  virtpool         = (str_attr, "optional"),
                  pager            = (str_attr, "optional"),
                  direct           = (bool_attr, "optional"),
                  kernel_heap      = (size_attr, "required"),
                  caps             = (long_attr, "optional"),
                  clists           = (long_attr, "optional"),
                  spaces           = (long_attr, "optional"),
                  mutexes          = (long_attr, "optional"),
                  threads          = (long_attr, "optional"),
                  priority         = (long_attr, "optional"),
                  max_priority     = (long_attr, "optional"),
                  platform_control = (bool_attr, "optional"))

DEFAULT_STACK_SIZE = 0x1000
DEFAULT_HEAP_SIZE = 0x10000
DEFAULT_CLIST_BASE = 0x70000000
DEFAULT_UTCB_BASE = 0x60000000

class OKL4Cell(Cell):
    # disable: Too many arguments
    # pylint: disable-msg=R0913
    """
    Cell for iguana programs, pds, drivers and other matters.
    """

    element = OKL4_el

    def __init__(self):
        Cell.__init__(self)
        self.name = ""
        self.heap_ms = None
        self.stack_ms = None
        self.utcb_ms = None
        self.elf = None
        self.api_version = None
        self.elf_segments = []
        self.elf_prog_segments = None
        self.def_virtpool = None
        self.def_physpool = None
        self.phys_addr = None
        self.virt_addr = None
        self.phys_attrs = None
        self.virt_attrs = None
        self.env = None
        self.cell = None
        self.namespace = None
        self.space = None

    def _collect_environment(self, env_el, env):
        """
        Collect the details of the environmen element.
        """

        # Collect any custom entries in the environment.
        if env_el is not None:
            for entry_el in env_el.find_children('entry'):
                cap_name    = None
                attach      = None

                if hasattr(entry_el, 'value'):
                    env.add_value_entry(entry_el.key, entry_el.value)
                else:
                    if not hasattr(entry_el, 'cap'):
                        raise MergeError, 'Value or cap attribute required.'

                    cap_name = entry_el.cap

                    if hasattr(entry_el, 'attach'):
                        attach = attach_to_elf_flags(entry_el.attach)

                    env.add_cap_entry(entry_el.key,
                                      cap_name = cap_name,
                                      attach = attach)


    def collect_xml(self, okl4_el, ignore_name, namespace, machine,
                    pools, kernel, image):
        """Handle an Iguana Server Compound Object"""
        self.cell = \
             kernel.register_cell(okl4_el.name,
                                  okl4_el.kernel_heap,
                                  max_caps = getattr(okl4_el, "caps", None),
                                  max_priority = getattr(okl4_el, "max_priority", None))
        self.name = okl4_el.name
        self.namespace = namespace.add_namespace(self.name)
        self.space = \
                   self.cell.register_space(self.namespace, "MAIN",
                                is_privileged = True,
                                max_clists = getattr(okl4_el,
                                                     "clists", None),
                                max_spaces = getattr(okl4_el,
                                                     "spaces", None),
                                max_mutexes = getattr(okl4_el,
                                                      "mutexes", None),
                                max_threads = getattr(okl4_el,
                                                      "threads", None),
                                max_priority = getattr(okl4_el,
                                                       "max_priority", None),
                                plat_control = \
                                            getattr(okl4_el,
                                                    "platform_control", False))

        image.push_attrs(
            virtual  = getattr(okl4_el, "virtpool", None),
            physical = getattr(okl4_el, "physpool", None),
            pager    = make_pager_attr(getattr(okl4_el, "pager", None)),
            direct   = getattr(okl4_el, "direct", None))

        (self.def_virtpool, self.def_physpool) = image.current_pools()

        self.collect_mutexes(okl4_el, self.namespace, self.space)

        env_el = okl4_el.find_child("environment")
        self.env = CellEnvironment(okl4_el.name, self.namespace,
                                   machine, image, kernel, self.space.mappings)

        if env_el != None:
            self._collect_environment(env_el, self.env)

        # Set these up now even though we can't actually assign values
        # till later
        self.phys_attrs = image.new_attrs(self.namespace.add_namespace("default_physpool"))
        self.phys_attrs.attach = PF_R | PF_W | PF_X
        self.phys_attrs.mem_type = self.phys_attrs.unmapped
        mapping = self.space.register_mapping(self.phys_attrs)
        self.env.add_physmem_segpool_entry("MAIN_PHYSMEM_SEGPOOL", mapping)

        self.virt_attrs = image.new_attrs(self.namespace.add_namespace("default_virtpool"))
        self.env.add_virtmem_pool_entry("MAIN_VIRTMEM_POOL", self.virt_attrs)

        self.space.utcb = image.new_attrs(self.namespace.add_namespace("main_utcb_area"))
        self.space.utcb.attach = PF_R

        filename = os.path.join(okl4_el._path, okl4_el.file)
        self.elf = UnpreparedElfFile(filename=filename)

        if self.elf.elf_type != ET_EXEC:
            raise MergeError("All the merged ELF files must be of EXEC type.")

        # Find out which version of libokl4 that the cell was built
        # against
        sym = self.elf.find_symbol("okl4_api_version")
        if sym == None:
            raise MergeError("Unable to locate the symbol 'okl4_api_version' in file \"%s\".  Cells must link with libokl4." % filename)

        self.api_version = self.elf.get_value(sym.value, sym.size,
                                              self.elf.endianess)
        if self.api_version == None:
            raise MergeError("Unable to read the symbol 'okl4_api_version' in file \"%s\".  Cells must link with libokl4." % filename)

        self.env.add_elf_info_entry(os.path.basename(okl4_el.file),
                image.PROGRAM, self.elf.entry_point)

        segment_els = okl4_el.find_children("segment")
        segs = collect_elf_segments(self.elf, image.PROGRAM, segment_els,
                                    filename, [], self.namespace, image,
                                    machine, pools)
        self.elf_prog_segments = segs

        for seg in segs:
            self.env.add_elf_segment_entry(okl4_el.name + '.' + seg.attrs.ns_node.name,
                    seg.segment)
            seg_ns = seg.attrs.ns_node
            mapping = self.space.register_mapping(seg.attrs)
            self.add_standard_mem_caps(seg_ns, mapping, seg.attrs)

        patch_els = okl4_el.find_children("patch")
        collect_patches(self.elf, patch_els, filename, image)

        # Record any IRQs that are assigned to the initial program.
        for irq_el in okl4_el.find_children("irq"):
            self.space.register_irq(irq_el.value)
        self.env.add_device_irq_list("NO_DEVICE",
                                     [irq_el.value for irq_el \
                                      in okl4_el.find_children("irq")])

        # Collect the implicit thread
        if not hasattr(okl4_el, 'priority'):
            okl4_el.priority = kernel.kernel.MAX_PRIORITY

        threads = []
        threads.append(self.collect_thread(self.elf, okl4_el, self.namespace,
                image, machine, pools, kernel, self.space, self.elf.entry_point,
                "main", True))

        # FIXME: Need to check up on actual entry point's
        for thread_el in okl4_el.find_children("thread"):
            threads.append(self.collect_thread(self.elf, thread_el,
                    self.namespace, image, machine, pools, kernel,
                    self.space, "thread_start", cell_create_thread = True))

        device_mem = \
                   self.collect_use_devices(okl4_el, self.space,
                                            self.namespace, image,
                                            machine, pools)

        memsection_objs = \
                    self.collect_memsections(okl4_el, self.space,
                            self.namespace, image, machine, pools)

        # Collect all data for any extra spaces defined in the XML
        for space_el in okl4_el.find_children("space"):
            space_ns = self.namespace.add_namespace(space_el.name)
            space = self.cell.register_space(space_ns, space_el.name,
                        max_priority = getattr(space_el, "max_priority", \
                                       getattr(okl4_el, "max_priority", None)))

            image.push_attrs(
                virtual  = getattr(space_el, "virtpool", None),
                physical = getattr(space_el, "physpool", None),
                pager    = make_pager_attr(getattr(space_el, "pager", None)),
                direct   = getattr(space_el, "direct", None))

            for thread_el in space_el.find_children("thread"):
                threads.append(self.collect_thread(self.elf, thread_el,
                        space_ns, image, machine, pools, kernel, space,
                        "thread_start", cell_create_thread = True))

            self.collect_mutexes(space_el, space_ns, space)

            device_mem.extend(
                self.collect_use_devices(space_el, space,
                                         space_ns, image, machine, pools))

            memsection_objs.extend(
                self.collect_memsections(space_el, space, space_ns, image,
                                    machine, pools))

            self.env.add_kspace_entry(space_el.name + "_KSPACE", space)

            space.utcb = image.new_attrs(space_ns.add_namespace(space_el.name + "_utcb_area"))
            space.utcb.attach = PF_R

        # Weave a kclist for the main space.
        main_kclist = self.env.add_kclist_entry("MAIN_KCLIST", self.cell)

        # Weave the root kspace object.
        main_kspace = self.env.add_kspace_entry("MAIN_KSPACE", self.space)

        # Weave the root protection domain object.
        self.env.add_pd_entry("MAIN_PD", self.space, main_kspace, 32,
                machine.min_page_size(), self.elf)

        # Find heap and add it
        heap_el = okl4_el.find_child('heap')
        if heap_el is None:
            heap_el = ParsedElement('heap')

        heap_attr = collect_memobj_attrs(heap_el, self.namespace,
                                         image, machine)

        if heap_attr.size == None:
            heap_attr.size = DEFAULT_HEAP_SIZE

        self.heap_ms = image.add_memsection(heap_attr, machine, pools)
        self.cell.heap = self.heap_ms.attrs
        mapping = self.space.register_mapping(self.heap_ms.attrs)

        self.add_standard_mem_caps(heap_attr.ns_node,
                                   mapping, heap_attr)

        self.elf_segments = \
                [thread.get_stack_ms() for thread in threads] + \
                [self.heap_ms] + memsection_objs + device_mem

        self.env.add_kernel_info_entry(0, 0, kernel)

        # Add command line arguments
        commandline_el = okl4_el.find_child("commandline")

        if commandline_el is not None:
            args = [arg_el.value for arg_el in commandline_el.find_children("arg")]
        else:
            args = []

        self.env.add_arg_list(args)

        self.cell.env = self.env

    def get_cell_api_version(self):
        """
        Return the libOKL4 API versions that the cell initial program
        was build against.
        """
        return self.api_version

    def generate_dynamic_segments(self, namespace, machine, pools, kernel,
                                  image):
        """
        Create  bootinfo segment and environment buffers.
        """
        utcb_mss = []
        for space in self.cell.spaces:
            space.utcb.size = align_up(space.max_threads *
                                       image.utcb_size,
                                       machine.min_page_size())
            # A space with no threads will get a 0 size as align_up doesn't
            # change a 0, so we explicity set it to at least one page
            if space.utcb.size == 0:
                space.utcb.size = machine.min_page_size()
            utcb_ms = image.add_utcb_area(space.utcb)
            utcb_mss.append(utcb_ms)

            kspace = None
            for (x, _) in self.env.space_list:
                if x.space.id == space.id:
                    kspace = x
            if image.utcb_size:
                self.env.add_utcb_area_entry("UTCB_AREA_%d" % space.id,
                                             space, kspace, image)

        self.env.add_bitmap_allocator_entry("MAIN_SPACE_ID_POOL",
                                            *self.cell.space_list)
        self.env.add_bitmap_allocator_entry("MAIN_CLIST_ID_POOL",
                                            *self.cell.cap_list)
        self.env.add_bitmap_allocator_entry("MAIN_MUTEX_ID_POOL",
                                            *self.cell.mutex_list)
        self.env.add_int_entry("MAIN_SPACE_ID", self.space.id)
        self.env.add_int_entry("MAIN_CLIST_ID", self.cell.clist_id)

        self.env.generate_dynamic_segments(self, image, machine, pools)

        self.elf_segments.extend(utcb_mss + [self.env.memsect])
        self.group_elf_segments(image)

    def generate_init(self, machine, pools, kernel, image):
        """
        Generate the bootinfo script for the cell, placing it into a
        segment.
        """

        self.set_free_pools(pools, kernel, image, machine)

        self.env.generate_init(machine, pools, kernel, image)

    def set_free_pools(self, pools, kernel, image, machine):
        phys_free = \
            pools.get_physical_pool_by_name(self.def_physpool).get_freelist()[:]
        virt_free = \
            pools.get_virtual_pool_by_name(self.def_virtpool).get_freelist()[:]

        # Sort biggest to smallest
        phys_free.sort(key=lambda x: x[0] - x[1])
        virt_free.sort(key=lambda x: x[0] - x[1])

        # Extract the biggest regions and remove them from the lists.
        (phys_base, phys_end, _) = phys_free[0]
        del phys_free[0]
        (virt_base, virt_end, _) = virt_free[0]
        del virt_free[0]

        self.phys_attrs.size = phys_end - phys_base + 1
        self.phys_attrs.phys_addr = phys_base
        def_phys_ms = image.add_memsection(self.phys_attrs, machine, pools)

        image.add_group(0, [def_phys_ms])

        self.virt_attrs.size = virt_end - virt_base + 1
        self.virt_attrs.virt_addr = virt_base

        self.env.add_kernel_info_entry(phys_base,
                                       phys_end,
                                       kernel)

    def collect_thread(self, elf, el, namespace, image, machine,
                       pools, kernel, space, entry,
                       thread_name = None,
                       cell_create_thread = False):
        """Collect the attributes of a thread element."""
        if entry is None:
            raise MergeError, "No entry point specified for thread %s" % el.name

        user_main = getattr(el, 'start', entry)

        entry = start_to_value(entry, elf)
        user_main = start_to_value(user_main, elf)

        priority = getattr(el, 'priority', kernel.kernel.DEFAULT_PRIORITY)
        physpool = getattr(el, 'physpool', None)
        virtpool = getattr(el, 'virtpool', None)

        # New namespace for objects living in the thread.
        if thread_name == None:
            thread_name = el.name
        thread_namespace = namespace.add_namespace(thread_name)

        # Push the overriding pools for the thread.
        image.push_attrs(virtual = virtpool,
                         physical = physpool)

        utcb = image.new_attrs(thread_namespace.add_namespace("utcb"))
        # Create the cell thread and assign the entry point
        thread = space.register_thread(entry, user_main, utcb,
                                       priority,
                                       create = cell_create_thread)

        thread_namespace.add('master', ThreadCellCap('master', thread))

        # Collect the stack.  Is there no element, create a fake one for
        # the collection code to use.
        stack_el = el.find_child('stack')
        if stack_el is None:
            stack_el = ParsedElement('stack')

        stack_attr = collect_memobj_attrs(stack_el, thread_namespace,
                                          image, machine)

        if stack_attr.size == None:
            stack_attr.size = DEFAULT_STACK_SIZE

        stack_ms = image.add_memsection(stack_attr, machine, pools)
        mapping = space.register_mapping(stack_ms.attrs)

        self.add_standard_mem_caps(stack_attr.ns_node,
                                   mapping, stack_attr)

        # Setup the stack for the new cell thread
        thread.stack = stack_ms.attrs
        thread.stack_ms = stack_ms

        # If this is the very first collect_thread call, we assume it is
        # the cell's main thread and we set the stack_ms accordingly.
        if self.stack_ms is None:
            self.stack_ms = stack_ms

        image.pop_attrs()

        return thread

    def collect_use_devices(self, el, space, namespace, image,
                            machine, pools):
        device_mem = []

        for device_el in el.find_children("use_device"):
            dev = machine.get_phys_device(device_el.name)

            # Potentially we can have multiple named physical mem sections
            # each with multiple ranges.
            for key in dev.physical_mem.keys():
                ranges = dev.physical_mem[key]
                index = 0
                for (base, size, rights, cache_policy) in ranges:
                    # If theres only one range just use the key otherwise
                    # append an index to distingiush entries
                    if len(ranges) == 1:
                        name = key
                    else:
                        name = key + '_' + str(index)
                        index += 1

                    device_ns = namespace.add_namespace(name)
                    attrs = image.new_attrs(device_ns)
                    attrs.attach = PF_R | PF_W

                    if cache_policy is not None:
                        attrs.cache_policy = machine.get_cache_policy(cache_policy)

                    attrs.phys_addr = base
                    attrs.size = size

                    device_ms = image.add_memsection(attrs, machine, pools)
                    device_mem.append(device_ms)

                    mapping = space.register_mapping(device_ms.attrs)
                    self.add_standard_mem_caps(device_ns, mapping,
                                               device_ms.attrs)

            for irq in dev.interrupt.values():
                space.register_irq(irq)
            self.env.add_device_irq_list(dev.name, dev.interrupt.values())

        return device_mem

    def collect_mutexes(self, el, namespace, space):
        for mutex_el in el.find_children("mutex"):
            m_ns = namespace.add_namespace(mutex_el.name)
            mutex = space.register_mutex(mutex_el.name)
            m_ns.add('master', MutexCellCap('master', mutex))

    def add_standard_mem_caps(self, namespace, mapping, attrs):
        namespace.add('master', PhysSegCellCap('master', mapping))
        namespace.add('physical', PhysAddrCellCap('physical', attrs))

    def collect_memsections(self, el, space, namespace, image, machine,
                            pools):

        memsection_objs = []

        for memsection_el in el.find_children('memsection'):
            memsection_attr = \
                        collect_memobj_attrs(memsection_el, namespace,
                                             image, machine)
            memsection_ns = memsection_attr.ns_node
            memsection_ms = image.add_memsection(memsection_attr, machine,
                                                 pools)
            memsection_objs.append(memsection_ms)
            mapping = space.register_mapping(memsection_ms.attrs)
            self.add_standard_mem_caps(memsection_ns, mapping,
                                       memsection_ms.attrs)

        return memsection_objs

    def group_elf_segments(self, image):
        """
        Group ELF segments together in a way that avoids domain faults
        and reduces final image size.
          - Any memsection that is under 1M goes into Group 1.
          - Any memsection above 1M but has contents, Group 2.
          - Any memsection above 1M which is empty, Group 3.
        """
        groups = [[], [], []]

        for seg in self.elf_segments:
            if seg.get_attrs().size <= 0x100000:
                idx = 0
            elif seg.get_attrs().data or seg.get_attrs().file:
                idx = 1
            else:
                idx = 2

            groups[idx].append(seg)

        for grp in groups:
            grp.sort(lambda x, y: int(x.get_attrs().size - y.get_attrs().size))

            if grp is groups[0]:
                grp = self.elf_prog_segments + grp
            image.add_group(None, grp)


OKL4Cell.register()

