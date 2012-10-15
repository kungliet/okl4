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

"""Collection of items that will initialise the kernel."""

import os.path
import glob
import subprocess
import tempfile
import atexit
from elf.util import align_up
from elf.segnames import add_segment_names, get_segment_names
from weaver import MergeError
from weaver.kernel_nano import NANO_KERNEL_API_VERSION, NanoKernel
from weaver.kernel_micro import MICRO_KERNEL_API_VERSION, MicroKernel
from elf.core import UnpreparedElfFile
from elf.constants import ET_EXEC
from weaver.segments_xml import collect_elf_segments, collect_patches, \
     Patch_el
from weaver.kernel_xml import get_symbol, Kernel_el
from weaver.parse_spec import XmlCollector
from weaver.cap import LocalCap
from weaver.link import link

# The default number of caps in a clist.
DEFAULT_MAX_CAPS = 1024
DEFAULT_TOTAL_THREADS = 1024

KERNEL_TAGS = set(["file", "sdk", "configuration", "platform", "linker_script",
                   "linker_wrapper", "kernel", "soc", "libs"])
KERNEL_PRELINK_TAGS = set(["file"])
KERNEL_SDKLINK_TAGS = set(["sdk", "configuration", "platform"])
KERNEL_LINK_TAGS = set(["linker_script", "linker_wrapper", "kernel", "soc",
                        "libs"])

# Different kernels supported by ElfWeaver
KERNEL_NANO = "nano"
KERNEL_MICRO = "micro"


#--------------------------------------------------------------------------
# Representation of the kernel.

class Kernel(XmlCollector):
    """
    Kernel initialisation parameters built from the XML file.
    These parameters are then merged into the kconfig section in the
    ELF file.
    """

    DEFAULT_MAX_CAPS = DEFAULT_MAX_CAPS
    element = Kernel_el

    class Cell(object):
        """
        Kernel information pertaining to a cell.
        """

        class Space(object):
            """An address space within a cell."""

            class Thread(object):
                """ A thread within a space """
                def __init__(self, entry, user_start, utcb, priority,
                        create = True):

                    assert(priority is not None)

                    self.cap_slot   = 0
                    self.priority   = priority
                    self.stack      = None
                    self.stack_ms   = None
                    self.entry      = entry
                    self.user_start = user_start
                    self.create     = create
                    self.utcb       = utcb
                    self.offset     = None

                def get_sp(self):
                    """
                    Return the stack pointer for the thread.
                    """
                    virt_addr = self.stack.virt_addr
                    if virt_addr is None:
                        virt_addr = 0

                    size = self.stack.size
                    if size is None:
                        size = 0

                    return virt_addr + size

                def get_stack_ms(self):
                    """
                    Return the stack memsection for the thread.
                    """
                    return self.stack_ms

            class Mutex(object):
                """A mutex within a space."""
                def __init__(self, name, create = True):
                    self.name   = name
                    self.offset = 0
                    self.create = create

            class Cap(LocalCap):
                """A cap slot that references another object."""
                def __init__(self, obj):
                    self.clist     = None
                    self.cap_slot  = None
                    self.obj       = obj

                def get_cap_slot(self):
                    return self.cap_slot

            # for class Space.
            def __init__(self, ns_node, env_prefix, is_privileged = False,
                         plat_control = False,
                         max_spaces = None,
                         max_clists = None,
                         max_mutexes = None,
                         max_threads = None,
                         max_phys_segs = 0,
                         max_priority = None,
                         create = True):
                self.ns_node       = ns_node
                self.env_prefix    = env_prefix
                self.create        = create
                self.is_privileged = is_privileged
                self.plat_control  = plat_control
                self.id            = 0
                self.utcb          = None
                self.space_id_base = 0
                self.max_spaces    = max_spaces
                self.clist_id_base = 0
                self.max_clists    = max_clists
                self.mutex_id_base = 0
                self.max_mutexes   = max_mutexes
                self.max_threads   = max_threads
                self.max_phys_segs = max_phys_segs
                self.total_threads = 0
                self.seg_id        = 0
                self.max_priority  = max_priority
                self.phys_segs     = []
                self.threads       = []
                self.mappings      = []
                self.irqs          = []
                self.mutexes       = []
                self.ipc_caps      = []
                self.mutex_caps    = []

            def calc_maxes(self):
                """
                Calculate the correct maximum values in the space.
                """
                # Can always access his own space.
                if self.max_spaces is None or self.max_spaces == 0:
                    self.max_spaces = 1

                # Can always access his own clist.
                if self.max_clists is None or self.max_clists == 0:
                    self.max_clists = 1

                # Can access max or created mutexes.
                if self.max_mutexes is None or \
                   self.max_mutexes < len(self.mutexes):
                    self.max_mutexes = len(self.mutexes)

                # Can access max or created physical segments.
                if self.max_phys_segs is None or \
                   self.max_phys_segs < len(self.phys_segs):
                    self.max_phys_segs = len(self.phys_segs)

                num_threads = len(self.threads)

                if self.max_threads is None:
                    self.max_threads = num_threads

                if self.max_threads < num_threads:
                    raise MergeError, \
                          "Number of threads created in space \"%s\" (%d) exceed the maximum allowed (%d)." % \
                          (self.ns_node.abs_name(), num_threads, self.max_threads)

            def register_thread(self, entry, user_start, utcb, priority,
                    create = True):
                assert self.create or not create

                if self.max_priority is not None and \
                        priority > self.max_priority:
                    raise MergeError("Tried to create thread with priority %s," \
                    " but space (%s) max priority is %s." % (priority,
                                                             self.ns_node.abs_name(),
                                                             self.max_priority))

                thread = self.Thread(entry, user_start, utcb, priority, create)
                self.threads.append(thread)

                return thread

            def get_static_threads(self):
                """
                Return all threads that should be created
                statically.
                """
                return [t for t in self.threads if t.create]

            def register_irq(self, irq):
                """Assign an irq number to the space."""
                self.irqs.append(irq)

            def register_mapping(self, attrs):
                mapping = (self.seg_id,
                           self.env_prefix + attrs.ns_node.abs_name()[len(self.ns_node.abs_name()):].replace("/", "_"),
                           attrs)
                self.seg_id += 1
                self.max_phys_segs += 1
                self.mappings.append(mapping)
                return mapping

            def register_mutex(self, name, create = True):
                assert self.create or not create

                mutex = self.Mutex(name, create)
                self.mutexes.append(mutex)
                return mutex

            def get_static_mutexes(self):
                """
                Return all mutexes that should be created
                statically.
                """
                return [m for m in self.mutexes if m.create]

            def add_ipc_cap(self, target):
                """
                Create a cap to the target thread.
                """
                cap = self.Cap(target)
                self.ipc_caps.append(cap)

                return cap

            def add_mutex_cap(self, target):
                """
                Create a cap to the target mutex.
                """
                cap = self.Cap(target)
                self.mutex_caps.append(cap)

                return cap

        # For class Cell
        def __init__(self, kernel_heap,
                     max_caps = None, max_priority = None,
                     kernel_max_priority = None):
            if max_caps is None:
                max_caps = DEFAULT_MAX_CAPS

            self.heap_size   = kernel_heap
            self.heap_phys_base = None
            self.clist_id    = 0
            self.max_threads = 0
            self.max_caps    = max_caps
            self.spaces      = []
            self.mutexes     = []
            self.env         = None
            self.space_list  = None
            self.caps_list   = None
            self.mutex_list  = None
            self.clist_offset = None
            self.max_priority = max_priority
            self.kernel_max_priority = kernel_max_priority
            self.total_spaces = 0

            assert self.max_priority <= self.kernel_max_priority

        def register_space(self, ns_node, env_prefix,
                           is_privileged = False,
                           plat_control = False,
                           max_clists = None,
                           max_spaces = None,
                           max_mutexes = None,
                           max_threads = None,
                           max_phys_segs = 0,
                           max_priority = None,
                           create = True):

            if max_priority is None:
                max_priority = self.max_priority
            elif max_priority > self.kernel_max_priority:
                raise MergeError, "Maximum priority of the Space %s (%s) " \
                      "exceeds the kernel max priority (%s)" % (ns_node.abs_name(),
                                     max_priority, self.kernel_max_priority)

            space = self.Space(ns_node, env_prefix, is_privileged, plat_control,
                               max_spaces, max_clists, max_mutexes, max_threads,
                               max_phys_segs, max_priority, create)
            self.spaces.append(space)
            return space

        def get_static_spaces(self):
            """
            Return all spaces that should be created statically.
            """
            return [s for s in self.spaces if s.create]

        def get_mr1(self):
            if self.env.memsect.attrs == None or \
                    self.env.memsect.attrs.virt_addr == None:
                return 0xfee1dead

            return self.env.memsect.attrs.virt_addr

    # For class Kernel
    def __init__(self, machine, section = "kernel.roinit"):
        self.kernel        = None
        self.machine       = machine
        self.section       = section
        self.configs       = []
        self.total_spaces  = 0
        self.total_mutexes = 0
        self.total_clists  = 0
        self.total_threads = None
        self.base_segment  = None
        self.cells         = {}
        self.heap_attrs    = None
        self.thread_array_base  = None
        self.thread_array_count = 0
        self.thread_array_size  = None
        self.dynamic_heap_size  = False

    def register_heap(self, attrs):
        self.heap_attrs = attrs

        if attrs.size is None:
            self.dynamic_heap_size = True
            attrs.size = 0
        else:
            self.dynamic_heap_size = False

    def register_cell(self, cell_tag, kernel_heap,
                     max_caps = DEFAULT_MAX_CAPS,
                     max_priority = None):
        """Register a cell with the kernel."""
        if max_priority is None:
            max_priority = self.kernel.MAX_PRIORITY
        elif max_priority > self.kernel.MAX_PRIORITY:
            raise MergeError, "Maximum priority of the Cell %s (%s) " \
                  "exceeds the kernel max priority (%s)" % (cell_tag,
                                   max_priority, self.kernel.MAX_PRIORITY)

        cell = self.Cell(kernel_heap, max_caps, max_priority,
                                           self.kernel.MAX_PRIORITY)
        self.cells[cell_tag] = cell

        if self.dynamic_heap_size:
            self.heap_attrs.size += kernel_heap

        return cell

    def get_cell(self, cell_tag):
        """Return the details of the named cell."""
        return self.cells.get(cell_tag, None)

    def add_config(self, key, value):
        """Record a configuration property."""

        if key == "threads":
            kernel_max = self.kernel.ABSOLUTE_MAX_THREADS
            if value > kernel_max:
                raise MergeError, \
                      "Maximum number of threads (%d) exceeded: %d" % \
                      (kernel_max, value)

            self.total_threads = value

    def calc_thread_array_sizes(self):
        self.thread_array_count = (self.total_threads)

        self.thread_array_size = self.thread_array_count  * \
                                 self.machine.sizeof_pointer

        if self.dynamic_heap_size:
            self.heap_attrs.size += align_up(self.thread_array_size,
                                             self.machine.min_page_size())

    def layout_cells_pre(self):
        # This is called before dynamic elements of cells as we should
        # be able to decide the number of cells, clists and mutexes at that
        # time

        clist_id = 0
        space_id = 0
        mutex_id = 0

        self.total_spaces  = 0
        self.total_clists  = 0
        self.total_mutexes = 0

        total_threads = 0

        for cell in self.cells.values():
            cap_slot = 0

            cell.clist_id = clist_id

            # Store the base id for the space, caps and mutex lists
            space_list_base = space_id
            cap_list_base = clist_id
            mutex_list_base = mutex_id

            # Init the number of used entries in the lists
            space_list_used = 0
            cap_list_used = 0
            mutex_list_used = len(cell.mutexes)

            cell_threads = 0

            tmp_cell_max_spaces = 0

            for space in cell.get_static_spaces():
                space.calc_maxes()
                if space.is_privileged and len(cell.spaces) > space.max_spaces:
                    space.max_spaces = len(cell.spaces)

                space.id = space_id
                space.space_id_base = space_id
                space_id += space.max_spaces
                if space.is_privileged:
                    cell.total_spaces += 1
                    self.total_spaces += space.max_spaces
                    tmp_cell_max_spaces = space.max_spaces
                else:
                    cell.total_spaces += space.max_spaces

                space.clist_id_base = clist_id
                clist_id += space.max_clists
                self.total_clists += space.max_clists

                space.mutex_id_base = mutex_id
                mutex_id += space.max_mutexes
                self.total_mutexes += space.max_mutexes

                for thread in space.get_static_threads():
                    thread.cap_slot = cap_slot
                    cap_slot += 1

                cell_threads += space.max_threads

                assigned_mutex_id = space.mutex_id_base
                for mutex in space.get_static_mutexes():
                    mutex.id = assigned_mutex_id
                    assigned_mutex_id += 1

                for cap in space.ipc_caps:
                    cap.clist = cell
                    cap.cap_slot = cap_slot
                    cap_slot += 1

                for cap in space.mutex_caps:
                    cap.clist = cell
                    cap.cap_slot = cap_slot
                    cap_slot += 1

                space_list_used += 1
                cap_list_used += cap_slot
                mutex_list_used += len(space.mutexes)

            if tmp_cell_max_spaces != 0 and \
               tmp_cell_max_spaces < cell.total_spaces:
                raise MergeError("Tried to allocate %s spaces, but cell max spaces is %s."
                % (cell.total_spaces, tmp_cell_max_spaces))

            if cell.max_threads < cell_threads:
                cell.max_threads = cell_threads

            cell.space_list = (space_list_base, space_id - space_list_base,
                    space_list_used)
            cell.cap_list = (cap_list_base, clist_id - cap_list_base,
                    cap_list_used)
            cell.mutex_list = (mutex_list_base, mutex_id - mutex_list_base,
                    mutex_list_used)

            total_threads += cell_threads

        if self.total_threads < total_threads:
            raise MergeError, \
                  "Total number of threads (%d) exceeds kernel maximum of %d." % \
                  (total_threads, self.total_threads)

        # Now do any kernel specific layout
        self.kernel.layout_cells_pre(self)


    def layout_cells_post(self, image):
        # This must be called after image.layout() and before the cell
        # init scripts are written out.
        #
        # Do any required layout here

        self.kernel.layout_cells_post(self, image)

        if self.machine.arch_max_spaces != 0 and \
                self.total_spaces > self.machine.arch_max_spaces:
            raise MergeError("Tried to allocate %d spaces, kernel only allows %d."
            % (self.total_spaces, self.machine.arch_max_spaces))

    def create_dynamic_segments(self, namespace, image, machine, pools):
        """ Do final thread array and heap calculations. """
        self.kernel.create_dynamic_segments(self, namespace, image,
                                            machine, pools, self.base_segment)

        # We need to mark the heap since its size is now known
        pools.mark_physical(self.heap_attrs.abs_name(),
                            self.heap_attrs.phys_addr,
                            self.heap_attrs.size,
                            self.heap_attrs.cache_policy)

    def generate_init_script(self, image, machine):
        self.kernel.generate_init_script(self, image.elf, image, machine)

    def update_elf(self, elf, image, machine):
        """Perform any required updates to the kernel elf."""
        self.kernel.update_elf(self, elf, image, machine)

    def collect_use_devices(self, el, image, machine, pools):
        """Collect information about devices used by the kernel."""

        # Initialise the kernel virtual pool if necessary
        self.kernel.init_kernel_pool(machine, pools)

        # Iterate through devices used by the kernel, store their memory ranges
        for device_el in el.find_children("use_device"):
            dev = machine.get_phys_device(device_el.name)
            # print "driver %s has keys: %s" % (dev.name, dev.physical_mem.keys())
            self.kernel.add_device_mem(dev, image, machine, pools)

        if self.kernel.devices:
            # collect the memsections
            device_ms = []
            for (_, mem) in self.kernel.devices:
                device_ms.extend(mem)
            image.add_group(None, device_ms)

    def collect_xml(self, kernel_el, kernel_heap_size, namespace, image,
                    machine, pools):
        """Collect the attributes of the kernel element."""

        (elf, kernel_file, kernel_tmp) = get_kernel_file(kernel_el, machine)

        if elf.elf_type != ET_EXEC:
            raise MergeError, \
                  "All the merged ELF files must be of EXEC type."

        # New namespace for objects living in the kernel.
        kernel_namespace = namespace.add_namespace('kernel')

        # We setup our kernel type depending on the api version number
        kern_ver = check_api_versions(elf)
        if kern_ver == NANO_KERNEL_API_VERSION:
            self.kernel = NanoKernel(elf, kernel_namespace, machine)
        else:
            self.kernel = MicroKernel(kernel_namespace)

        assert self.total_threads is None
        self.total_threads = self.kernel.ABSOLUTE_MAX_THREADS

        # Record the default pools for cells to use if they don't have
        # defaults set.
        virtpool = getattr(kernel_el, "virtpool", None)
        physpool = getattr(kernel_el, "physpool", None)
        image.set_attrs_stack(def_virt = virtpool,
                              def_phys = physpool)
        pools.set_default_pools(virtpool, physpool)

        segment_els = kernel_el.find_children("segment")
        patch_els   = kernel_el.find_children("patch")

        machine.set_cache_attributes(elf)

        image.set_kernel(elf)
        segs = collect_elf_segments(elf, image.KERNEL, segment_els,
                                    'kernel', self.kernel.segment_drops,
                                    kernel_namespace, image, machine, pools)
        # Set base segment
        self.base_segment = self.kernel.get_base_segment(elf, segs, image)

        if self.base_segment.attrs.phys_addr is not None and \
            (self.base_segment.attrs.phys_addr & ((1024 * 1024) - 1)) != 0:
            raise MergeError("Physical address of %s must be 1MB aligned!" %
                                    self.base_segment.attrs.abs_name())

        elf = elf.prepare(elf.wordsize, elf.endianess)
        # The extra_patches attr may be added by a plugin.
        for patch in getattr(Kernel_el, "extra_patches", []):
            addr = get_symbol(elf, patch[0], True)
            if addr != None:
                addr = int(addr[0])+ int(patch[1])
                new_patch = Patch_el(address=hex(addr), bytes=patch[2],
                                     value=patch[3])
                patch_els.append(new_patch)

        collect_patches(elf, patch_els, kernel_file, image)

        # Collect the config elements, some of these relate to
        # heap and thread array, so do it first
        config_el = kernel_el.find_child("config")
        if config_el is not None:
            for option in config_el.find_children("option"):
                self.add_config(option.key, option.value)

        heap_attrs       = image.new_attrs(namespace.root.add_namespace("kernel_heap"))
        heap_attrs.align = machine.kernel_heap_align
        heap_el          = kernel_el.find_child("heap")
        if heap_el is not None:
            heap_attrs.phys_addr = getattr(heap_el, 'phys_addr',
                                           heap_attrs.phys_addr)
            heap_attrs.size      = getattr(heap_el, 'size', heap_attrs.size)
            heap_attrs.align     = getattr(heap_el, 'align', heap_attrs.align)

        # Override the size with the command line value, if present.
        if kernel_heap_size != 0:
            heap_attrs.size = kernel_heap_size

        # Nano needs a heap size set early on
        # Micro remains unassigned since it will get dynamically calculated later
        if not heap_attrs.size and isinstance(self.kernel, NanoKernel):
            heap_attrs.size = self.kernel.DEFAULT_KERNEL_HEAP_SIZE

        self.heap = image.set_kernel_heap(heap_attrs, machine,
                                          pools, self.base_segment.segment,
                                          isinstance(self.kernel, NanoKernel))
        image.add_group(machine.kernel_heap_proximity, (self.base_segment, self.heap))
        self.register_heap(heap_attrs)

        if isinstance(self.kernel, MicroKernel):
            image.utcb_size = self.kernel.get_utcb_size(elf, image)

        self.collect_use_devices(kernel_el, image, machine, pools)

        if kernel_tmp:
            kernel_tmp.close()

def check_api_versions(kernel):
    """ Return the kernel API version number.  """
    kern_ver = get_symbol(kernel, "kernel_api_version", True)
    if kern_ver:
        addr = kern_ver[0]
        size = kern_ver[1]
    else:
        return None
    kern_ver = kernel.get_value(addr, size, kernel.endianess)

    # Check that both the kernel and soc api versions match
    if kern_ver == MICRO_KERNEL_API_VERSION:
        (addr, size) = get_symbol(kernel, "soc_api_version")
        soc_ver = kernel.get_value(addr, size, kernel.endianess)
        if soc_ver == None:
            raise MergeError("Unable to locate soc api symbol")

        if kern_ver != soc_ver:
            raise MergeError("Kernel api version %d doesn't match soc api version %d" % (kern_ver, soc_ver))

    return kern_ver

def get_kernel_version(kernel_el, machine):
    (elf, _, _) = get_kernel_file(kernel_el, machine)
    (addr, size) = get_symbol(elf, "kernel_api_version")
    kern_ver = elf.get_value(addr, size, elf.endianess)
    if kern_ver == None:
        raise MergeError("Unable to locate kernel api symbol")
    return kern_ver

# Returns a fully linked kernel.o this may either be;
#
# file=<kernel.o> - we just return the specified file
# OR
# sdk=<sdk-path> AND configuration=<kernel-config> AND platform=<platform-file>
#  - We need to both construct the full paths for the kernel and platform and
#    shell out to the linker.sh or linker.bat to built the kernel then return
#    that file
# OR
# linker=<linker-wrapper> AND kernel=<partial kernel.o> AND soc=<soc-file> AND
# libs=[<libs>] - Shell out to the linker but no need for directory expansion
def get_kernel_file(kernel_el, machine):
    def relative(file):
        """Return a path taking relativity to XML include directories into account"""
        return os.path.join(kernel_el._path, file)
    if has_kernel_attr(kernel_el, KERNEL_PRELINK_TAGS):
        elf = UnpreparedElfFile(filename = relative(kernel_el.file))
        return (elf, kernel_el.file, None)
    elif has_kernel_attr(kernel_el, KERNEL_SDKLINK_TAGS):
        kernel_dir = os.path.join(relative(kernel_el.sdk), "kernel", machine.get_cpu())
        object_dir = os.path.join(kernel_dir, kernel_el.configuration, "object")
        lib_dir = os.path.join(kernel_dir, kernel_el.configuration, "libs")

        linker_script = os.path.abspath(os.path.join(object_dir, "linker.lds"))
        linker_wrapper = os.path.abspath(os.path.join(object_dir, "linker.sh"))
        kernel = os.path.abspath(os.path.join(object_dir, machine.get_cpu() +
                                              ".o"))
        soc = os.path.abspath(os.path.join(object_dir, kernel_el.platform +
                                           ".o"))
        libs = [os.path.abspath(lib) for lib in glob.glob(os.path.join(lib_dir,
                                                          "*.a"))]

        return link_kernel(kernel_el, linker_script, linker_wrapper, kernel,
                           soc, libs)
    elif has_kernel_attr(kernel_el, KERNEL_LINK_TAGS):
        libs = [relative(lib.strip()) for lib in kernel_el.libs.split(",")]

        return link_kernel(kernel_el, relative(kernel_el.linker_script),
                           relative(kernel_el.linker_wrapper),
                           relative(kernel_el.kernel),
                           relative(kernel_el.soc),
                           libs)

    raise MergeError("Invalid kernel tags; a prelinked kernel, SDK information or linking information must be provided")

# Given a set of tags check if the kernel element contains all the tags and
# that it does not contain any other tags
def has_kernel_attr(kernel_el, test_tags):
    for tag in test_tags:
        if not hasattr(kernel_el, tag):
            break
    else:
        for tag in KERNEL_TAGS.difference(test_tags):
            if hasattr(kernel_el, tag):
                raise MergeError("Found unexpected kernel tag %s when %s has already been specificed" % (tag, test_tags))

        return True

    return False

# Given a linker, kernel, platform and set of libraries shell out to the
# linker and return the generated fully linked kernel.o location
def link_kernel(kernel_el, linker_script, linker_wrapper, kernel, soc, libraries):
    link_type = get_linker_type(linker_script)

    kernel_tmp = None
    if link_type == 'rvct':
        # Current elfweaver linker does not support RVCT so shell out to an
        # external linker
        kernel_tmp = tempfile.NamedTemporaryFile()

        # Remove the file at exit.
        atexit.register(os.remove, kernel_tmp.name)

        command = [os.path.abspath(linker_wrapper), kernel, kernel_tmp.name,
                   linker_script, soc] + libraries

        ret = subprocess.Popen(command, cwd = os.path.dirname(linker_script)).wait()

        if ret != 0:
            raise MergeError("Failed to link kernel, return code %d" % ret)

        elf = UnpreparedElfFile(kernel_tmp.name)
        kernel_out_name = kernel_tmp.name
    else:
        elf = link([kernel, soc])

        # Just getting a random temp file name, there must be a nicer way
        # to do this?
        tmp = tempfile.NamedTemporaryFile()

        # Remove the file at exit.
        atexit.register(os.remove, tmp.name)

        kernel_out_name = tmp.name
        tmp.close()

        kernel_out = elf.prepare(elf.wordsize, elf.endianess)
        kernel_out.to_filename(kernel_out_name)

        elf = UnpreparedElfFile(kernel_out_name)

    # Kernel is linked, now add the segment names (as per old elfadorn)
    # Kernel elements seg_names overwrites defaults
    if hasattr(kernel_el, "seg_names"):
        seglist = [seg.strip() for seg in kernel_el.seg_names.split(",")]
    else:
        seglist = None

    link_type = get_linker_type(linker_script)

    if link_type == "rvct":
        scripts = [linker_script]
    else:
        scripts = []
    add_segment_names(elf, get_segment_names(elf, seglist, scripts, link_type))

    return elf, kernel_out_name, kernel_tmp

def get_linker_type(linker_script):
    buffer = open(linker_script, "r").read()

    if "NOCOMPRESS" in buffer:
        return "rvct"
    elif "SECTIONS" in buffer:
        return "gnu"
    else:
        raise MergeError("Unkown linker.lds type")
