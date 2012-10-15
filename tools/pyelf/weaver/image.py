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

"""Generate the ELF image file."""

import os.path
from elf.core import UnpreparedElfFile, SectionedElfSegment
from elf.section import UnpreparedElfSection
from elf.constants import PT_PAX_FLAGS, PT_GNU_STACK, PT_PHDR, SHT_ARM_EXIDX, \
                          PT_LOAD, PT_MIPS_REGINFO, PT_ARM_EXIDX, ET_EXEC, \
                          SHT_PROGBITS, SHF_WRITE, SHF_ALLOC, \
                          PF_R, PF_W, ElfMachine, EF_MIPS_ABI_O64
from elf.ByteArray import ByteArray
from elf.util import align_up, align_down
from weaver import MergeError
import weaver.pools
from weaver.allocator import AllocatorItem, AllocatorGroup
import weaver.machine

# A list of segment types that are not included in the final image.
skipped_types = [
    PT_PAX_FLAGS, # PAX_FLAGS.  Added by the gentoo linker.
    PT_GNU_STACK, # PT_GNU_STACK.  Indicates stack executability.
    PT_MIPS_REGINFO, #
    PT_PHDR,      # PT_PHDR. Entry for header table itself
    SHT_ARM_EXIDX,
    ]

# List of symbols that should be changed when they're added to the
# image symbol table.
# Probably should be list of regexs, but that's not needed yet.
NO_PREFIX_SYMBOLS = (
    # ARM mapping symbols.
    "$a", # ARM code
    "$t", # THUMB code.
    "$d", # Data items
    )

def can_prefix_symbol(symbol):
    """
    Return whether or not a symbol should have a prefix added to it in
    the final image.
    """
    return symbol.name not in NO_PREFIX_SYMBOLS

def valid_segment(segment):
    if segment.type in skipped_types:
        return False

    if segment.get_memsz() == 0:
        return False

    if segment.type != PT_LOAD and segment.type != PT_ARM_EXIDX:
        raise MergeError, "Unable to handle segments that aren't " \
              "of type LOAD (found type 0x%x)." % (segment.type)

    return True

class ImageAttrs(object):
    """
    Attributes of an item that appears in the ELF image.
    """
    # Name used to be an attribute.  Disable to catch bugs.
    name = property()

    class MemType (object):
        """
        Representation of the properties of an object memory type.
        """
        def __init__(self, need_virt, need_phys, need_mapping):
            self.needs_virt    = need_virt
            self.needs_phys    = need_phys
            self.needs_mapping = need_mapping

        def need_virtual(self):
            """
            Return whether or not the objecy needs virtual memory.
            """
            return self.needs_virt

        def need_physical(self):
            """
            Return whether or not the object needs physical memory.
            """
            return self.needs_phys

        def need_mapping(self):
            """
            Return whether or not the object requires a mapping.
            """
            return self.needs_mapping

    # Representation of an unmapped object.  Used internally.
    unmapped = MemType(True, True, False)

    def __init__(self, ns_node, virtpool, physpool, pager, direct):
        self.ns_node      = ns_node
        self.file         = None
        self.data         = None
        self.size         = None
        self.virt_addr    = None
        self.phys_addr    = None
        self.direct       = direct
        self.virtpool     = virtpool
        self.physpool     = physpool
        self.align        = None
        self.attach       = PF_R | PF_W
        self.pager        = pager
        self.elf_flags    = 0
        self.scrub        = True
        self.protected    = False
        self.cache_policy = weaver.machine.CACHE_POLICIES['default']
        self.user_map     = False
        self.mem_type     = None

        assert self.cache_policy is not None


    def has_virt_addr(self):
        """
        Return whether or not the virtual address attribute has been set.
        """
        return self.virt_addr is not None

    def has_phys_addr(self):
        """
        Return whether or not the physical address attribute has been set.
        """
        return self.phys_addr is not None

    def need_memory(self, is_virtual):
        """
        Return whether or not the object needs virtual or physial
        memory.
        """
        if self.mem_type is None:
            return True

        if is_virtual:
            return self.mem_type.need_virtual()
        else:
            return self.mem_type.need_physical()

    def need_mapping(self):
        """
        Return whether or not the object requires a mapping.
        """

        return self.mem_type is None or \
               self.mem_type.need_mapping()

    def should_scrub(self):
        """
        Indicates whether or not iguana should zero out the memsection
        associated with the item.
        """
        if self.scrub:
            return (self.file is None and self.data is None)
        else:
            return False

    def abs_name(self):
        """Return the absolute name of the attribute."""
        return self.ns_node.abs_name()

class ImageAllocatorItem(AllocatorItem):
    """
    Allocator item for ELF image contents.
    """
    def __init__(self, image_object, is_virtual):
        self.image_object = image_object
        self._is_virtual  = is_virtual
        self.attrs        = self.image_object.get_attrs()

    def is_virtual(self):
        """Return whether or not a virtual address is being allocated."""
        return self._is_virtual

    def get_name(self):
        """Return a text description of the item."""
        return self.attrs.abs_name()

    def get_size(self):
        """Return the size of the item ."""
        return self.attrs.size

    def get_alignment(self):
        """Return the desired alignment of the item."""
        return self.attrs.align

    def get_offset(self):
        """Return the desired offset into the page of the item."""

        # If we have a virtual address, calculate the offset...
        if self.attrs.virt_addr is not None:
            return self.attrs.virt_addr % self.attrs.align
        # Otherwise place the item at the bottom of the page.
        else:
            return 0

    def get_cache_policy(self):
        return self.attrs.cache_policy

    def get_addr(self):
        """
        Return the address of the object.
        Returns None if the address has not been set.
        """
        if self._is_virtual:
            return self.attrs.virt_addr
        else:
            return self.attrs.phys_addr

    def set_addr(self, addr):
        """Set the address of the object to the given value."""
        if self._is_virtual:
            self.attrs.virt_addr = addr
        else:
            self.attrs.phys_addr = addr

    def pre_alloc(self, system_default_virt, system_default_phys,
                  machine, pools):
        """
        Set attribute defaults that are not known until layout time.
        """
        # If the pools are not known use the default pools.
        if self.attrs.virtpool is None:
            self.attrs.virtpool = system_default_virt

        if self.attrs.physpool is None:
            self.attrs.physpool = system_default_phys

        # Use the natural alignment of the item if non is specified.
        if self.attrs.align is None:
            self.attrs.align = \
                             machine.natural_alignment(self.attrs.size)

        if self.attrs.direct and \
               not pools.is_physical_direct(self.attrs.physpool):
            raise MergeError, \
                  'Physical pool "%s" does not support direct memory ' \
                  'allocation.' % \
                  self.attrs.physpool

    def post_alloc(self, pools):
        """
        Perform any post processing once the address has been
        calculated.

        This function may be called mulitple times.
        """
        self.image_object.post_alloc(pools)

    def get_pool(self):
        """
        Get the pool associated with this item.
        """
        if self._is_virtual:
            return self.attrs.virtpool
        else:
            return self.attrs.physpool

class ImageGroup(AllocatorGroup):
    """
    A collection of items that should be allocated together.

    An ImageGroup represents a collection of items that static
    analysis indicates should be allocated together.  Such groups
    could include the kernel and kernel heap, a programs text, heap
    and stack, etc.

    However, the user is free to request that the  items in the group
    be placed in different pools or zones, and so it is not safe to
    pass this group directly into the allocator.

    Instead, the layout() method runs along the collection, creating a
    new AllocatorGroup object whenever a new pool is found.  These
    dynamic collections are passed to the allocator and must be
    allocated together.
    """
    def __init__(self, distance, entries = None, error_message = None):
        AllocatorGroup.__init__(self, distance, entries, error_message)

    def layout(self, system_default_virt, system_default_phys,
               machine, pools):
        """Allocate memory for every item in the group."""
        alloc_items = []
        last_item = None

        for ent in self.get_entries():
            # Pre-allocation final initialisation of the item.
            ent.pre_alloc(system_default_virt, system_default_phys,
                          machine, pools)

            if last_item is not None and \
                   last_item.get_pool() != ent.get_pool():
                alloc_group = AllocatorGroup(self.get_distance(),
                                             alloc_items,
                                             self.get_error_message())

                pools._alloc(alloc_items[0], alloc_group)
                alloc_items = [ent]
            else:
                alloc_items.append(ent)

            last_item = ent

        # Allocate the remaining group.
        alloc_group = AllocatorGroup(self.get_distance(),
                                     alloc_items,
                                     self.get_error_message())

        pools._alloc(alloc_items[0], alloc_group)

        # Perform any post-allocation processing on each item.
        for ent in self.get_entries():
            ent.post_alloc(pools)

class ImageObject(object):
    """Description of objects that end up in the image."""

    def __init__(self, attrs):
        self.attrs = attrs

    def get_allocator_item(self, is_virtual):
        """
        Return an object used to allocate a virtual or physical
        address for the item, or None if such allocation is not
        meaningful.

        is_virtual:  Indicate whether or nor an allocator item for the
        virtual or physical address is being requested.
        """
        return ImageAllocatorItem(self, is_virtual)

    def get_attrs(self):
        """Return the image attributes of the object."""
        return self.attrs

    def root_mappings(self):
        """
        Return the tuple (virt, phys, size) of the object if the object
        is a rootserver object.  Returns None otherwise.

        These mappings setup by the kernel before is starts the
        rootserver.
        """
        return None

    def post_alloc(self, pools):
        """
        Called after the memory has been allocated for the item.

        This method may be called multiple times are the virtual and
        physical addresses are allocated.
        """
        pass

class ImageSegment(ImageObject):
    """ELF Segment for the image."""

    def __init__(self, segment, segment_index,
                 file_type, attrs, pools):
        ImageObject.__init__(self, attrs)

        self.segment       = segment
        self.segment_index = segment_index
        self.file_type     = file_type
        self.attrs.size    = segment.get_memsz()

        # Set direct addressing, if that's what's wanted.
        if self.attrs.direct:
            self.attrs.phys_addr = segment.vaddr
            self.attrs.virtpool = 'direct'

        # Declare the segment's physical memory range in use.
        marked = pools.mark_physical(self.attrs.abs_name(),
                                     self.attrs.phys_addr,
                                     self.attrs.size,
                                     self.attrs.cache_policy)

        if self.attrs.phys_addr is not None and not marked:
            raise MergeError, \
                  'Segment "%s": Cannot reserve physical addresses ' \
                  '%#x--%#x.' % \
                  (self.attrs.abs_name(), self.attrs.phys_addr,
                   self.attrs.phys_addr + self.attrs.size - 1)

        # If it's a protected segment, reserve everything in the same SECTION
        if self.attrs.protected:
            base = align_down(segment.vaddr, 1024 * 1024)
            size = align_up(self.attrs.size + base - segment.vaddr,
                            1024 * 1024)
        else:
            base = segment.vaddr
            size = self.attrs.size
        pools.mark_virtual(self.attrs.abs_name(), base, size,
                                    self.attrs.cache_policy)

    def get_allocator_item(self, is_virtual):
        """Return allocator items for the segment."""
        # Return an allocator item only for physical addresses.
        if is_virtual:
            return ImageAllocatorItem(self, is_virtual = True)
        else:
            return ImageAllocatorItem(self, is_virtual = False)

    def post_alloc(self, _):
        """
        Set the segment's physical address once allocation is
        complete.
        """
        self.segment.paddr = self.attrs.phys_addr

    def root_mappings(self):
        if self.file_type == Image.ROOT_PROGRAM:
            # FIXME (benno) 0x1000 is hard coded, bad!
            virt_addr = align_down(self.attrs.virt_addr, 0x1000)
            phys_addr = align_down(self.attrs.phys_addr, 0x1000)
            if (self.attrs.virt_addr - virt_addr) != \
                   (self.attrs.phys_addr - phys_addr):
                raise Exception("Non congruent segment mappings, can't deal")
            size = self.attrs.size + self.attrs.virt_addr - virt_addr
            return (virt_addr, phys_addr, size)
        else:
            return None

class ImageMemsection(ImageObject):
    """Memsection for the image."""
    def __init__(self, segment, attrs, pools):
        ImageObject.__init__(self, attrs)

        self.segment             = segment
        self.owned_by_rootserver = False

        if self.attrs.direct:
            self.attrs.virtpool = 'direct'

        pools.mark_virtual(attrs.abs_name(),
                           self.attrs.virt_addr,
                           self.attrs.size, self.attrs.cache_policy)
        pools.mark_physical(attrs.abs_name(),
                            self.attrs.phys_addr,
                            self.attrs.size, self.attrs.cache_policy)


    def set_root_server_ownership(self, ownership):
        self.owned_by_rootserver = ownership

    def get_allocator_item(self, is_virtual):
        # Memsections with direct addressing do not need virtual
        # allocators.  Rather the allocated physical address is copied
        # into the virtual address.
        if self.attrs.direct and is_virtual:
            return None
        elif self.attrs.need_memory(is_virtual):
            return ImageAllocatorItem(self, is_virtual = is_virtual)
        else:
            return None

    def post_alloc(self, pools):
        if self.attrs.direct and self.attrs.has_phys_addr():
            pools.mark_virtual(self.attrs.abs_name(),
                               self.attrs.phys_addr,
                               self.attrs.size)
            self.attrs.virt_addr = self.attrs.phys_addr

        if self.segment:
            self.segment.paddr = self.attrs.phys_addr
            self.segment.vaddr = self.attrs.virt_addr
            self.segment.sections[0].address = self.attrs.virt_addr

    def root_mappings(self):
        if self.owned_by_rootserver:
            return (self.attrs.virt_addr, self.attrs.phys_addr, self.attrs.size)
        else:
            return None

class ImageUtcbArea(ImageObject):
    def __init__(self, attrs):
        ImageObject.__init__(self, attrs)

    def get_allocator_item(self, is_virtual):
        # Return an allocator item only for virtual addresses.
        if is_virtual:
            return ImageAllocatorItem(self, is_virtual = is_virtual)
        else:
            return None

class ImageKernelHeap(ImageObject):
    """ Kernel Heap for the image. """

    def __init__(self, attrs, pools):
        ImageObject.__init__(self, attrs)

        if self.attrs.size is not None:
            pools.mark_physical(self.attrs.abs_name(),
                                self.attrs.phys_addr,
                                self.attrs.size,
                                self.attrs.cache_policy)

    def get_allocator_item(self, is_virtual):
        # Return an allocator item only for physical addresses.
        if is_virtual:
            return None
        else:
            class ImageKernelHeapAllocatorItem(ImageAllocatorItem):
                """
                Custom class that sets the default alignment of
                the heap to superpage alignment.
                """
                def pre_alloc(self, system_default_virt,
                              system_default_phys, machine, pools):
                    """Set the alignment to superpage size."""
                    # Record if the alignment should be set.
                    align = self.attrs.align

                    # Call the superclass.  This will set the
                    # alignment to the natural size.
                    ImageAllocatorItem.pre_alloc(self, system_default_virt,
                                                 system_default_phys,
                                                 machine, pools)

                    # Set it to the superpage size, it is should be.
                    if align is None:
                        self.attrs.align = machine.superpage_alignment(self.attrs.size)

            return ImageKernelHeapAllocatorItem(self, is_virtual = False)

class ImageKernelHeapBacked(ImageKernelHeap):
    """ Kernel Heap backed by segment for the image. """

    def __init__(self, attrs, elf, _, pools, kernel_seg):
        ImageKernelHeap.__init__(self, attrs, pools)

        self.segment = None
        self.kernel_segment = kernel_seg

        data = self.attrs.data = ByteArray([])
        if attrs.size is not None and len(data) < self.attrs.size:
            data.extend([0] * (self.attrs.size - len(data)))

        self.attrs.size = data.buffer_info()[1] * data.itemsize

        section_name = "kernel.heap"
        sect = UnpreparedElfSection(elf, section_name, SHT_PROGBITS,
                                    self.attrs.virt_addr,
                                    data = data,
                                    flags = SHF_WRITE | SHF_ALLOC)
        elf.add_section(sect)
        self.segment = SectionedElfSegment(elf, PT_LOAD, self.attrs.virt_addr,
                                          self.attrs.phys_addr, PF_R | PF_W,
                                          self.attrs.align,
                                          sections=[sect])
        elf.add_segment(self.segment)

    def post_alloc(self, _):
        self.attrs.virt_addr = self.kernel_segment.vaddr + \
                (self.attrs.phys_addr - self.kernel_segment.paddr)

        if self.segment:
            self.segment.paddr = self.attrs.phys_addr
            self.segment.vaddr = self.attrs.virt_addr
            self.segment.sections[0].address = self.attrs.virt_addr

class ImageZone(ImageObject):
    """
    Zones for the image.

    Zones don't actually appear in the image but they need to know
    what virtual pool they are associated with and if it is the
    default virtual pool this is only known during the layout function
    below.

    Again, zones do not produce anything that appears in the image.
    """

    def __init__(self, attrs, zone):
        ImageObject.__init__(self, attrs)

        self.zone        = zone
        self.memsections = []

    def get_allocator_item(self, _):
        """
        Return allocator items for the zone.

        Since the zone doe snot need allocation, nothing is returned.
        """
        # Should never be called, but just in case don't return an
        # allocator item.
        return None

    def add_memsection(self, memsect):
        """Add a memsection to the zone."""
        self.memsections.append(memsect)

    def prime(self, system_default_virt, _, pools):
        """
        Prime the zone with the pools from which is it to get extra
        memory and the areas of memory that are already allocated.
        """
        if self.attrs.virtpool is None:
            self.attrs.virtpool = system_default_virt

        # Tell the underlying zone allocator where to get its memory
        # from.
        self.zone.prime(self.attrs.virtpool,
                        [(ms.get_attrs().virt_addr, ms.get_attrs().size)
                         for ms in self.memsections
                         if ms.get_attrs().has_virt_addr()],
                        pools)
class Image:
    """Representation of the contents of the final image."""
    # Different types of segments.
    # NOTE:  PROGRAM and EXTENSION must have the values 1 and 2 to
    # maintain binary compatibility with the iguana library function
    # get_elf_info().
    PROGRAM      = 1
    EXTENSION    = 2
    KERNEL       = 3
    ROOT_PROGRAM = 4

    class Patch:
        """A Class for describing patches to segments."""
        def __init__(self, addr, size, value, offset = 0):
            self.addr    = addr
            self.size    = size
            self.value   = value
            self.offset  = offset

    class AttrStack:
        """
        Class for holding a stack of attribute values.  Virtpool,
        physpool and pagers operate in that way.

        These stacks differ from regular stacks in that the top of the
        stack is defined to be the last non-None entry in the list.
        """
        def __init__(self):
            self.stack = []
            self.top   = None

        def __getitem__(self, index):
            return self.stack[index]

        def set(self, value):
            """
            Set the stack to only contain the given value.
            """
            assert value is not None

            self.stack = [value]
            self.top   = 0

            assert self.top is None or self.top < len(self.stack)

        def push(self, value):
            """
            Push an item onto the stack.  If the item is not None,
            then then will become the top of the stack.
            """
            self.stack.append(value)

            if value is not None:
                self.top = len(self.stack) - 1

                if self.top < 0:
                    self.top = None

            assert self.top is None or \
                   (self.top < len(self.stack) and \
                    self.stack[self.top] is not None)

        def pop(self):
            """
            Pop on item off the stack.  If the item is non-None,
            then the top of the stack is moved to the last non-None
            value in the list.
            """
            value = self.stack.pop()

            if value is not None:
                # Recalculate the top of the stack.
                self.top = None
                i        = 0

                for item in self.stack:
                    if item is not None:
                        self.top = i
                    i += 1

                assert self.top is None or \
                       self.stack[self.top] is not None

            assert self.top is None or self.top < len(self.stack)

        def tos(self):
            """
            Return the item at the top of the stack, or None if there
            is no such item.
            """
            if self.top is None:
                return None
            else:
                return self.stack[self.top]

        def bot(self):
            """
            Return the item at the bottom of the stack, or None if the
            stack is empty.
            """
            if len(self.stack) == 0:
                return None
            else:
                return self.stack[0]


    def __init__(self, ph_offset):
        self.ph_offset = ph_offset
        self.objects = None
        self.kernel_segments = []
        self.kernel_heap     = None
        self.kernel_arrays   = []
        self.utcb_areas = []
        self.utcb_size = 0
        self.segments    = []
        self.memsections = []
        self.zones       = []
        self.elf = None
        self.endianess = None
        self.wordsize = None
        self.patches = []
        self.virt_pool_stack = Image.AttrStack()
        self.phys_pool_stack = Image.AttrStack()
        self.pager_stack     = Image.AttrStack()
        self.direct_stack    = Image.AttrStack()
        self.protected_segment = None
        self.groups = []

    def get_elf(self):
        """Return the ELF file that will contain the image."""
        return self.elf

    def current_pools(self):
        """Return the current virtual and physical pools."""
        return (self.virt_pool_stack.tos(),
                self.phys_pool_stack.tos())

    def new_attrs(self, ns_node, for_segment = False):
        """
        Create a new attribute object.

        The attributes are initialised with the current values from
        the attribute stack and the supplied namespace.
        """
        def_direct = False

        if for_segment:
            def_direct = self.direct_stack.tos()

        return ImageAttrs(ns_node  = ns_node,
                          virtpool = self.virt_pool_stack.tos(),
                          physpool = self.phys_pool_stack.tos(),
                          pager    = self.pager_stack.tos(),
                          direct   = def_direct)

    def set_attrs_stack(self, def_virt = None, def_phys = None,
                        def_pager = None, def_direct = None):
        """
        Prime the attribute stack with initial values.
        """
        if def_virt is not None:
            self.virt_pool_stack.set(def_virt)

        if def_phys is not None:
            self.phys_pool_stack.set(def_phys)

        if def_pager is not None:
            self.pager_stack.set(def_pager)

        if def_direct is not None:
            self.direct_stack.set(def_direct)

    def push_attrs(self, virtual = None, physical = None,
                   pager = None, direct = None):
        """Push values onto the attribute stack."""
        self.virt_pool_stack.push(virtual)
        self.phys_pool_stack.push(physical)
        self.pager_stack.push(pager)
        self.direct_stack.push(direct)

    def pop_attrs(self):
        """Pop values from the attribute stack."""
        self.virt_pool_stack.pop()
        self.phys_pool_stack.pop()
        self.pager_stack.pop()
        self.direct_stack.pop()

    def prepare(self):
        """Prepare the ELF file for writing to disk."""
        self.elf = self.elf.prepare(self.wordsize, self.endianess)

    def write_out_image(self, output_file, image, kernel, machine):
        """Write out the final ELF file."""
        kernel.update_elf(self.elf, image, machine)
        #self.elf = self.elf.prepare(self.wordsize, self.endianess)
        self.elf.to_filename(output_file)

    def make_single_list(self):
        """
        Place all of the objects into a single list to generate a good
        layout.

        Items that will be written to the ELF file are placed together
        to try and reduce the size of the image.
        """
        #
        # Approximate proper support for proximity by placing the
        # kernel heap close to the kernel and memsections with data
        # close to the segments.
        #
        # Proper support should be added to ensure that wombat's
        # vmlinux memsection is close to the wombat text.
        #
        self.objects = []
        self.objects.extend(self.kernel_segments)
        self.objects.extend(self.kernel_arrays)
        self.objects.append(self.kernel_heap)
        self.objects.extend(self.utcb_areas)
        self.objects.extend(self.segments)
        self.objects.extend(self.memsections)

    def layout(self, machine, pools):
        """Layout the image in memory."""
        self.make_single_list()

        for obj in self.zones:
            obj.prime(self.virt_pool_stack[0],
                      self.phys_pool_stack[0],
                      pools)

        for obj in self.groups:
            obj.layout(self.virt_pool_stack[0],
                       self.phys_pool_stack[0],
                       machine,
                       pools)

    def apply_patches(self):
        """Apply registered patches."""
        for segment in self.elf.segments:
            for section in segment.sections:
                patches = [patch for patch in self.patches
                           if patch.addr >= section.address and
                           patch.addr < section.address + section.get_size()]
                for patch in patches:
                    offset = patch.addr - section.address
                    if isinstance(patch.value, weaver.image.ImageObject):
                        value = patch.value.attrs.phys_addr + patch.offset
                    else:
                        value = patch.value + patch.offset
                    section.get_data().set_data(offset, value,
                                                patch.size, self.endianess)

    def get_value(self, address, size, endianess=None):
        """get a value from the image."""

        # Refactored into elf/core.py
        ret = self.elf.get_value(address, size, endianess)
        if ret != None:
            return ret

        raise MergeError, "Could not find address %x in Image." % address

    def set_kernel(self, kernel):
        """ Record the kernel."""
        self.elf = UnpreparedElfFile()
        self.endianess = kernel.endianess
        self.wordsize = kernel.wordsize

        self.elf.elf_type = ET_EXEC
        self.elf.machine = kernel.machine
        self.elf.osabi = kernel.osabi
        self.elf.abiversion = kernel.abiversion
        self.elf.flags = kernel.flags
        self.elf.entry_point = kernel.entry_point
        if self.ph_offset is not None:
            self.elf.set_ph_offset(self.ph_offset, fixed=True)

    def patch(self, addr, size, value, offset = 0):
        """Record the details of a patch to a segment."""
        if self.elf.machine == ElfMachine(8):
            if self.elf.flags & EF_MIPS_ABI_O64:
                if addr & 0x80000000:
                    addr |= 0xffffffff00000000L
        self.patches.append(self.Patch(addr, size, value, offset))

    def set_kernel_heap(self, attrs, machine, pools, kern_seg, is_backed):
        """
        Record the details of the kernel heap.
        """
        if is_backed:
            self.kernel_heap = ImageKernelHeapBacked(attrs, self.elf, machine,
                                                     pools, kern_seg)
        else:
            self.kernel_heap = ImageKernelHeap(attrs, pools)

        return self.kernel_heap

    def add_utcb_area(self, attrs):
        """Record the details of an UTCB area"""
        area = ImageUtcbArea(attrs)
        self.utcb_areas.append(area)
        return area

    def add_segment(self, segment_index, section_prefix,
                    segment, file_type, attrs, machine, pools):
        """Create a segment for inclusion in the image."""
        if not valid_segment(segment):
            return None

        # Remove any pathname components from the prefix.
        section_prefix = os.path.basename(section_prefix)

        # Prepare the image for inclusion.
        new_segment = segment.copy_into(self.elf)

        # Align segments to the page boundary if is safe to do so.
        # RVCT tends to give very conservative alignment (1 word) to
        # segments that could be paged aligned.
        if new_segment.vaddr % machine.min_page_size() == 0 and \
               new_segment.align < machine.min_page_size():
            new_segment.align = machine.min_page_size()

        # Rename the sections in the segment, giving each the supplied
        # prefix
        if new_segment.has_sections():
            for section in new_segment.get_sections():
                assert section.link is None

                sec_name = section.name
                #strip GNU leading dots in section names
                if sec_name[0] == ".":
                    sec_name = sec_name[1:]

                section.name = "%s.%s" % (section_prefix, sec_name)
                # Add the program name as a prefix to all non-kernel
                # symbols, except for those symbols that can't have a
                # prefix added.
                if section_prefix != "kernel":
                    for symbol in self.elf.section_symbols(section):
                        if can_prefix_symbol(symbol):
                            symbol.name = "%s-%s" % (section_prefix, symbol.name)
                            self.elf.get_symbol_table().link.add_string(symbol.name)
        self.elf.add_segment(new_segment)

        iseg = ImageSegment(new_segment, segment_index, file_type,
                            attrs, pools)

        if attrs.protected:
            if self.protected_segment is not None:
                raise MergeError, \
                      'Only one segment can be declared protected.  ' \
                      'Found "%s" and  "%s".' % \
                      (self.protected_segment.get_attrs().abs_name(),
                      attrs.abs_name())

            self.protected_segment = iseg

        # Kernel segments need to be at the start of the memory pools
        # to place them in a different list to keep track of them.
        if file_type == Image.KERNEL:
            self.kernel_segments.append(iseg)
        else:
            self.segments.append(iseg)


        return iseg

    def add_memsection(self, attrs, machine, pools, section_prefix = None):
        """
        Create a memsection for inclusion in the image.

        If the data or file attributes of 'attr' are non-None, then a
        ELF segment will be created, otherwise the memsection will
        will be included in the address layout process, but will be
        created at runtime by Iguana server.
        """
        new_segment = None
        in_image = False

        if attrs.file is not None or attrs.data is not None:
            if attrs.file is not None:
                the_file = open(attrs.file, 'r')
                data = ByteArray(the_file.read())
                the_file.close()
            else:
                data = attrs.data

            if attrs.size is not None and len(data) < attrs.size:
                data.extend([0] * (attrs.size - len(data)))

            attrs.size = data.buffer_info()[1] * data.itemsize

            if section_prefix:
                section_name = "%s.%s" % (section_prefix, attrs.ns_node.name)
            else:
                section_name = attrs.ns_node.name
            sect = UnpreparedElfSection(self.elf, section_name, SHT_PROGBITS,
                                        attrs.virt_addr,
                                        data = data,
                                        flags = SHF_WRITE | SHF_ALLOC)
            self.elf.add_section(sect)
            new_segment = SectionedElfSegment(self.elf, PT_LOAD, attrs.virt_addr,
                                              attrs.phys_addr, PF_R | PF_W,
                                              machine.min_page_size(),
                                              sections=[sect])
            self.elf.add_segment(new_segment)
            in_image = True
# This check should be added, but currently fails with iguana.
#         elif attrs.size is None:
#             raise MergeError, \
#                   'No size attribute given for memsection \"%s\".' % attrs.abs_name()

        obj = ImageMemsection(new_segment, attrs, pools)

        # If the memsection has data that goes into the image, then
        # put it at the front of the list so that it will be near the
        # code segments.
        if in_image:
            self.memsections = [obj] + self.memsections
        else:
            self.memsections.append(obj)

        return obj

    def add_zone(self, attrs, zone):
        """Create a zone for inclusion in the image."""
        izone = ImageZone(attrs, zone)
        self.zones.append(izone)

        return izone

    def _add_group(self, distance, items, is_virtual, error_message=None):
        """Generate the group for either virtual or physical addresses."""
        group_items = [i.get_allocator_item(is_virtual = is_virtual)
                 for i in items
                 if i.get_allocator_item(is_virtual = is_virtual) is not None]

        if group_items:
            group = ImageGroup(distance, group_items, error_message)
            self.groups.append(group)

    def add_group(self, distance, items, error_message = None):
        """
        Add a static group of items for allocation.

        A static group is a group that the developer thinks should be
        allocated together (such as a program and all of its
        memsections.

        These will later be converted into dynamic groups, which take
        into account the possibily different pools that the items will
        be allocated from.
        """
        # Generate a static group for virtual addresses.
        self._add_group(distance, items, True, error_message)

        # Generate a static group for physical addresses.
        self._add_group(distance, items, False, error_message)

    def dump(self):
        """
        Print out a virtual and physical memory map of the final
        image.
        """
        virtual_objects = {}
        physical_objects = {}

        for obj in self.objects:
            if obj.attrs.virt_addr is not None:
                vbase = obj.attrs.virt_addr
                vend = vbase + obj.attrs.size - 1
                virtual_objects[vbase, vend] = obj.attrs.abs_name()
            if obj.attrs.phys_addr is not None:
                pbase = obj.attrs.phys_addr
                pend = pbase + obj.attrs.size - 1
                if (pbase, pend) in physical_objects:
                    physical_objects[pbase, pend].append(obj.attrs.abs_name())
                else:
                    physical_objects[pbase, pend] = [obj.attrs.abs_name()]

        print "VIRTUAL:"
        for (base, end), name in sorted(virtual_objects.items()):
            print "  <%08x:%08x> %s" % (base, end, name)

        print "PHYSICAL:"
        for (base, end), names in sorted(physical_objects.items()):
            for name in names:
                print "  <%08x:%08x> %s" % (base, end, name)
