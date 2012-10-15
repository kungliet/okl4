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

"""Binary representations of cell environment entries."""

from struct import pack
from elf.util import align_up, align_down
from weaver.util import _0, get_symbol
from elf.constants import PF_R, PF_W, PF_X
from weaver.kernel import KERNEL_MICRO, KERNEL_NANO

class CellEnvironmentEntry:
    """
    Base class for all environment entries.

    An environment entry writes a collection of <words> into the
    environment.

    The structure size must be a multiple of the word size and will be
    word aligned in memory.
    """
    def __init__(self, machine, image):
        self.virt_base = 0
        self.machine = machine
        self.image = image

    def get_size(self):
        """Return the number of bytes in this structure."""
        writer = MeasuringBinaryWriter(self.machine, self.image)
        self.write_struct(writer)

        # Ensure the user wrote out aligned data.
        assert writer.get_bytes_written() % self.machine.sizeof_word == 0

        return writer.get_bytes_written()

    def set_virt_base(self, base):
        self.virt_base = base

    def write_struct(self, section):
        """Write the binary form of the entry."""
        raise NotImplementedError

class CellEnvKernelInfo(CellEnvironmentEntry):
    """
    Environment entry for Kernel configuration info.

    The data structure for the entry is:

    struct okl4_kernel_info {
        okl4_word_t base;
        okl4_word_t end;
        okl4_word_t max_spaces;
        okl4_word_t max_mutexes;
        okl4_word_t max_threads;
    }

    """
    def __init__(self, machine, image, base, end, kernel):
        CellEnvironmentEntry.__init__(self, machine, image)

        self.base        = base
        self.end         = end
        self.kernel      = kernel

    def write_struct(self, section):
        """Write the binary form of the kernel info struct."""
        section.write_word(self.base)
        section.write_word(self.end)
        section.write_word(self.kernel.total_spaces)
        section.write_word(self.kernel.total_mutexes)
        section.write_word(self.kernel.total_threads)

class CellEnvBitmapAllocator(CellEnvironmentEntry):
    """
    Environment entry for a libokl4 bitmap allocator.

    The data structure for the entry is:

    struct okl4_bitmap_allocaor {
        okl4_word_t base;
        okl4_word_t size;
        okl4_word_t pos_guess;
        okl4_word_t data[1]; /* Variable length */
    }
    """
    def __init__(self, machine, image, base = 0, size = 0, preallocated = 0):
        CellEnvironmentEntry.__init__(self, machine, image)

        self.size = size
        self.base = base
        self.preallocated = preallocated

    def write_struct(self, section):
        """Write the binary form of the kernel info struct."""

        section.write_word(self.base)
        section.write_word(self.size)
        section.write_word(0) # pos_guess
        bits_per_word = self.machine.sizeof_word * 8

        # Write out the bitmap
        rounded_up_size = align_up(self.size, bits_per_word)
        current_word = 0
        for pos in range(0, rounded_up_size):
            # The current bit should be '0' if the resource is free. This is
            # true if (i) we are past the 'used' range and (ii) we are still in
            # the valid range.
            current_bit = 0
            if pos < self.preallocated or pos >= self.size:
                current_bit = 1

            # Write out the bit
            current_word |= (current_bit << (pos % bits_per_word))

            # If we have finished a word, write it out
            if pos % bits_per_word == bits_per_word - 1:
                section.write_word(current_word)
                current_word = 0

        # Finally, we must make sure that we always write out at least one
        # word.
        if self.size == 0:
            section.write_word(0)

class CellEnvRangeItem(CellEnvironmentEntry):
    """
    Environment entry for a libokl4 range allocator.

    The data structure for the entry is:

    struct okl4_range_item {
        okl4_word_t base;
        okl4_word_t size;
        okl4_word_t total_size;
        struct okl4_range_item *next;
    }
    """
    def __init__(self, machine, image, base, size, free_size):
        CellEnvironmentEntry.__init__(self, machine, image)

        self.base = base
        self.size = size
        self.free_size = free_size

    def write_struct(self, section):
        """Write the binary form of the struct."""

        section.write_word(self.base) # base
        section.write_word(self.size) # size
        section.write_word(self.free_size) # total_size
        section.write_word(0) # next

class CellEnvUtcbArea(CellEnvironmentEntry):
    """
    Environment entry for the UTCB Area.
    The data structure for the entry is :

    struct okl4_utcb_area {
        okl4_virtmem_item_t utcb_memory;
        struct okl4_bitmap_allocator utcb_allocator; /* variable sized */
    };

    """
    def __init__(self, machine, image, space):
        CellEnvironmentEntry.__init__(self, machine, image)
        self.space = space

        # Calculate number of UTCB entires
        num_entries = (space.utcb.size / image.utcb_size)

        # Generate a bitmap allocator
        self.bitmap_allocator = CellEnvBitmapAllocator(machine, image,
                size = num_entries, preallocated = len(space.threads))

    def write_struct(self, section):
        section.write_word(_0(self.space.utcb.virt_addr))
        section.write_word(self.space.utcb.size)
        section.write_word(0)
        section.write_word(0)

        # Write the bitmap allocator
        self.bitmap_allocator.write_struct(section)

class CellEnvKclist(CellEnvironmentEntry):
    """
    Environment entry for the kclist object.
    The data structure for the entry is:

    struct okl4_kclist {
        okl4_kclistid_t id;
        okl4_word_t num_cap_slots;
        struct okl4_kspace * kspace_list;
        struct okl4_bitmap_allocator kcap_allocator;
    };
    """
    def __init__(self, machine, image, cell):
        CellEnvironmentEntry.__init__(self, machine, image)

        self.cell = cell
        self.bitmap_allocator = CellEnvBitmapAllocator(machine, image,
                size = self.cell.max_caps)
        self.kspace_list = 0

    def write_struct(self, section):
        """ Write the binary form of the kclist structure """
        section.write_word(self.cell.clist_id) # kclist id
        section.write_word(self.cell.max_caps) # number of cap slots in the kclist
        section.write_word(self.kspace_list) # kspace_list

        # Determine how many threads have already been created
        i = 0
        for space in self.cell.spaces:
            i = i + len(space.threads) + len(space.ipc_caps)
            i = i + len(space.threads)
        self.bitmap_allocator.preallocated = i

        # Write out the bitmap allocator
        self.bitmap_allocator.write_struct(section)

class CellEnvKspace(CellEnvironmentEntry):
    """
    Environment entry for the Kspace object.
    The data structure for the entry is:

    struct okl4_kspace {
        okl4_kspaceid_t id;
        struct okl4_kclist *kclist;
        struct okl4_utcb_area *utcb_area;
        struct okl4_kthread *kthread_list;
        struct okl4_kspace *next;
        okl4_word_t privileged;
    };
    """

    def __init__(self, machine, image, kernel_type, space):
        CellEnvironmentEntry.__init__(self, machine, image)
        self.space = space
        self.kclist = 0
        self.next = 0
        self.utcb_area = 0
        self.kthread_list = 0
        self.kernel_type = kernel_type

    def write_struct(self, section):
        """ Write the binary form of the kspace struct. """

        if self.kernel_type == KERNEL_MICRO:
            section.write_word(self.space.id) # kspace's id
            section.write_word(self.kclist) # the kclist (comes from the cell object)
            section.write_word(self.utcb_area)
            section.write_word(self.kthread_list) # the kthread_list
            section.write_word(self.next) # next
            section.write_word(self.space.is_privileged) # privileged
        else:
            assert self.kernel_type == KERNEL_NANO
            section.write_word(self.kthread_list) # the kthread_list

class CellEnvKthread(CellEnvironmentEntry):
    """
    Environment entry for the Kthread object.
    The data structure for the entry is:
    struct okl4_kthread {
        okl4_kcap_t cap;
        okl4_word_t sp;
        okl4_word_t ip;
        okl4_word_t priority;
        OKL4_MICRO(struct okl4_kspace *kspace;)
        OKL4_MICRO(okl4_utcb_t *utcb;)
        OKL4_MICRO(okl4_kcap_t pager;)
        OKL4_MICRO(struct okl4_kthread *next;)
    };
    """
    def __init__(self, machine, image, thread, kernel_type):
        CellEnvironmentEntry.__init__(self, machine, image)
        self.thread = thread
        self.kspace = 0
        self.next = 0
        self.kernel_type = kernel_type

    def write_struct(self, section):
        section.write_word(self.thread.cap_slot) # thread's cap
        section.write_word(self.thread.get_sp()) # sp of the thread
        section.write_word(self.thread.entry) # ip of the thread
        section.write_word(self.thread.priority) # priority of the thread

        if self.kernel_type == KERNEL_MICRO:
            section.write_word(self.kspace) # kspace of the thread
            section.write_word(_0(self.thread.utcb.virt_addr)) # utcb
            section.write_word(-3) # pager of thread
            section.write_word(self.next) #next

class CellEnvVirtmemPool(CellEnvironmentEntry):
    """
    Environment entry for a libokl4 virtmem pool.

    The data structure for the entry is:

    struct okl4_virtmem_pool {
        okl4_virtmem_item_t interval;
        struct okl4_range_allocator allocator;
        struct okl4_range_allocator * parent;
    };
    """
    def __init__(self, machine, image, attrs):
        CellEnvironmentEntry.__init__(self, machine, image)

        self.attrs = attrs

    def write_struct(self, section):
        """Write the virtmem item."""
        section.write_word(_0(self.attrs.virt_addr)) # base
        section.write_word(_0(self.attrs.size)) # size
        section.write_word(0) # total_size
        section.write_word(0) # next

        # Write the allocator struct.
        allocator = CellEnvRangeItem(self.machine, self.image,
                _0(self.attrs.virt_addr), 0, _0(self.attrs.size))
        allocator.write_struct(section)

        # rite the parent pointer.
        section.write_word(0) # parent

class CellEnvPhysmemItem(CellEnvironmentEntry):
    """
    Environment entry for a libokl4 physmem item.

    The data structure for the entry is:

    struct okl4_physmem_item {
        L4_Word_t segment_id;
        okl4_word_t paddr; // optional, otherwise poisened/random
        struct okl4_range_item range;
    };
    """
    def __init__(self, machine, image, segment, offset, size, paddr):
        CellEnvironmentEntry.__init__(self, machine, image)

        self.segment = segment
        self.offset = offset
        self.size = size
        self.paddr = paddr

    def write_struct(self, section):
        """Write the segment ID and physical address."""
        section.write_word(self.segment) # segment_id
        section.write_word(self.paddr) # paddr

        # Write the range item.
        section.write_word(0) # base
        section.write_word(_0(self.size)) # size
        section.write_word(0) # total_size
        section.write_word(0)

class CellEnvPhysmemSegpool(CellEnvironmentEntry):
    """
    Environment entry for a libokl4 physical segment pool.

    The data structure for the entry is:

    struct okl4_physmem_segpool {
        okl4_physmem_item_t phys;
        okl4_word_t pagesize;
        struct okl4_range_allocator allocator;
        struct okl4_range_allocator * parent;
    };
    """
    def __init__(self, machine, image, mapping, min_page_size):
        CellEnvironmentEntry.__init__(self, machine, image)

        self.mapping = mapping
        self.min_page_size = min_page_size

    def write_struct(self, section):
        """Write the physmem item."""
        (num, _, attrs) = self.mapping

        phys = CellEnvPhysmemItem(self.machine, self.image, num, 0,
                _0(attrs.size), _0(attrs.phys_addr))
        phys.write_struct(section)

        # Write the page size.
        section.write_word(self.min_page_size)

        # Write the allocator structure.
        allocator = CellEnvRangeItem(self.machine, self.image,
                0, 0, _0(attrs.size))
        allocator.write_struct(section)

        # Write the parent pointer.
        section.write_word(0) # parent

class CellEnvPhysmemPagepool(CellEnvironmentEntry):
    """
    Environment entry for a libokl4 physical page pool.

    The data structure for this entry is:

    struct okl4_physmem_pagepool {
        okl4_physmem_item_t phys;
        okl4_word_t pagesize;
        struct okl4_range_allocator * parent;
        struct okl4_bitmap_allocator allocator;
    };
    """

    def __init__(self, machine, image, segment, offset, size, paddr, pagesize):
        CellEnvironmentEntry.__init__(self, machine, image)

        self.bitmap_allocator = CellEnvBitmapAllocator(machine, image,
                size = size / pagesize)
        self.segment = segment
        self.offset = offset
        self.size = size
        self.paddr = paddr
        self.pagesize = pagesize

    def write_struct(self, section):
        """Write the physmem item."""
        phys = CellEnvPhysmemItem(self.machine, self.image, self.segment,
                self.offset, self.size, self.paddr)
        phys.write_struct(section)

        # Write the page size and parent pointer.
        section.write_word(self.pagesize) # pagesize
        section.write_word(0) # parent

        # Write the bitmap allocator structure.
        self.bitmap_allocator.write_struct(section)

class CellEnvMcNode(CellEnvironmentEntry):
    """
    Enironment entry for a libokl4 mcnode.

    The data structure for the entry is:

    struct _okl4_mcnode {
        _okl4_mcnode_t *next;
        _okl4_mem_container_t *payload;
        okl4_word_t attach_perms;
    };
    """
    def __init__(self, machine, image):
        CellEnvironmentEntry.__init__(self, machine, image)

        self.next = None
        self.payload = None
        self.attach_perms = 0
        self.range = CellEnvRangeItem(machine, image, 0, 0, 0)

    def write_struct(self, section):
        """Write the binary form of the mcnode struct."""
        if self.next is None:
            section.write_word(0)
        else:
            section.write_word(self.next.virt_base)
        section.write_word(self.payload.virt_base) # pointer to the memory container
        section.write_word(self.attach_perms)



class CellEnvMemsec(CellEnvironmentEntry):
    """
    Environment entry for a libokl4 memsection.

    The data structure for the entry is:

    struct okl4_memsec
    {
        _okl4_mem_container_t super;
        _okl4_mcnode_t list_node;
        okl4_word_t page_size;
        okl4_word_t perms;
        okl4_word_t attributes;
        okl4_premap_func_t premap_callback;
        okl4_access_func_t access_callback;
        okl4_destroy_func_t destroy_callback;
        void *owner;
    };

    struct _okl4_mem_container {
        okl4_virtmem_item_t range;
        enum _okl4_mem_container_type type;
    };

    struct _okl4_mcnode {
        struct _okl4_mcnode *next;
        struct _okl4_mem_container *payload;
        okl4_word_t attach_perms
    };
    """
    def __init__(self, machine, image, base, size, page_size, perms, attr):
        CellEnvironmentEntry.__init__(self, machine, image)

        self.range = CellEnvRangeItem(machine, image, base, size, size)
        self.type = 0 # _OKL4_TYPE_MEMSECTION
        self.page_size = page_size
        self.perms = perms
        self.attr = attr
        self.owner = None
        self.list_node = CellEnvMcNode(machine, image)
        self.list_node_offset = 0
        self.premap_callback = None
        self.access_callback = None
        self.destroy_callback = None

    def set_callbacks(self, elf, premap_callback, access_callback, destroy_callback):
        (self.premap_callback, _) = get_symbol(elf, premap_callback)
        (self.access_callback, _) = get_symbol(elf, access_callback)
        (self.destroy_callback, _) = get_symbol(elf, destroy_callback)

    def write_struct(self, section):
        """Write the mem container."""
        # Write mem_container struct
        self.range.write_struct(section) # super.range -> virtmem_item struct (typedef of range_item struct)
        section.write_word(self.type) # super.type

        # Write the mcnode.
        self.list_node.write_struct(section)

        # Write the rest of the structure.
        section.write_word(self.page_size)
        section.write_word(self.perms)
        section.write_word(self.attr)
        section.write_word(self.premap_callback)
        section.write_word(self.access_callback)
        section.write_word(self.destroy_callback)
        assert not (self.owner is None)
        section.write_word(self.owner.virt_base) # Object that this memsec is attached to

class CellEnvStaticMemsec(CellEnvironmentEntry):
    """
    Environment entry for a static libokl4 memsection.

    The data structure for the entry is:

    struct okl4_static_memsec {
        okl4_memsec_t base;
        okl4_physmem_item_t target;
    };
    """
    def __init__(self, machine, image, perms, attr, base, size, segment, offset, paddr, pagesize):
        CellEnvironmentEntry.__init__(self, machine, image)

        self.perms = perms
        self.attr = attr
        self.base = base
        self.size = size
        self.segment = segment
        self.offset = offset
        self.paddr = paddr
        self.pagesize = pagesize

    def write_struct(self, section):
        """Write the base memsec object."""
        ms = CellEnvMemsec(self.machine, self.image, self.perms, self.attr,
                self.base, self.size)
        ms.write_struct(section)

        # Write the physmem item.
        item = CellEnvPhysmemItem(self.machine, self.image, self.segment,
                self.offset, self.size, self.paddr)
        item.write_struct(section)

class CellEnvPd(CellEnvironmentEntry):
    """
    Environment entry for a protection domain.

    The data structure for the entry is:

    struct okl4_pd {
        _okl4_mcnode_t *mem_container_list;
        okl4_kclist_t *parent_kclist;
        okl4_kspace_t *kspace;
        okl4_memsec_t utcb_memsec;
        okl4_pd_t *dict_next;
        okl4_thread_t *thread_pool;
        okl4_bitmap_allocator_t *thread_alloc;
        okl4_kcap_t default_pager;
        okl4_extension_t *extension;
        struct {
            okl4_bitmap_allocator_t *kclistid_pool;
            okl4_bitmap_allocator_t *kspaceid_pool;
            okl4_virtmem_pool_t *virtmem_pool;
            okl4_virtmem_item_t utcb_area_virt_item;
            okl4_kspaceid_t kspace_id;
            okl4_kclistid_t kclist_id;
            okl4_utcb_area_t *utcb_area;
            okl4_kclist_t *kclist;
            okl4_kspace_t kspace_mem;
        };
    };
    """

    def __init__(self, machine, image, space, max_threads, page_size, elf):
        CellEnvironmentEntry.__init__(self, machine, image)

        # pointers + sizeof memsec + sizeof kcap + sizeof virtmem item
        # + sizeof kspaceid + sizeof kclistid + sizeof kspace_t
        self.pd_num_words = 12 + 15 + 1 + 4 + 1 + 1 + 6

        self.max_threads = max_threads
        self.thread_pool_num_words = max_threads * 8
        self.thread_allocator = CellEnvBitmapAllocator(machine, image,
                size = max_threads)
        self.page_size = page_size
        self.kspace = 0
        self.kclist = 0
        self.space = space
        self.utcb_memsec = None
        self.mem_list = None
        self.thread_pool_offset = 0
        self.thread_alloc_offset = 0
        self.utcb_memsec_offset = 0
        self.utcb_list_node_offset = 0
        self.elf = elf

    def attach_memsection(self, memsection, perms):
        memsection.owner = self
        memsection.list_node.attach_perms = perms
        memsection.list_node.payload = memsection
        memsection.list_node.range = memsection.range
        current = self.mem_list
        previous = None
        while (not (current is None)) and (current.range.base < memsection.range.base):
            previous = current
            current = current.next
        if previous is None:
            self.mem_list = memsection.list_node
        else:
            previous.next = memsection.list_node
        memsection.list_node.next = current

    def set_offsets(self, sizeof_word):
        self.thread_pool_offset = self.pd_num_words * sizeof_word
        self.thread_alloc_offset = (self.pd_num_words + self.thread_pool_num_words) * sizeof_word
        self.utcb_memsec_offset = 3 * sizeof_word # 3 pointers
        self.utcb_list_node_offset = (3 + 5) * sizeof_word # 3 pointers + sizeof mem_container (utcb_memsec.super)

    def write_struct(self, section):
        """Write the binary form of the PD struct."""

        # Construct utcb memsection
        if self.utcb_memsec == None: # Create and attach the memsection only once at first round
            self.utcb_memsec = CellEnvMemsec(self.machine, self.image,
                    _0(self.space.utcb.virt_addr), _0(self.space.utcb.size),
                    self.page_size, self.space.utcb.attach,
                    self.space.utcb.cache_policy)
            self.utcb_memsec.set_callbacks(self.elf, "_okl4_utcb_memsec_lookup", "_okl4_utcb_memsec_map", "_okl4_utcb_memsec_destroy")
            self.attach_memsection(self.utcb_memsec, PF_R | PF_W | PF_X)
        self.utcb_memsec.set_virt_base(self.virt_base + self.utcb_memsec_offset)
        self.utcb_memsec.list_node.set_virt_base(self.virt_base + self.utcb_list_node_offset)

        # Write mem container list.
        section.write_word(self.mem_list.virt_base)
        # Write parent clist (usually copied over from the attr).
        section.write_word(self.kclist)
        # Write kspace pointer.
        section.write_word(self.kspace)
        # Write out the memsec.
        self.utcb_memsec.write_struct(section)
        # Write out dict_next pointer.
        section.write_word(0)
        # Write out the thread pool pointer.
        section.write_word(self.virt_base + self.thread_pool_offset)
        # Write out the thread alloc pointer.
        section.write_word(self.virt_base + self.thread_alloc_offset)
        # Write out the default pager.
        section.write_word(0)
        # Write out the extension pointer.
        section.write_word(0)

        # Initialise the _init struct to 0 because it is not needed when the PD
        # is weaved.
        # Write out the kclistid pool pointer.
        section.write_word(0)
        # Write out the kspaceid pool pointer.
        section.write_word(0)
        # Write out the virtmem pool pointer.
        section.write_word(0)
        # Write out the utcb area virt item.
        for i in range(4):
            section.write_word(0)
        # Write out the kspace id.
        section.write_word(0)
        # Write out the kclist id.
        section.write_word(0)
        # Write out the utcb area pointer.
        section.write_word(0)
        # Write out the kclist pointer.
        section.write_word(0)
        # Write out the okl4 space.
        for i in range(6):
            section.write_word(0)
        # Initialise the thread pool memory to 0.
        for i in range(self.thread_pool_num_words):
            section.write_word(0)
        # Write out the thread bitmap allocator
        self.thread_allocator.write_struct(section)

class CellEnvNanoPd(CellEnvironmentEntry):
    """
    Environment entry for a Nano protection domain.

    Nano protection domains are significantly simpler than Micro,
    and hence we treat is as a seperate entity instead of attempting
    to conditionally construct them.

    The data structure for the entry is:

    struct okl4_pd {
        okl4_kthread_t *thread_pool;
        okl4_bitmap_allocator_t *thread_alloc;
    };

    The structure is followed by an array of kthread_t structures, one per
    thread.
    """

    def __init__(self, machine, image, max_threads):
        CellEnvironmentEntry.__init__(self, machine, image)

        # Save paramters for later use
        self.max_threads = max_threads

        # Get the size of our subelements
        self.bitmap_allocator = CellEnvBitmapAllocator(machine, image,
                size = max_threads)
        self.kthread_pool_size = max_threads * 4

    def write_struct(self, section):
        """Write the binary form of the Nano PD struct."""

        # Write out the pointer to the pool of kthreads
        section.write_word(self.virt_base
                + self.bitmap_allocator.get_size()
                + 2 * self.machine.sizeof_word)

        # Write out the pointer to the bitmap allocator
        section.write_word(self.virt_base
                + 2 * self.machine.sizeof_word)

        # Write out the bitmap allocator
        self.bitmap_allocator.write_struct(section)

        # Write out empty space for the threads
        for i in range(self.kthread_pool_size):
            section.write_word(0)

class CellEnvZone(CellEnvironmentEntry):
    """
    Environment entry for a zone.

    The data structure for the entry is:

    struct okl4_zone {
        struct _okl4_mem_container super;
        _okl4_mcnode_t *mem_container_list;
        okl4_word_t pd_ref_count;
        _okl4_mcnode_t *mcnode_pool;
        okl4_bitmap_allocator_t *mcnode_alloc;
    };
    """

    def __init__(self, machine, image, base, size, max_pds):
        """ # sizeof pointers + ref count + sizeof container """
        CellEnvironmentEntry.__init__(self, machine, image)
        self.base = base
        self.size = size
        self.max_pds = max_pds
        self.bitmap_allocator = CellEnvBitmapAllocator(machine,
                image, size = max_pds)

    def write_struct(self, section):

        # Write the mem container.
        section.write_word(self.base) # super.range.base
        section.write_word(self.size) # super.range.size
        section.write_word(0) # super.range.total_size
        section.write_word(0) # super.range.next

        # Write out the list pointer.
        section.write_word(0)

        # Write the reference count.
        section.write_word(0)

        # Write out the pointer to the pool structure.
        section.write_word(self.virt_base + 8 * self.machine.sizeof_word)

        # Write out the pointer to the allocator.  We do this by 
        # skipping over the zone and the mcnode pool structure.
        section.write_word(self.virt_base + 8 * self.machine.sizeof_word +
            self.max_pds * 2 * self.machine.sizeof_word)

        # Write the mcnode.
        for _ in range(0, self.max_pds):
            for _ in range(0, 2):
                section.write_word(0)

        # Write the bitmap allocator structure.
        self.bitmap_allocator.write_struct(section)

class CellEnvSegments(CellEnvironmentEntry):
    """
    Environment entry for the list of segments

    The data structure for the entry is:

    struct okl4_env_segment {
        okl4_word_t virt_addr;
        okl4_word_t segment;
        okl4_word_t offset;
        okl4_word_t size;
        okl4_word_t rights;
        okl4_word_t cache_policy;
    }

    struct okl4_env_segments {
        okl4_word_t num_segments;
        okl4_env_segment segments[];
    }
    """
    def __init__(self, machine, image, mappings, min_page_size):
        CellEnvironmentEntry.__init__(self, machine, image)
        self.mappings = mappings
        self.min_page_size = min_page_size

    def write_struct(self, section):
        """Write the binary form of the segment mapping struct."""
        section.write_word(len(self.mappings))

        for (num, _, attrs) in self.mappings:
            #
            # Align the segments to nearest page boundaries. If we have to move
            # 'virt_addr' backwards, we need to compensate by increasing 'size'
            # by the same amount.
            #
            # This is also done in kernel.py when writing out kernel mapping
            # operations.
            #
            if attrs.virt_addr is None:
                virt_addr = -1
                size = _0(attrs.size)
            else:
                virt_addr = align_down(_0(attrs.virt_addr), self.min_page_size)
                alignment_slack = _0(attrs.virt_addr) - virt_addr
                size = _0(attrs.size) + alignment_slack

            size = align_up(size, self.min_page_size)

            section.write_word(virt_addr)
            section.write_word(num) # seg
            section.write_word(0) # offset
            section.write_word(size)
            section.write_word(attrs.attach)
            section.write_word(attrs.cache_policy)

class CellEnvSegmentEntry(CellEnvironmentEntry):
    """
    Environment entry for a segment entry, this entry gives the segment
    number that can be used to look up the segments table.

    word_t seg_no;
    """
    def __init__(self, machine, image, mapping):
        CellEnvironmentEntry.__init__(self, machine, image)
        self.mapping = mapping

    def write_struct(self, section):
        """Write the binary form of the segment struct."""
        section.write_word(self.mapping[0]) # Seg number

class CellEnvValueEntry(CellEnvironmentEntry):
    """
    Environment entry for a simple word.

    The data structure for the entry is:

    word_t value;
    """
    def __init__(self, machine, image, value):
        CellEnvironmentEntry.__init__(self, machine, image)

        self.value = value

    def write_struct(self, section):
        """Write the binary form of the struct."""
        section.write_word(self.value)

class CellEnvCapEntry(CellEnvironmentEntry):
    """
    Environment entry a cap.  A cap is a single word, but is
    value is calculated when laying out the kernel init script.

    The data structure for the entry is:

    word_t value;
    """
    def __init__(self, machine, image, cap_name, attach):
        CellEnvironmentEntry.__init__(self, machine, image)

        self.cap_name  = cap_name
        self.attach    = attach
        self.cap       = None
        self.dest_cap = None

    def write_struct(self, section):
        """Write the binary form of the struct."""
        assert self.cap is not None
        section.write_word(_0(self.dest_cap.get_cap_slot()))

class CellEnvElfSegmentEntry(CellEnvironmentEntry):
    """
    Environment entry for elf segment information.

    The data structure for the entry is:

    struct elf_segment {
        word_t vaddr;
        word_t flags;
        word_t paddr;
        word_t offset;
        word_t filesz;
        word_t memsz;
    }
    """
    def __init__(self, machine, image, seg):
        CellEnvironmentEntry.__init__(self, machine, image)

        self.seg = seg

    def write_struct(self, section):
        """Write the binary form of the struct."""

        paddr = 0
        offset = 0
        if self.seg.prepared:
            paddr = self.seg.paddr
            offset = self.seg.offset

        section.write_word(self.seg.vaddr)
        section.write_word(self.seg.flags)
        section.write_word(paddr)
        section.write_word(offset)
        section.write_word(self.seg.get_filesz())
        section.write_word(self.seg.get_memsz())

class CellEnvElfInfoEntry(CellEnvironmentEntry):
    """
    Environment entry for elf info

    The data structure for the entry is:

    struct elf_info {
        word_t type;
        word_t entry;
    }
    """
    def __init__(self, machine, image, type, entry):
        CellEnvironmentEntry.__init__(self, machine, image)

        self.type = type
        self.entry = entry

    def write_struct(self, section):
        """Write the binary form of the struct."""

        section.write_word(self.type)
        section.write_word(self.entry)

class CellEnvDeviceIrqList(CellEnvironmentEntry):
    """
    Environment entry to describe the IRQs belonging
    to a device.

    struct device_irq_list {
        word_t num_irqs;
        word_t irqs[];
    }
    """

    def __init__(self, machine, image, num_irqs, irqs):
        CellEnvironmentEntry.__init__(self, machine, image)

        self.num_irqs = num_irqs
        self.irqs = irqs

    def write_struct(self, section):
        """Write the binary form of the struct."""

        section.write_word(self.num_irqs)
        for irq in self.irqs:
            section.write_word(irq)

class CellEnvArgList(CellEnvironmentEntry):
    """
    Environment entry to describe the command line argument list.

    struct okl4_env_args {
        int   argc;
        char *argv;
    }

    +-----------------+
    | argc            |  sizeof(argc) = 1 word
    +-----------------+
    | argv[0..n]      |  sizeof(argv) = argc + 1 word
    |   ...           |
    |   NULL          |
    +-----------------+
    | args[0..n]      |  sizeof(args) =
    |  - "arg0\0"     |    sum of [sizeof(args[i]) + 1 byte] for i = 0..n
    |  - "arg1\0"     |
    |   ...           |
    +-----------------+
    """
    def __init__(self, machine, image, num_args, args):
        CellEnvironmentEntry.__init__(self, machine, image)
        self.num_args = num_args
        self.args = args

    def write_struct(self, section):
        """Write the binary form of the struct."""
        # First, write argc.
        section.write_word(self.num_args)

        # Second, write argv pointers.
        base = self.virt_base
        offset = (self.num_args + 2) * self.machine.sizeof_word
        for arg in self.args:
            section.write_word(base + offset)
            offset += len(arg) + 1

        # NULL terminate the array.
        section.write_word(0)

        # Third, write the argv strings.
        section.write_strings(self.args)

        # Finally, write any padding if needed.
        while (offset % self.machine.sizeof_word != 0):
            section.write_bytes(0, 1)
            offset += 1

ENV_HEADER_MAGIC = 0xb2a4

class BinaryWriter:
    """Conveinience functions for writing binary data to some output stream.
    Users should inherit from this class and implement 'write_output'."""

    def __init__(self, machine, image):
        if machine.word_size == 64:
            word_char = 'Q'
            signed_word_char = 'q'
        else:
            word_char = 'I'
            signed_word_char = 'i'

        self.format_word = image.endianess + word_char
        self.format_signed_word = image.endianess + signed_word_char
        self.format_string = image.endianess + '%ds'

    def encode_word(self, word):
        """Encode a single word."""
        return pack(self.format_word, word)

    def encode_signed_word(self, word):
        """Encode a single signed word."""
        return pack(self.format_signed_word, word)

    def encode_string(self, string):
        """Encode a string.  The string will be NUL terminated."""
        string += '\0'
        return pack(self.format_string % len(string), string)

    def encode_byte(self, value):
        """Encode a byte."""
        return pack('b', value)

    def write_word(self, word):
        """Write out an encoded word."""
        if word < 0:
            self._write_output(self.encode_signed_word(word))
        else:
            self._write_output(self.encode_word(word))

    def write_ptrs(self, ptr_array):
        """Write out a pointer array."""
        for (key_ptr, value_ptr) in ptr_array:
            self.write_word(key_ptr)
            self.write_word(value_ptr)

    def write_strings(self, strings):
        """
        Write out a list of strings.

        The strings will be byte aligned in memory.
        """
        for string in strings:
            self._write_output(self.encode_string(string))

    def write_bytes(self, value, count):
        """
        Write out count number of bytes with the same value.
        Useful for padding.
        """
        self._write_output(self.encode_byte(value) * count)

    def _write_output(self, value):
        """Write the given string to the output stream."""
        raise NotImplementedError

class MeasuringBinaryWriter(BinaryWriter):
    def __init__(self, machine, image):
        BinaryWriter.__init__(self, machine, image)
        self.count = 0

    def _write_output(self, value):
        self.count += len(value)

    def get_bytes_written(self):
        return self.count

# Class for writing out the environment to the image
class CellEnvSection(BinaryWriter):
    """
    Class for the binary representation of the environment.

    Data structure for the cell environment:

    +-----------------+
    | Env Header      |
    |  - ID           | (One word)
    |  - Num objects  |
    +-----------------+
    | ptrs            |
    |  - string_ptr   | (<num_entries> 2 word structs)
    |  - entry_ptr    |
    | ...             |
    +-----------------+
    | Entry structs   | (Header with version for each?)
    | ...             |
    +-----------------+
    | Null terminated |
    | strings         |
    | ...             |
    +-----------------+
    """
    def __init__(self, machine, image, output):
        BinaryWriter.__init__(self, machine, image)
        self.output = output
        self.machine = machine

    def encode_hdr(self, magic, count):
        """Encode the environment header."""
        if self.machine.word_size == 64:
            return self.encode_word(magic << 32 | count)
        else:
            assert self.machine.word_size == 32
            return self.encode_word(magic << 16 | count)

    def write_env_header(self, num_items):
        """Write out the environment header."""
        self.output.write(self.encode_hdr(ENV_HEADER_MAGIC, num_items))

    def _write_output(self, value):
        self.output.write(value)

    def write_environment(self, base, entries, pd_list, space_list, clist,
                          space_utcb_list):
        """
        Generate the binary form of the environment.

        The environment structure contains pointers to internal
        objects.  These addresses are calculated relative to the base
        address of the environment's memsection.
        """


        # Calculate the size of the header and the pointer arrays.
        header_size = self.machine.sizeof_word
        ptrs_size   = self.machine.sizeof_pointer * len(entries)

        # Calculate the total size of the entries.
        struct_size = sum([entry.get_size() for entry in entries.values()])

        entry_addr = base + header_size + 2 * ptrs_size
        entry_array = []

        string_addr = base + header_size + 2 * ptrs_size + struct_size
        string_array = []

        for (key, entry) in entries.iteritems():
            string_array.append(string_addr)
            string_addr += len(key) + 1 # with NUL terminator

            entry_array.append(entry_addr)
            entry.set_virt_base(entry_addr)
            entry_addr += entry.get_size()

        size = len(space_utcb_list)

        for (kspace, utcb_area) in space_utcb_list:
            kspace.utcb_area = utcb_area.virt_base

        for (kspace, thread_list) in space_list:
            size = len(thread_list)
            kspace.kthread_list = 0 # Print a null if no threads

            if size > 0:
                kspace.kthread_list = thread_list[0].virt_base

            for i in range(0, size):
                kthread = thread_list[i]
                kthread.kspace = kspace.virt_base

                if (i + 1) < size:
                    kthread.next = thread_list[i+1].virt_base

        size = len(space_list)

        for i in range(0, size):
            (kspace, _) = space_list[i]

            if (i + 1) < size:
                (next_kspace, _) = space_list[i+1]
                kspace.next = next_kspace.virt_base

            kspace.kclist = clist.virt_base

        if len(space_list) > 0:
            (kspace, _) = space_list[0]
            clist.kspace_list = kspace.virt_base

        for (pd, kspace) in pd_list:
            pd.kclist = kspace.kclist
            pd.kspace = kspace.virt_base
            pd.set_offsets(self.machine.sizeof_word)

        # At last we can write out the contents.
        # Start with the header.
        num_items = len(string_array)
        self.write_env_header(num_items)

        # and then the pointer table.
        self.write_ptrs(zip(string_array, entry_array))

        for entry in entries.values():
            # Write out the sturct.
            initial_size = self.output.tell()
            entry.write_struct(self)
            end_size = self.output.tell()

            # Ensure that the sizes reported by the structure were correct
            assert end_size - initial_size == entry.get_size()

            # Ensure that every entry is word aligned.
            assert self.output.tell() % self.machine.sizeof_word == 0,      \
                    "Binary output is not word aligned after writing this"  \
                    " struct."

        # Finally the strings, which are stored straight after the
        # entries.
        self.write_strings(entries.keys())
