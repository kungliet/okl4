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

"""Environment descriptor for a Cell."""

from StringIO import StringIO
from elf.ByteArray import ByteArray
from elf.constants import PF_R, PF_W
from weaver.cell_environment_elf import CellEnvSection, \
        CellEnvKernelInfo, CellEnvBitmapAllocator, CellEnvSegments, \
        CellEnvSegmentEntry, CellEnvValueEntry, \
        CellEnvCapEntry, CellEnvBitmapAllocator, \
        CellEnvUtcbArea, CellEnvKclist, CellEnvKspace, CellEnvPd, \
        CellEnvKthread, CellEnvVirtmemPool, CellEnvPhysmemSegpool, \
        CellEnvElfSegmentEntry, CellEnvElfInfoEntry, \
        CellEnvDeviceIrqList, CellEnvArgList, CellEnvNanoPd
from weaver.kernel import KERNEL_MICRO, KERNEL_NANO
from weaver.cap import CellCap
from weaver import MergeError
from weaver.kernel_nano import NanoKernel
from weaver.kernel_micro import MicroKernel

class CellEnvironment:
    """
    Environment descriptor for a Cell.
    """
    def __init__(self, progname, namespace, machine, image, kernel, mappings):
        self.namespace = namespace
        self.progname = progname
        self.min_page_size = machine.min_page_size()
        self.machine = machine
        self.image = image
        self.entries = {}
        self.cap_entries = []
        self.memsect = None
        self.mappings = mappings
        self.space_list = []
        self.space_utcb_list = []
        self.pd_list = []
        self.clist = None

        # Determine the type of kernel we need to weave for.
        if isinstance(kernel.kernel, NanoKernel):
            self.kernel_type = KERNEL_NANO
        else:
            self.kernel_type = KERNEL_MICRO

    def _add_entry(self, tag, entry, no_dup = True):
        """Add an entry to the environment."""

        # Raise an error if the entry name is already used.
        if no_dup and tag.upper() in self.entries:
            raise MergeError, \
                  'Key "%s" is already present in environment for %s.' % \
                  (tag, self.namespace.abs_name())

        self.entries[tag.upper()] = entry

    def add_kernel_info_entry(self, base, end, kernel):
        """Add kernel info details to the environment."""

        self._add_entry('OKL4_KERNEL_INFO',
                CellEnvKernelInfo(self.machine, self.image, base, end, kernel),
                no_dup = False)

    def add_bitmap_allocator_entry(self, tag, base = 0,
            size = 0, preallocated = 0):
        """Add bitmap allocator details to the environment."""

        self._add_entry(tag,
                CellEnvBitmapAllocator( self.machine, self.image,
                    base = base, size = size, preallocated = preallocated))

    def add_virtmem_pool_entry(self, tag, attrs):
        """Add virtmem pool details to the environment."""

        self._add_entry(tag,
                CellEnvVirtmemPool(self.machine, self.image, attrs))

    def add_physmem_segpool_entry(self, tag, mapping):
        """Add physmem segpool details to the environment."""

        self._add_entry(tag,
                CellEnvPhysmemSegpool(self.machine, self.image,
                    mapping, self.min_page_size))

    def add_utcb_area_entry(self, tag, space, kspace, image):
        """ Add segment mapping details to the environment. """
        utcb_area = CellEnvUtcbArea(self.machine, self.image, space)
        self._add_entry(tag, utcb_area)
        self.space_utcb_list.append((kspace, utcb_area))

    def add_kclist_entry(self, tag, cell):
        """ Add segment mapping details to the environment """
        kclist = CellEnvKclist(self.machine, self.image, cell)
        self._add_entry(tag, kclist)
        self.clist = kclist
        return kclist

    def add_kthread_entry(self, tag, thread):
        """ Add segment mapping details to the environment """
        self._add_entry(tag,
                CellEnvKthread(self.machine, self.image,
                    thread, self.kernel_type))

    def add_kspace_entry(self, tag, space):
        """ Add segment mapping details to the environment """
        kspace = CellEnvKspace(self.machine, self.image,
                self.kernel_type, space)
        self._add_entry(tag, kspace)
        kthreads = []
        for i, thread in enumerate(space.threads):
            kthread = CellEnvKthread(self.machine, self.image,
                    thread, self.kernel_type)
            self._add_entry("%s_THREAD_%d" % (tag, i), kthread)
            kthreads.append(kthread)
        self.space_list.append((kspace, kthreads))
        return kspace

    def add_pd_entry(self, tag, space, kspace, max_threads, min_page_size, elf):
        """ Add protection domain details to the environment """

        if self.kernel_type == KERNEL_NANO:
            # Weave a Nano PD.
            pd = CellEnvNanoPd(self.machine, self.image, max_threads)
        else:
            # Weave a Micro PD.
            pd = CellEnvPd(self.machine, self.image, space, max_threads,
                    min_page_size, elf)
            self.pd_list.append((pd, kspace))

        # Add the PD to our environment
        self._add_entry(tag, pd)

    def add_value_entry(self, key, value):
        """Add an integer to the environment."""
        self._add_entry(key, CellEnvValueEntry(self.machine, self.image, value))

    def add_elf_segment_entry(self, name, seg):
        """Add info about an elf segment to the environment."""
        self._add_entry('ELFSEG_' + name,
                CellEnvElfSegmentEntry(self.machine, self.image, seg))

    def add_elf_info_entry(self, name, file_type, entry):
        """Add info about an elf file to the environment."""
        self._add_entry('ELFINF_' + name,
                CellEnvElfInfoEntry(self.machine, self.image, file_type, entry))

    def add_cap_entry(self, key, cap_name, attach):
        """Add a cap to the environment."""
        cap_entry = CellEnvCapEntry(self.machine, self.image, cap_name, attach)
        self._add_entry(key, cap_entry)
        self.cap_entries.append(cap_entry)

    def add_device_irq_list(self, name, irqs):
        """Add a device's list of IRQs to the environment."""
        irq_list = CellEnvDeviceIrqList(self.machine,
                self.image, len(irqs), irqs)
        name = name + "_irq_list"
        self._add_entry(name.upper(), irq_list)

    def add_arg_list(self, args):
        """Add the arguments structure to the environment."""
        arg_list = CellEnvArgList(self.machine, self.image, len(args), args)
        self._add_entry("MAIN_ARGS", arg_list)

    def snap(self, cell):
        """
        Snap the cap pointer names in caps.
        """

        for ent in self.cap_entries:
            if ent.cap is None and ent.cap_name is not None:
                cap = self.namespace.lookup(ent.cap_name)

                if cap is None:
                    # Needs more context.
                    raise MergeError, "Cap %s not found." % ent.cap_name
                else:
                    assert isinstance(cap, CellCap)

                    ent.cap = cap
                    ent.dest_cap = cap.export(cell.space)

    def add_int_entry(self, tag, integer):
        """
        Create an environment entry that stores an integer.
        """
        self.add_value_entry(tag, integer)

    def generate_dynamic_segments(self, cell, image, machine, pools):
        """
        Create a memsection for the environment based on the size
        of the contents.
        """
        attrs        = image.new_attrs(
            cell.namespace.add_namespace("cell_environment"))
        attrs.attach = PF_R | PF_W
        attrs.pager  = None
        attrs.data   = ByteArray('\0')

        cell.space.register_mapping(attrs)

        self._add_entry("SEGMENTS",
                CellEnvSegments(self.machine, self.image, self.mappings,
                        self.min_page_size))
        for mapping in self.mappings:
            self._add_entry(mapping[1],
                    CellEnvSegmentEntry(self.machine, self.image, mapping))

        self.snap(cell)

        out = self.generate_data(machine, image)
        attrs.size = len(out.getvalue())

        self.memsect = image.add_memsection(attrs, machine, pools,
                                            section_prefix = cell.name)

    # def generate_init(self, machine, kernel, pools, image):
    def generate_init(self, machine, _, pools, image):
        """Create the environment section."""
        out = self.generate_data(machine, image)

        dlen = len(out.getvalue())
        assert(self.memsect.attrs.size == dlen)

        self.memsect.segment.sections[0]._data[:dlen] = \
                                                      ByteArray(out.getvalue())

    def generate_data(self, machine, image):
        """
        Generate the binary form of the environment.
        """
        out = StringIO()

        section = CellEnvSection(self.machine, self.image, out)

        # Determine the virtual address of each string and entry.
        # This method can be called before the base address of the
        # memsection is know, in which case use a base address of 0,
        # because it doesn't really matter.
        if self.memsect != None and self.memsect.attrs.virt_addr is not None:
            base = self.memsect.attrs.virt_addr
        else:
            base = 0

        section.write_environment(base, self.entries, self.pd_list, self.space_list,
        self.clist, self.space_utcb_list)

        return out
