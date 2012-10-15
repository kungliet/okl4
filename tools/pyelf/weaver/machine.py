#############################################################################
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
Properties of the target machine.
"""

import math, os
from weaver import MergeError
from weaver.kernel_micro_elf import find_elfweaver_info
from weaver.machine_xml import Machine_el
from weaver.parse_spec import XmlCollector

        # Map between cache policy strings and L4 attribute types.
CACHE_POLICIES = None

CACHE_PERM_BITS = None

CACHE_PERMS = None

def _assert_no_overlaps(mem_map):
    """Raise an exception if any of the memory regions in the map overlap."""
    # Extract all of the memory regions and record where they came from.
    # Sort by base address
    mem_array = sorted([(base, size, name)
                        for (name, mem) in mem_map.items()
                        for (base, size, _) in mem])

    highest_memory = -1L
    highest = ()

    # Check for overlaps and give a meaningful error message if there
    # is one.
    for (base, size, name) in mem_array:
        if base <= highest_memory:
            raise MergeError, \
                  'The machine memory region 0x%x--0x%x (size 0x%x) in ' \
                  '"%s" overlaps with region 0x%x--0x%x (size 0x%x) in ' \
                  '"%s".' % (base, base + size - 1, size, name,
                             highest[0], highest[0] + highest[1] -1,
                             highest[1], highest[2])
        else:
            highest_memory  = base + size - 1
            highest = (base, size, name)

class PhysicalDevice(object):
    """
    Description of a physical device.  A simple interface to the data
    in the XML file.
    """

    def __init__(self, name):
        self.name            = name
        self.physical_mem    = {}
        self.interrupt       = {}

    def add_physical_mem(self, name, mem):
        """
        Add a named list of physical memory ranges. The memory ranges must be a
        list of tuples of the form (base, size, rights, cache_policy)
        """
        self.physical_mem[name] = mem
        # _assert_no_overlaps(self.physical_mem)

    def add_interrupt(self, name, number):
        """Add a named interrupts."""
        # print "New interrupt %d added" % number
        self.interrupt[name] = number

    def get_physical_mem(self, name):
        """
        Get the named list of physical memory ranges.

        Returns None if the named memory range does not exist.
        """
        return self.physical_mem.get(name)

    def get_interrupt(self, name):
        """
        Get the named interrupt.

        Raises a MergeError if the named interrupt does not exist.
        """
        if not self.interrupt.has_key(name):
            raise MergeError, "Interrupt called %s not found." % name

        return self.interrupt[name]

#pylint: disable-msg=R0902
# We have a large number of instance attributes
class Machine(XmlCollector):
    """Description of the image's target machine."""

    element = Machine_el

    def __init__(self):
        XmlCollector.__init__(self)

        # Default if no machine is given in XML
        self._word_size = None
        self._heap_proximity = None

        self.page_sizes = [0x1000]
        self.word_size = 32
        self.cpu             = ""
        self.virtual_mem = {}
        self.physical_mem = {}
        self.physical_device = {}
        # 64M is the ARM requirement.
        self.kernel_heap_proximity = 64 * 1024 * 1024
        self.kernel_heap_align = None
        self.arch_max_spaces = None

        global CACHE_POLICIES
        global CACHE_PERM_BITS
        global CACHE_PERMS

        CACHE_POLICIES = {
            'default'        : None,
            'cached'         : None,
            'uncached'       : None,
            'writeback'      : None,
            'writethrough'   : None,
            'coherent'       : None,
            'device'         : None,
            'writecombining' : None,
            'strong'         : None
        }
        CACHE_PERM_BITS = {
            'uncached'      : 0x1,
            'cached'        : 0x2,
            'iomemory'      : 0x4,
            'iomemoryshared': 0x8,
            'writethrough'  : 0x10,
            'writeback'     : 0x20,
            'shared'        : 0x40,
            'nonshared'     : 0x80,
            'custom'        : 0x100,
            'strong'        : 0x200,
            'buffered'      : 0x400,
            'all'           : 0xffff
        }
        CACHE_PERMS = {
            'uncached'      : None,
            'cached'        : None,
            'iomemory'      : None,
            'iomemoryshared': None,
            'writethrough'  : None,
            'writeback'     : None,
            'shared'        : None,
            'nonshared'     : None,
            'custom'        : None,
            'strong'        : None,
            'buffered'      : None
        }

    def _set_word_size(self, word_size):
        """ Set the word size in bits. Must be a multiple of 8."""
        assert word_size % 8 == 0
        self._word_size = word_size
    def _get_word_size(self):
        """ Return the word size in bits. """
        return self._word_size
    word_size = property(_get_word_size, _set_word_size)

    def _get_sizeof_word(self):
        """Get the word size in bytes."""
        return self.word_size / 8
    sizeof_word = property(_get_sizeof_word)

    def _sizeof_pointer(self):
        """Return the size of a pointer on the machine."""
        return self.word_size / 8
    sizeof_pointer = property(_sizeof_pointer)

    def set_cache_attributes(self, elf):
        """Load the cache attribute values from kernel elf file."""
        cacheinfo = find_elfweaver_info(elf)
#        cacheinfo.output()
        self.add_cache_policies(cacheinfo.get_attributes())
        self.add_cache_permissions(cacheinfo.get_arch_cache_perms())
        self.arch_max_spaces = cacheinfo.get_arch_max_spaces()

    def set_kernel_heap_proximity(self, distance):
        """
        The maximum distance, in bytes, that the kernel heap can be
        from the kernel.  If the distance is None, then the current
        value is not changed.
        """
        if distance is not None:
            self._heap_proximity = distance

    def get_kernel_heap_proximity(self):
        """
        Return the maximum distance, in bytes, that the kernel heap
        can be from the kernel.
        """
        return self._heap_proximity
    kernel_heap_proximity = property(get_kernel_heap_proximity,
                                     set_kernel_heap_proximity)

    def set_page_sizes(self, sizes):
        """Set the allowed page sizes for the machine."""
        self.page_sizes = sizes[:]

        # Sort in descending order to make min_page_size() and
        # superpage_alignment's lives easier.
        self.page_sizes.sort(reverse=True)

    def add_cache_policies(self, policies):
        """Add extra cache policy values to the default map."""
        CACHE_POLICIES.update(policies)

    def add_cache_permissions(self, perms):
        """Add the machine cache permissions."""
        CACHE_PERMS.update(perms)

    def check_cache_permissions(self, attr, perms):
        """
        Check if the cache attributes given are allowed according to the
        permissions.
        """
        assert(attr is not None)
        assert(perms is not None)

        ## XXX: It would be better to assert CACHE_PERMS[] is not None, but
        #  this function gets called by mark and alloc before the CACHE_PERMS
        #  are set by the elfweaver_info code.
        if CACHE_PERMS['uncached'] is None:
            return True

        attr_perms = 0

        for name, (mask, comp) in CACHE_PERMS.iteritems():
            assert CACHE_PERM_BITS.get(name, None) is not None
            check = attr & mask

            if check == comp:
                attr_perms |= CACHE_PERM_BITS[name]

        if ((attr_perms == CACHE_PERM_BITS['shared']) or
            (attr_perms == CACHE_PERM_BITS['nonshared']) or
            (attr_perms == 0)):
            return False

        if (attr_perms != (attr_perms & perms)):
            return False

        return True

    def cache_policy_to_str(self, attr):
        """Convert cache policy to a string"""
        for name, val in CACHE_POLICIES.iteritems():
            if val == attr:
                return name

        return "custom: %x" % (attr)

    def cache_perms_to_str(self, perms):
        """Convert the cache permissions into a string."""
        if perms == 0xffff:
            return "all"

        return ', '.join([name for name, bits in CACHE_PERM_BITS.iteritems()
                          if name != "all" and (bits & perms) != 0])

    def get_cache_policy(self, attr):
        """
        Return the numeric value of a named cache policy or raise
        an exception.
        """
        try:
            assert CACHE_POLICIES.get(attr, None) is not None
            return CACHE_POLICIES[attr]
        except:
            raise MergeError, ("Unknown cache policy: '%s'." % attr)

    def add_virtual_mem(self, name, mem):
        """
        Add a named list of virtual memory ranges. The memory ranges must be a
        list of tuples of the form (base, size, mem_type)
        """
        self.virtual_mem[name] = mem
        _assert_no_overlaps(self.virtual_mem)

    def add_physical_mem(self, name, mem):
        """
        Add a named list of physical memory ranges. The memory ranges must be a
        list of tuples of the form (base, size, mem_type)
        """
        self.physical_mem[name] = mem
        _assert_no_overlaps(self.physical_mem)

    def add_phys_device(self, name):
        """
        Create and add a named physical device.
        Returns the newly created device.
        """
        self.physical_device[name] = PhysicalDevice(name)
        return self.physical_device[name]

    def get_phys_device(self, name):
        """Get the details of the named physical device."""
        return self.physical_device[name]

    def get_virtual_memory(self, name):
        """Get the names list of virtual memory ranges."""
        if not self.virtual_mem.has_key(name):
            raise MergeError, "Virtual memory called %s not found." % name

        return self.virtual_mem[name]

    def get_physical_memory(self, name):
        """Get the names list of physical memory ranges."""
        # First look for the physical memory in devices...
        for dev in self.physical_device.itervalues():
            phys_mem = dev.get_physical_mem(name)

            if phys_mem is not None:
                return phys_mem

        # ... then try to look for it in the machine
        if not self.physical_mem.has_key(name):
            raise MergeError, "Physical memory called %s not found." % name

        return self.physical_mem[name]

    def set_cpu(self, cpu):
        """Set the CPU of the target platform."""
        self.cpu = cpu

    def get_cpu(self):
        """Return the CPU of the target platform."""
        return self.cpu

    def min_page_size(self):
        """Return the smallest allowable page size."""
        return self.page_sizes[-1]

    def max_page_size(self):
        """Return the largest allowable page size."""
        return self.page_sizes[0]

    def superpage_alignment(self, size):
        """Calculate the biggest alignment supported on the current
        machine for the given size.

        Items smaller than the min page size are min page size aligned
        and items larger than the max page size are max page sise
        aligned. Items within this range are rounded down to the nearest
        page size.
        """
        # Align small objects to the page size.
        if size <= self.min_page_size():
            return self.min_page_size()

        # Align large items to the max page size.
        if size > self.max_page_size():
            return self.max_page_size()

        for page_size in self.page_sizes:
            if page_size <= size:
                return page_size

    def natural_alignment(self, size):
        """
        Return the natural alignment for the size.  Eg 64K regions are aligned
        to 64K boundaries.

        Items smaller than the min page size are min page size aligned
        and items larger than the max page size are max page sise
        aligned. Items within this range are rounded down to the nearest power
        of two.
        """

        # Align small objects to the page size.
        if size <= self.min_page_size():
            return self.min_page_size()

        # Align large items to the max page size.
        if size > self.max_page_size():
            return self.max_page_size()

        return 1 << int(math.log(size, 2))

    def map_mem_rights_str(self, string):
        """Map the string memory rights to a memory rights value."""
        return_rights = 0

        for right in string.split(","):
            right = right.strip()

            if not CACHE_PERM_BITS.has_key(right):
                raise MergeError, "Unknown memory rights %s" % right

            return_rights |= CACHE_PERM_BITS[right]

        return return_rights

    def map_mem_rights(self, region_el):
        """Map the string memory rights to a memory rights value."""
        mem_rights = getattr(region_el, "rights", "all")

        return self.map_mem_rights_str(mem_rights)

    def collect_xml(self, machine_el, ignore_name, **_):
        """Collect the attributes of the machine element."""

        # If the file attribute is specified, then read the full version
        # of the machine element from that file.
        if hasattr(machine_el, "file"):
            machine_el = \
                       Machine_el.parse_xml_file(os.path.join(machine_el._path,
                                                              machine_el.file))

        self.word_size = machine_el.find_child("word_size").size

        self.set_cpu(machine_el.find_child("cpu").name)

        self.set_page_sizes([el.size for el in
                             machine_el.find_children("page_size")])

        self.add_cache_policies([(el.name, el.value) for el in
                                 machine_el.find_children("cache_policy")])

        attrs = machine_el.find_child("kernel_heap_attrs")
        if attrs is not None:
            self.kernel_heap_proximity = getattr(attrs, 'distance', None)
            self.kernel_heap_align = getattr(attrs, 'align', None)

        for p_el in machine_el.find_children("physical_memory"):
            if not ignore_name.match(p_el.name):
                mem = [(el.base, el.size, self.map_mem_rights(el))
                       for el in p_el.find_children("region")]
                self.add_physical_mem(p_el.name, mem)

        for v_el in machine_el.find_children("virtual_memory"):
            if not ignore_name.match(v_el.name):
                mem = [(el.base, el.size, self.map_mem_rights(el))
                       for el in v_el.find_children("region")]
                self.add_virtual_mem(v_el.name, mem)

        for d_el in machine_el.find_children("phys_device"):
            if not ignore_name.match(d_el.name):
                device = self.add_phys_device(d_el.name)
                for p_el in d_el.find_children("physical_memory"):
                    if not ignore_name.match(p_el.name):
                        mem = [(el.base, el.size, self.map_mem_rights(el),
                                getattr(el, 'cache_policy', None))
                               for el in p_el.find_children("region")]
                        device.add_physical_mem(p_el.name, mem)
                for i_el in d_el.find_children("interrupt"):
                    if not ignore_name.match(i_el.name):
                        device.add_interrupt(i_el.name, i_el.number)
