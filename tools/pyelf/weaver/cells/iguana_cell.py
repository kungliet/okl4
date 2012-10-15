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
Support class for the iguana cell.
"""

import os
from elf.core import UnpreparedElfFile
from elf.constants import ET_EXEC, PF_R, PF_W, PF_X

from weaver import MergeError
from weaver.cells import Cell
from weaver.cells.iguana.extension_xml import get_symbol, \
     collect_extension_element
from weaver.cells.iguana.memobjs_xml import create_alias_cap
from weaver.cells.iguana.device import PhysicalDevice
from weaver.cells.iguana.prog_pd_xml import collect_environment_element, \
     collect_thread, Environment_el, collect_program_pd_elements, \
     Program_el, PD_el, Environment_el
import weaver.cells.iguana.bootinfo
from weaver.memobjs_xml import Stack_el
from weaver.cells.iguana.memobjs_xml import collect_memsection_element, \
     create_standard_caps
from weaver.ezxml import Element, bool_attr, str_attr, long_attr, \
     size_attr, ParsedElement
from weaver.segments_xml import Segment_el, Patch_el, Heap_el, \
     collect_elf_segments, collect_patches
from weaver.cell_environment import CellEnvironment

Extension_el = Element("extension", Segment_el, Patch_el,
        name     = (str_attr, "required"),
        file     = (str_attr, "optional"),
        start    = (str_attr, "optional"),
        direct   = (bool_attr, "optional"),
        pager    = (str_attr, "optional"),
        physpool = (str_attr, "optional"))

IRQ_el = Element("irq",
                 value = (long_attr, "required"))

Iguana_el = Element("iguana", Segment_el, Patch_el, Extension_el,
                    Environment_el, Stack_el, Heap_el, Program_el,
                    PD_el, IRQ_el,
                    name     = (str_attr, "required"),
                    file     = (str_attr, "required"),
                    physpool = (str_attr, "optional"),
                    virtpool = (str_attr, "optional"),
                    pager    = (str_attr, "optional"),
                    direct   = (bool_attr, "optional"),
                    kernel_heap = (size_attr, "required"),
                    caps     = (long_attr, "optional"),
                    clists   = (long_attr, "optional"),
                    spaces   = (long_attr, "optional"),
                    mutexes  = (long_attr, "optional"),
                    threads  = (long_attr, "optional"),
                    priority = (long_attr, "optional"),
                    platform_control = (bool_attr, "optional"))

class IguanaCell(Cell):
    #pylint: disable-msg=R0913
    """
    Cell for iguana programs, pds, drivers and other matters.
    """

    element = Iguana_el

    def __init__(self):
        Cell.__init__(self)
        self.namespace = None
        self.bootinfo = weaver.cells.iguana.bootinfo.BootInfo()
        self.env = None
        self.api_version = None
        self.elf_segments = []
        self.elf_prog_segments = None
        self.space = None
        self.name = None
        self.s_namespace = None

    def collect_xml(self, iguana_el, ignore_name, namespace, machine,
                    pools, kernel, image):
        """Handle an Iguana Server Compound Object"""
        cell = \
             kernel.register_cell(iguana_el.name,
                                  iguana_el.kernel_heap,
                                  max_caps = getattr(iguana_el, "caps", None))

        # New namespace for objects living in the root program's PD.
        ig_namespace = namespace.add_namespace(iguana_el.name)
        self.namespace = ig_namespace
        self.s_namespace = self.namespace
        self.name = iguana_el.name

        self.space = \
                   cell.register_space(self.namespace, "MAIN",
                                       is_privileged = True,
                                       max_clists = getattr(iguana_el,
                                                            "clists", None),
                                       max_spaces = getattr(iguana_el,
                                                            "spaces", None),
                                       max_mutexes = getattr(iguana_el,
                                                             "mutexes", None),
                                       max_threads = getattr(iguana_el,
                                                             "threads", None),
                                       plat_control = \
                                       getattr(iguana_el,
                                               "platform_control",
                                               False))
        self.setup_bootinfo_pools(namespace, pools)
        self.setup_device_namespace(namespace, machine)


        self.env = CellEnvironment(iguana_el.name, self.namespace,
                                   machine, image, kernel,
                                   self.space.mappings)
        cell.env = self.env

        pd = weaver.cells.iguana.bootinfo.RootServerPD(iguana_el.name,
                                                       ig_namespace)

        # Declare the default memory pools.
        def_virtpool = getattr(iguana_el, "virtpool",
                               pools.get_default_virtual_pool())
        def_physpool = getattr(iguana_el, "physpool",
                               pools.get_default_physical_pool())
        def_pager    = getattr(iguana_el, "pager", None)
        def_direct   = getattr(iguana_el, "direct", None)

        # Record any IRQs that are assigned to Iguana
        for irq_el in iguana_el.find_children("irq"):
            self.space.register_irq(irq_el.value)


        self.bootinfo.set_system_default_attrs(def_virtpool,
                                               def_physpool,
                                               image,
                                               def_pager,
                                               def_direct)

        # Iguana is not aware of segment ids.
        # The old mapping API uses segment 0 for all mapping ops.
        # Segment 0 needs to cover all of physical memory.
        # Subsequent segments will be created but never used,
        # but must still be mapped!

        # XXX: VVVVV THIS IS PURE EVIL VVVVV
        physpool_attrs = \
                       image.new_attrs(self.s_namespace.add_namespace("physpool_hack"))
        physpool_attrs.phys_addr = 0 #first_phys_base
        physpool_attrs.size = 0xfffff000 #first_phys_end
        physpool_attrs.attach = PF_R | PF_W | PF_X
        physpool_attrs.cache_policy = 0xff # XXX: Define me properly
        physpool_attrs.mem_type = physpool_attrs.unmapped
        self.space.register_mapping(physpool_attrs)
        # XXX: ^^^^^ THIS IS PURE EVIL ^^^^^

        filename = os.path.join(iguana_el._path, iguana_el.file)
        elf = UnpreparedElfFile(filename=filename)

        pd.set_default_pools(image, self.bootinfo)

        # Collect the object environment
        pd.add_env_ms(image, ig_namespace, machine, pools)
        env, extra_ms = \
             collect_environment_element(iguana_el.find_child('environment'),
                                         ig_namespace, machine, pools,
                                         image, self.bootinfo)

        segment_els = iguana_el.find_children("segment")
        segs = collect_elf_segments(elf,
                                    image.ROOT_PROGRAM,
                                    segment_els,
                                    filename,
                                    [],
                                    ig_namespace,
                                    image,
                                    machine,
                                    pools)

        self.elf_prog_segments = segs
        for seg in segs:
            self.space.register_mapping(seg.attrs)

        if elf.elf_type != ET_EXEC:
            raise MergeError, "All the merged ELF files must be of EXEC type."

        # Find out which version of libokl4 that iguana was built
        # against
        sym = elf.find_symbol("okl4_api_version")
        if sym == None:
            raise MergeError("Unable to locate the symbol 'okl4_api_version' "
                             "in file \"%s\".  Cells must link with libokl4." %
                             filename)

        self.api_version = elf.get_value(sym.value, sym.size,
                                         elf.endianess)
        if self.api_version == None:
            raise MergeError("Unable to read the symbol 'okl4_api_version' in "
                             "file \"%s\".  Cells must link with libokl4." %
                             filename)

        # Record any patches being made to the program.
        patch_els = iguana_el.find_children("patch")
        for patch in getattr(Iguana_el, "extra_patches", []):
            addr = get_symbol(elf, patch[0], True)
            if addr == None:
                continue
            addr = int(addr[0])+ int(patch[1])
            new_patch = Patch_el(address=hex(addr), bytes=patch[2],
                                 value=patch[3])
            patch_els.append(new_patch)
        collect_patches(elf, patch_els, filename, image)

        for extension_el in iguana_el.find_children("extension"):
            if not ignore_name.match(extension_el.name):
                collect_extension_element(extension_el,
                                          pd,
                                          ig_namespace,
                                          elf,
                                          image,
                                          machine,
                                          self.bootinfo,
                                          pools)

        # Collect the main thread.  The root program can only have one
        # thread, so this call chiefly is used to collect information
        # about the stack.
        #
        # The stack is not set up as a memsection, so it is not put in the
        # object environment.
        if not hasattr(iguana_el, 'priority'):
            iguana_el.priority = 255
        thread = collect_thread(elf, iguana_el, ignore_name, ig_namespace,
                                image, machine, pools, self.space,
                                entry = elf.entry_point,
                                name = 'iguana',
                                namespace_thread_name = "main",
                                cell_create_thread = True)
        pd.add_thread(thread)

        # Collect the heap.  Is there no element, create a fake one for
        # the collection code to use.
        #
        # The heap is not set up as a memsection, so it is not put in the
        # object environment.
        heap_el = iguana_el.find_child('heap')

        if heap_el is None:
            heap_el = ParsedElement('heap')

        heap_ms = collect_memsection_element(heap_el, ignore_name,
                                             ig_namespace, image,
                                             machine, pools)
        pd.attach_heap(heap_ms)

        self.space.register_mapping(heap_ms.ms.attrs)
        self.space.register_mapping(thread.get_stack().get_ms().attrs)

        self.elf_segments.extend([pd.env_ms.get_ms(),
            thread.get_stack().get_ms(), heap_ms.get_ms()] +
            [ms.get_ms() for ms in extra_ms])

        pd.add_environment(env)
        pd.utcb_size = image.utcb_size
        self.bootinfo.add_rootserver_pd(pd)

        # And now parse the programs and pd's
        collect_program_pd_elements(iguana_el, ignore_name,
                                    ig_namespace, image, machine,
                                    self.bootinfo, pools, kernel,
                                    cell)

    #def generate_dynamic_segments(self, namespace, machine, pools,
    #                              kernel, image):
    def generate_dynamic_segments(self, _, machine, pools, kernel, image):
        """
        Create  bootinfo segment and environment buffers.
        """
        self.bootinfo.create_dynamic_segments(self, self.namespace, image,
                                              kernel, machine, pools,
                                              self.bootinfo)
        self.space.register_mapping(self.bootinfo.ms.attrs)
        self.space.utcb = self.bootinfo.utcb
        self.env.generate_dynamic_segments(self, image, machine, pools)
        self.elf_segments.append(self.env.memsect)
        self.group_elf_segments(image)

    def generate_init(self, machine, pools, kernel, image):
        """
        Generate the bootinfo script for the cell, placing it into a
        segment.
        """
        self.bootinfo.generate(image, kernel, machine, self.bootinfo)
        self.env.generate_init(machine, pools, kernel, image)

    def get_cell_api_version(self):
        """
        Return the libOKL4 API versions that the cell initial program
        was build against.
        """
        return self.api_version

    # This used to be done in pools_xml.py but since bootinfo has become part
    # of the iguana cell we need to do it here
    def setup_bootinfo_pools(self, namespace, pools):
        """
        Map elfweaver pools into iguana pools.
        """
        for pool in pools.get_virtual_pools():
            boot_pool = \
                      weaver.cells.iguana.bootinfo.VirtPool(pool.get_name(),
                                                            pool)
            self.bootinfo.add_virtpool(boot_pool)

            # New namespace for the memory pool's caps.
            new_namespace = namespace.add_namespace(pool.get_name())
            if pool.get_name() == "direct":
                master = weaver.cells.iguana.bootinfo.Cap("master", ["master"])
                boot_pool.add_cap(master)
                new_namespace.add(master.get_name(), master)
            else:
                # Add the standard caps for the pool.
                create_standard_caps(boot_pool, new_namespace)

        for pool in pools.get_physical_pools():
            boot_pool = \
                      weaver.cells.iguana.bootinfo.PhysPool(pool.get_name(),
                                                            pool)
            self.bootinfo.add_physpool(boot_pool)
            # New namespace for the memory pool's caps.
            new_namespace = namespace.add_namespace(pool.get_name())

            # Add the standard caps for the pool.
            create_standard_caps(boot_pool, new_namespace)

    def setup_device_namespace(self, namespace, machine):
        """
        Populate the /dev namespace with Iguana aliases to the
        physical devices.
        """
        dev_ns = namespace.root.add_namespace("dev")

        for device in machine.physical_device.values():
            create_alias_cap(PhysicalDevice(device), dev_ns)

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

IguanaCell.register()
