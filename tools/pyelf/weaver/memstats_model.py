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
This module contains classes representing the concepts stored in
the memory stats XML file.
"""
from parsers.cxxfilt.cxxfilt import demangle
import sys

page_size = 4096

class ElfProgram(object):
    """
    Class to take the bits of an elf image and translate them into the
    memstats model.
    """

    def __init__(self, name, memstats):
        self.name              = name
        self.section_list      = []
        self.segment_list      = []
        self.func_list         = []
        self.data_list         = []
        self.section_count     = 0

        self._memstats = memstats

        if self.name == "kernel":
            self.cell = self._memstats.env.add_kernel()
            self.space = self.cell.add_space("kspace")
            self.kernel_vm = self._memstats.resources.add_vm("kspc")
            self.space.set_vm(self.kernel_vm['id'])
            self.program = self.space.create_program("kernel")
        else:
            self.program = None

    # Quick and dirty equality operator.
    def __eq__(self, prog):
        return self.name == prog.name \
               and self.static_size == prog.static_size \
               and self.segment_count == prog.segment_count \
               and self.rounded_size == prog.rounded_size

    def add_section(self, name, addr, size):
        """Add a section to this program."""
        phys_addr = addr - self.current_seg['virt'] + self.current_seg['phys']
        new_sec = self._memstats.resources.add_section(phys_addr, size)
        self.current_seg['sections'].append(new_sec['id'])

        if self.name == "kernel":
            mapping_id = self.kernel_vm['ids'].id()
            self.kernel_vm['list'].append((mapping_id, addr, size))
            self.program.add_psec(name, new_sec['id'], mapping_id)
        else:
            # NOTE: at this stage we know that this section will appear
            # against the listed program, but we dont know what the
            # vitrual mem resource is, nor do we know the ids.  We won't
            # know that until we read the init script, which comes much
            # later.
            #
            if self.program is None:
                self.program = \
                             self._memstats.env.add_unassigned_program(self.name)

            self.program.add_expected_psec(name, phys_addr, addr, size)

    def add_segment(self, number, phys, virt, filesz, memsz, secs):
        self.current_seg = \
                         self._memstats.resources.add_segment(list(), phys,
                                                              virt, filesz,
                                                              memsz)

    def add_func_obj(self, name, size):
        """Add a text symbol to this program, escape it's name."""
        if self.program:
            self.program.add_function_object(demangle(name), size)

    def add_data_obj(self, name, size):
        """Add a data symbol to this program, escape it's name"""
        if self.program:
            self.program.add_data_object(demangle(name), size)

    def parse_xml(self, prog):
        """Parse a 'program' XML element"""
        self.segment_count = prog.num_segments
        s_sz = prog.find_child("static_sizes")
        self.static_size  = int(s_sz.find_child("static_size").size)
        self.rounded_size = int(s_sz.find_child("static_size_rounded").size)

        for section in prog.find_child("section_list").find_children("section"):
            self.add_section(section.name, section.size)

        objects = prog.find_child("largest_objects")

        if objects is not None:
            for elm in [oel for oel in objects.find_children("object")
                        if oel.type == "text"]:
                self.add_func_obj(elm.name, int(elm.size))

            for elm in [oel for oel in objects.find_children("object")
                        if oel.type == "data"]:
                self.add_data_obj(elm.name, int(elm.size))


class InitScript(object):
    """
    Class to take the objects from the init script and translate them into
    the relevant bits of the memory statistics model.
    """
    def __init__(self, memstats, sizes, wordsize):
        self._memstats = memstats
        self._current_cell = None
        self._sizes = sizes
        self._wordsize = wordsize
        self._vm_ids = IdList("vm_pool")
        self._mappings = {} # used to store segments until the mapping comes
                            # along

    def add_heap(self, base, size, chunksize, blocksize):
        """Record a new heap being created by the init script."""
        # Currently a heap denotes a new cell, so let's set things up
        # accordingly.  We also need to create a new physical section
        # for the heap.
        if self._current_cell is None:
            kernel_heap = True
        else:
            kernel_heap = False

        self._current_cell = self._memstats.env.add_cell()
        new_section = self._memstats.resources.add_section(base, size)
        self._current_mempool = \
                              self._memstats.resources.add_heap(new_section['id'],
                                                                chunksize,
                                                                blocksize)
        self._current_cell.add_heap(self._current_mempool)

        if kernel_heap:
            self._memstats.env.kernel().add_heap(self._current_mempool)

    def allocate(self, group, size):
        self._current_mempool.allocate(group, size)

    def add_spaceid_list(self, count):
        self._spaceid_list  = \
            self._memstats.resources.add_id_pool("spaceid",
                                                 self._current_mempool.id,
                                                 count)
        self.allocate("spaceids", count * self._wordsize)

    def add_clistid_list(self, count):
        self._clistid_list = \
            self._memstats.resources.add_id_pool("clistid",
                                                 self._current_mempool.id,
                                                 count)
        self.allocate("clistids", count * self._wordsize)

    def add_mutexid_list(self, count):
        self._mutexid_list = \
            self._memstats.resources.add_id_pool("mutexid",
                                                 self._current_mempool.id,
                                                 count)
        self.allocate("mutexids", count * self._wordsize)

    def add_global_thread_handles(self, phys, size):
        new_section = \
            self._memstats.resources.add_section(phys,
                                                 size * self._wordsize)
        self._thread_handles = \
            self._memstats.resources.add_id_pool("thread_handles",
                                                 new_section['id'],
                                                 size)
        self._thread_handles_range_id = \
            self._thread_handles.add_range(0, size)[0]

    def create_clist(self, list_id, size):
        self._current_clist = \
            self._memstats.resources.add_id_pool("capabilities%d" % list_id,
                                                 self._current_mempool.id,
                                                 size)
        #
        # We only have single usage of clists currently
        #
        new_range = self._current_clist.add_range(0, size)
        self._current_cell.add_id_range("caps", new_range[0])

        self.allocate("root_clist",
                      self._sizes.clist_size + size * self._sizes.cap_size)

    def create_space(self,
                     space_info,
                     clist_info,
                     mutex_info,
                     utcb_base,
                     has_kresources,
                     max_phys_seg):
        if len (self._memstats.env.pending_space_names) > 0:
            name = self._memstats.env.pending_space_names.pop()
        else:
            name = ""

        self._current_space = self._current_cell.add_space(name)
        new_vm = self._memstats.resources.add_vm(self._vm_ids.id())
        self._current_space.set_vm(new_vm['id'])
        self._utcb_virt = utcb_base
        #
        # Create ranges in id pools based on the values in space_info,
        # clist_info, mutex_info.
        #
        # Map all of handes if has_kresources is true.
        #
        base, size = space_info

        if size != 0:
            new_range = self._spaceid_list.add_range(base, size)
            self._current_space.space_range_id = new_range[0]

        base, size = clist_info

        if size != 0:
            new_range = self._clistid_list.add_range(base, size)
            self._current_space.clist_range_id = new_range[0]
            self._current_clist.parent = new_range[0]

        base, size = mutex_info

        if size != 0:
            new_range = self._mutexid_list.add_range(base, size)
            self._current_space.mutex_range_id = new_range[0]

        if has_kresources:
            self._current_space.thread_range_id = self._thread_handles_range_id
        #
        # clear current program
        #
        self._current_program = None

        #
        # Need to allocate for:
        # - space_t
        # - page directory
        # - segment list
        #
        machine_name = self._memstats.resources.phys_mem['machine']
        bs = self._sizes.small_object_blocksize
        cs = self._sizes.kmem_chunksize
        self._current_mempool.allocate("space", self._sizes.space_size)

        if machine_name.find("ARM") == -1:
            self._current_mempool.allocate("pgtab", bs)
        else:
            self._current_mempool.allocate("l0_allocator", bs)

        self._current_pgtable = Pagetable(1024 * 1024 * 4,
                                          self._current_mempool,
                                          cs,
                                          bs,
                                          self._sizes.pgtable_top_size)

        new_vm['pgtable'] = self._current_pgtable

        segment_list_size = self._sizes.segment_list_size + \
                            self._wordsize * max_phys_seg
        if segment_list_size > cs:
            self._current_mempool.allocate("physseg_list",
                                           ((segment_list_size / cs) + 1) * cs)
        else:
            self._current_mempool.allocate("physseg_list", cs)

#    def create_thread(self, utcb_addr):
    def create_thread(self, _):
        """Record a new thread created by the init script."""
        list_id = None

        for lst in self._current_cell._id_range_list:
            if lst[0] == "caps":
                list_id = lst[1]
                break

        assert (list_id is not None)
        self._current_space.add_thread(self._current_mempool.id,
                                       self._thread_handles_range_id,
                                       list_id)
        #
        # Now to allocate memory for tcb and utcb.  Also, if there is
        # a new utcb block allocated, it needs to be mapped.
        #
        self._current_mempool.allocate("tcb", self._sizes.tcb_size)
        (_, utcb_blocks, utcb_size) = self._current_mempool.utcb_size()
        self._current_mempool.allocate("utcb", self._sizes.utcb_size)

        if utcb_size + self._sizes.utcb_size > utcb_blocks:
            bs = self._sizes.small_object_blocksize
            self._current_pgtable.add_mapping(self._utcb_virt, bs, 1)
            self._utcb_virt += bs

    def create_mutex(self):
        """Record a new mutex created by the init script."""
        list_id = None

        for lst in self._current_cell._id_range_list:
            if lst[0] == "caps":
                list_id = lst[1]
                break

        assert (list_id is not None)
        self._current_space.add_mutex(self._current_mempool.id,
                                      self._current_space.mutex_range_id,
                                      list_id)
        self._current_mempool.allocate("mutex", self._sizes.mutex_size)

    def create_segment(self, num, phys, size):
        """
        Record a segment created by the init script. No actual allocations
        are recorded until a corresponding map operaion is recorded.
        """
        self._mappings[num] = (self._current_space.vm(),
                               phys,
                               size)

    def map_memory(self, num, virt, size):
        """
        Record the mapping of memory by the init script.  There should
        have previously been a segment created to hold this.
        """
        details = self._mappings[num]
        assert(details is not None)
        (space_vm, phys, psize) = details

        # search for relevant section
        phys_section = None

        for sec in self._memstats.resources.section_list:
            if sec['address'] == phys:
                phys_section = sec
                break

        if phys_section is None:
            phys_section = self._memstats.resources.add_section(phys, size)

        vm_pool = self._memstats.resources.find_vm_pool(space_vm[1])
        assert(vm_pool is not None)
        #print "mapping 0x%x against pool %s" % (size, vm_pool['id'])
        mapping_id = vm_pool['ids'].id()
        vm_pool['list'].append((mapping_id, virt, size))
        vm_pool['pgtable'].add_mapping(virt, size, 1)

        #
        # Are we working against a current program?
        #

        if (self._current_program is None) or \
               (len(self._current_program.expected_list) == 0):
            self._current_program = \
                                  self._memstats.env.find_program_by_phys(phys)
            self._current_space.add_program(self._current_program)

        assert(self._current_program is not None)

        this_sec = None

        for sec in self._current_program.expected_list:
            if sec[1] == phys:
                this_sec = sec
                break

        if this_sec is not None:
            self._current_program.expected_list.remove(this_sec)
            self._current_program.add_psec(this_sec[0], phys_section['id'],
                                           space_vm[1])
        else:
            name = ""
            if self._current_program.extra_mappings is not None:
                for extra_map in self._current_program.extra_mappings:
                    ex_name, ex_phys, ex_virt, ex_size = extra_map

                    if ex_phys == phys_section['address']:
                        name = self._current_program._name + '.' + ex_name

            self._current_program.add_psec(name,
                                           phys_section['id'],
                                           space_vm[1])


class Pagetable(object):
    """
    Class to track mappings done in init script and calculate the resulting
    memory allocated.
    """
    def __init__(self, subrange, heap, chunk_size, block_size, top):
        self._subrange = subrange
        self._heap = heap
        self._chunk_size = chunk_size
        self._block_size = block_size
        self._subpages = set()
        # Allocate top-level block.
        # if top > 256 bytes, we are 2 level, otherwise 3 level.
        multilevel = (top <=  256)

        if multilevel:
            if top < self._chunk_size:
                top = self._block_size
            else:
                top = top + self._chunk_size - \
                    (chunk_size - (top % self._chunk_size))
            self._heap.allocate("l1_allocator", top)
        else:
            self._heap.allocate("pgtab", top)


    def add_mapping(self, virt, pgsize, npages):
        section_index = virt / self._subrange
        num_sections = ((npages * pgsize - 1) / self._subrange) + 1

        for section in range(section_index, section_index + num_sections):
            if not section in self._subpages:
                self._heap.allocate("pgtab", self._block_size)
                self._subpages.add(section)


class NanoHeap(object):
    """
    This class translates the information from the elfweaver-generated
    heap in the Nano kernel into the memstats model.
    """
    def __init__(self, memstats, heap_phys, tcb_sz, pgtbl_sz, queue_sz,
                 futex_sz):
        self._memstats = memstats

        #
        # Find the correct section for the "heap"
        #
        for section in memstats.resources.section_list:
            if section['address'] == heap_phys:
                memstats.env.kernel().add_nano_heap(section['id'])
                self._nheap = memstats.resources.add_nano_heap(section['id'])

        #
        # Heap "allocations"
        #
        self._nheap.allocate("pgtable", pgtbl_sz)
        self._nheap.allocate("tcb", tcb_sz)
        self._nheap.allocate("queue", queue_sz)
        self._nheap.allocate("futex", futex_sz)

        #
        # Create the single Cell and Space for Nano.
        #
        self._cell = self._memstats.env.add_cell()
        self._space = self._cell.add_space("")
        self._space.set_vm('kspc')

        for program in self._memstats.env.program_list:
            self._space.add_program(program)

    def process_mappings(self, decoded_mappings):
        vm_map = self._memstats.resources.find_vm_pool('kspc')
        assert(vm_map is not None)

        def in_map(map_list, virt):
            for sec in map_list:
                if sec[1] == virt:
                    return True

            return False

        def in_section(section, phys, pgsize):
            start = section['address']
            end = start + section['size'] - 1
            return ((phys >= start) and ((phys + pgsize - 1) <= end))

        phys_section = None

        for m in decoded_mappings:
#            phys, virt, pgsize, rwx, cache = m
            phys, virt, pgsize, _, _ = m

            #
            # What we get here is NOT a list of sections to be mapped,
            # but the individual pages mapped.  They need to be reconstructed
            # into the original section, if one exists. If there is no
            # original physical section, then create one.
            #
            if phys_section is not None:
                if in_section(phys_section, phys, pgsize):
                    continue

            # Remember, if we get here the current mapping wasn't in the
            # current section, so we start over.
            phys_section = None

            for sec in self._memstats.resources.section_list:
                if sec['address'] == phys:
                    phys_section = sec
                    break

            if phys_section is None:
                phys_section = self._memstats.resources.add_section(phys,
                                                                    pgsize)
            if not in_map(vm_map['list'], virt):
                #
                # Doesn't exist from analysing elf file; Either it is
                #   a) An expected mapping somewhere other than the kernel, or
                #   b) A new thingy.
                #
                new_id = vm_map['ids'].id()
                vm_map['list'].append((new_id, virt, pgsize))
                kernel_prog = \
                            self._memstats.env.kernel()._space._program_list[0]
                kernel_prog.add_psec("(weaved object)",
                                     phys_section['id'],
                                     new_id)

                def find_mapping(programs, phys, phys_section, new_id):
                    for program in programs:
                        this_sec = None
                        for sec in program.expected_list:
                            if sec[1] == phys:
                                this_sec = sec
                                break

                        if this_sec is not None:
                            program.expected_list.remove(this_sec)
                            program.add_psec(this_sec[0],
                                             phys_section['id'],
                                             new_id)

                            return
                        else:
                            #
                            # Note, if a mapping is not found here, it implies
                            # a vm (i.e. pagetable) being shared between
                            # spaces.  Nano does this.  Don't record it
                            # repeateldy.
                            #
                            if program.extra_mappings is not None:
                                for extra_map in program.extra_mappings:
                                    (ex_name, ex_phys, _, _) = extra_map

                                    if ex_phys == phys_section['address']:
                                        name = program._name + '.' + ex_name
                                        program.add_psec(name,
                                                         phys_section['id'],
                                                         new_id)
                                        return

                find_mapping(self._space.programs(), phys, phys_section, new_id)

    def process_threads(self, thread_table, spare_threads):
        #
        # Create id pool of correct size and apply to both kernel
        # and single cell space.  Then create thread in space.
        #
        total_len = len(thread_table) + spare_threads
        self._memstats.resources.add_id_pool("threads",
                                             "tcb",
                                             total_len)
        self._memstats.resources.add_block("tcb", self._nheap.id)

        for thread in thread_table:
            self._space.add_thread("tcb", "tcb", "tcb")

    def process_futexes(self, slots):
        self._memstats.resources.add_id_pool("futexes",
                                             self._nheap.id,
                                             slots)

class IdList(object):
    """ Simple class to produce unique ids for XML """

    m_ids = set()      # track all ids generated
    m_prefixes = set() # track all prefixes

    # Needed or things go bad in the unit testing.
    @classmethod
    def reset(cls):
        cls.m_ids = set()
        cls.m_prefixes = set()

    def __init__(self, prefix):
        assert(prefix not in IdList.m_prefixes)
        IdList.m_prefixes.add(prefix)
        self._prefix = prefix
        self._count = 0

    def id(self):
        retval = "%s%d" % (self._prefix, self._count)
        assert(retval not in IdList.m_ids)
        IdList.m_ids.add(retval)
        self._count += 1
        return retval

class Memstats(object):
    """ The top-level object representing the memory statistics collected. """
    templates = {}

    def __init__(self,
                 machine,
                 cpu,
                 poolname,
                 memsecs,
                 cell_names,
                 space_names,
                 mappings):
        self.env = Environment(cell_names, space_names, mappings)
        self.resources = Resources(machine, cpu, poolname)
        self.revision = None
        for sec in memsecs:
            sec_id, addr, size = sec
            self.resources.add_phys_mem(sec_id, addr, size)

    def set_revision(self, stats):
        """ Set the repository+changeset values"""
        if (stats[0] is not None) or (stats[1] is not None):
            self.revision = stats

    def format(self):
        output =  '<?xml version="1.0"?>\n' \
                  '<!DOCTYPE memstats SYSTEM "memstats-2.0.dtd">\n'
        output += '<memstats>\n'

        if self.revision is not None:
            rep, changeset = self.revision
            if rep is not None:
                rep_str = rep
            else:
                rep_str = ""

            if changeset is not None:
                changeset_str = changeset
            else:
                changeset_str = ""

            output += '  <revision repository="%s" changeset="%s" />\n' % \
                (rep_str, changeset_str)

        output += self.env.format()
        output += self.resources.format()

        output += "</memstats>\n"
        return output


class Environment(object):
    """
    <!ELEMENT environment (kernel cell+) >
    """
    def __init__(self, cell_names, space_names, mappings):
        self._cells = []
        self._pending_cell_names = cell_names
        self._pending_cell_names.reverse() #so pop() will work as desired.
        self.pending_space_names = space_names
        self.pending_space_names.reverse() #so pop() will work as desired.
        self._extra_mappings = {}
        for mapping in mappings:
            name, phys, virt, size = mapping
            index = name.index('/', 2)
            progname = name[1:index]
            secname = name[(index + 1):]
            if self._extra_mappings.has_key(progname):
                self._extra_mappings[progname].append((secname, phys, virt,
                                                       size))
            else:
                self._extra_mappings[progname] = [(secname, phys, virt, size)]
        self._kernel = None
        self._pool_ids = IdList("pool")
        self.program_list = []
        self.env_type = "unknown"

    def add_kernel(self):
        assert(self._kernel is None)
        self._kernel = Kernel()
        return self._kernel

    def kernel(self):
        return self._kernel

    def add_cell(self, name = None):
        if name is not None:
            cell_name = name
        elif len(self._pending_cell_names) > 0:
            cell_name = self._pending_cell_names.pop()
        else:
            cell_name = None
        new_cell = Cell(cell_name)
        self._cells.append(new_cell)
        return new_cell

    def cells(self):
        return self._cells

    def add_unassigned_program(self, name):
        new_program = Program(name)
        if self._extra_mappings.has_key(name):
            new_program.extra_mappings = self._extra_mappings[name]
        self.program_list.append(new_program)
        return new_program

    def check_unassigned_programs(self):
        for cell in self._cells:
            for space in cell._space_list:
                for program in space._program_list:
                    for unall_prog in self.program_list:
                        if program._name == unall_prog._name:
                            self.program_list.remove(unall_prog)
                            break
        if len(self.program_list) != 0:
            print >> sys.stderr, "\nWarning: Memory usage from the following " \
                  "programs in the ELF image has not been included.  They are" \
                  " started outside of the normal Cell initialisation process:"
            for prog in self.program_list:
                print >> sys.stderr, "  %s" % prog._name
            print

    def find_program_by_phys(self, phys):
        for program in self.program_list:
            for sec in program.expected_list:
                if sec[1] == phys: # we have a match
                    return program
        return None

    def format(self):
        assert(self._kernel is not None)
        self.check_unassigned_programs()

        output = '  <environment type="%s">\n' % self.env_type

        output += self._kernel.format()

        for cell in self._cells:
            output += cell.format()

        output += '  </environment>\n'
        return output

class Kernel(object):
    """
    <!ELEMENT kernel (id_range* pool* space) >
    <!ATTLIST id_range name CDATA #REQUIRED >
    <!ATTLIST id_range node IDREF #REQUIRED >

    Note that the id range needs the id_pool to already exist.

    <!ATTLIST pool id   ID    #REQUIRED >
    <!ATTLIST pool vmid IDREF #REQUIRED >

    Likewise the pool needs the vm resource to already exist.
   """
    def __init__(self):
        self._mempool_id = None
        self._id_range_list = []
        self._space = None
        self._name = "kernel"

    def add_id_range(self, name, id_pool):
        self._id_range_list.append((name, id_pool))

    def add_heap(self, mempool):
        self._mempool_id = mempool.id

    def add_nano_heap(self, sec_id):
        self._mempool_id = sec_id

    def add_space(self, name):
        if self._space is None:
            self._space = Space(name)
        return self._space

    def format(self):
        output =  '    <kernel>\n'
        output += '      <pool id="kheap" vmid="%s" />\n' % self._mempool_id
        for rnge in self._id_range_list:
            output += '      <id_range name="%s" node="%s" />\n' % rnge
        output += self._space.format()
        output += '    </kernel>\n'
        return output

class Cell(object):
    """
    <!ELEMENT cell (id_range* pool* space*) >
    <!ATTLIST cell name CDATA #OPTIONAL >
    """
    heap_ids = IdList("cell_heap_")

    def __init__(self, name):
        self._pool_ids = None
        self._space_list = []
        self._id_range_list = []
        self._name = name
        self._mempool = None # Nano doesn't have a heap.

    def add_id_range(self, name, id_pool):
        self._id_range_list.append((name, id_pool))

    def add_heap(self, mempool):
        self._mempool = (Cell.heap_ids.id(), mempool.id)

    def add_space(self, name):
        new_space = Space(name)
        self._space_list.append(new_space)
        return new_space

    def format(self):
        output =  '    <cell name="%s">\n' % self._name
        if self._mempool is not None:
            output += '      <pool id="%s" vmid="%s" />\n' % self._mempool
        for rnge in self._id_range_list:
            output += '      <id_range name="%s" node="%s" />\n' % rnge
        for space in self._space_list:
            output += space.format()
        output += '    </cell>\n'
        return output

class Space(object):
    """
    <!ELEMENT space (id_range* pool* program+ thread+ mutex*) >
    <!ATTLIST space name CDATA #REQUIRED >
    """
    vm_ids = IdList("vm")
    def __init__(self, name):
        self._program_list = []
        self._thread_list = []
        self._mutex_list = []
        self._name = name
        self._vm = None
        self.space_range_id = None
        self.clist_range_id = None
        self.mutex_range_id = None
        self.thread_range_id = None

    def set_vm(self, vm_pool_id):
        self._vm = (Space.vm_ids.id(), vm_pool_id)

    def vm(self):
        return self._vm

    def create_program(self, name):
        new_program = Program(name)
        self._program_list.append(new_program)
        return new_program

    def add_program(self, program):
        self._program_list.append(program)

    def programs(self):
        return self._program_list

    def add_thread(self, pool_id, handle_list, cap_list):
        self._thread_list.append((pool_id, handle_list, cap_list))

    def add_mutex(self, pool_id, mutex_list, cap_list):
        self._thread_list.append((pool_id, mutex_list, cap_list))

    def format(self):
        if self._name is None:
            outname = ""
        else:
            outname = self._name
        output =  '      <space name="%s">\n' % outname
        output += '        <pool id="%s" vmid="%s" />\n' %  self._vm

        if self.thread_range_id is not None:
            output += '        <id_range name="threads" node="%s" />\n' % \
                self.thread_range_id
        if self.space_range_id is not None:
            output += '        <id_range name="spaces" node="%s" />\n' % \
                self.space_range_id
        if self.clist_range_id is not None:
            output += '        <id_range name="clists" node="%s" />\n' % \
                self.clist_range_id
        if self.mutex_range_id is not None:
            output += '        <id_range name="mutexes" node="%s" />\n' % \
                self.mutex_range_id

        for prog in self._program_list:
            output += prog.format()

        for thread in self._thread_list:
            output += '        <thread pool_id="%s" handle_id="%s" ' \
                      'clist_id="%s" />\n' % thread

        for mutex in self._mutex_list:
            output += '        <mutex pool_id="%s" list_id="%s" ' \
                      'clist_id="%s" />\n' % mutex

        output += '      </space>\n'

        return output

class Program(object):
    def __init__(self, name):
        self._name = name
        self._psec_list = []
        self.expected_list = []
        self._func_list = []
        self._data_list = []
        self.extra_mappings = None

    def add_expected_psec(self, name, phys, virt, size):
        self.expected_list.append((name, phys, virt, size))

    def add_psec(self, name, phys, virt):
        self._psec_list.append((name, phys, virt))

    def add_function_object(self, name, size):
        self._func_list.append((name, size))

    def add_data_object(self, name, size):
        self._data_list.append((name, size))

    def format(self):
        output =  '        <program name="%s">\n' % self._name
        output += '          <psec_list>\n'

        for sec in self._psec_list:
            output += '            <psec name="%s" phys="%s" virt="%s" />\n' % \
                      sec

        output += '          </psec_list>\n'
        output += '          <object_list num_text="%d" num_data="%d">\n' % \
                  (len(self._func_list), len(self._data_list))

        for obj in self._func_list:
            output += '            <object type="text" name="%s" ' \
                      'size="%d"/>\n' % obj

        for obj in self._data_list:
            output += '            <object type="data" name="%s" ' \
                      'size="%d"/>\n' % obj

        output += '          </object_list>\n'
        output += '        </program>\n'

        return output

class Resources(object):
    """
    <!ELEMENT resources (phys_mem section_list segment_list ids pools) >
    """

    #
    # Note: due to everything currently being in one SAS, the
    # SAS generation logic is completely hard-coded.
    #
    sas_id = "sas1"

    def __init__(self, machine, cpu, poolname):
        machine = "%s" % machine
        if machine.find(':') != -1:
            machine = machine[machine.index(':') + 1:]
        self.phys_mem = {"machine"  : machine,
                         "cpu"      : cpu,
                         "poolname" : poolname,
                         "list"     : []}
        self.section_list = []
        self.segment_list = []
        self._ids = []
        self._pools = []
#        self._phys_ids = IdList("pm")
        self._sec_ids = IdList("phys")

    def add_phys_mem(self, id, addr, size):
        self.phys_mem['list'].append((id, addr, size))

    def add_section(self, addr, size):
        new_sec = {'id'      : self._sec_ids.id(),
                   'address' : addr,
                   'size'    : size}
        self.section_list.append(new_sec)
        return new_sec

    def add_segment(self, sections, phys, virt, filesz, memsz):
        new_seg = {'sections' : sections,
                   'phys'     : phys,
                   'virt'     : virt,
                   'filesz'   : filesz,
                   'memsz'    : memsz}
        self.segment_list.append(new_seg)
        return new_seg

    def add_id_pool(self, name, source, count):
        new_pool = IdPool(name, source, count)
        self._ids.append(new_pool)
        return new_pool

    def add_vm(self, id_prefix):
        new_vm = {'id'   : id_prefix,
                  'ids'  : IdList(id_prefix),
                  'list' : []}
        self._pools.append(new_vm)
        return new_vm

    def find_vm_pool(self, id):
        for pool in self._pools:
            if (type(pool) == dict) and (pool['id'] == id):
                return pool
        return None

    def add_heap(self, section_id, cs, bs):
        new_heap = Heap(section_id, cs, bs)
        self._pools.append(("mem_pool", new_heap))
        return new_heap

    def add_nano_heap(self, section_id):
        new_heap = Heap(section_id, 1, 1, "nano")
        self._pools.append(("mem_pool", new_heap))
        return new_heap

    def add_block(self, id, source_id):
        new_block = Block(id, source_id)
        self._pools.append(("mem_pool", new_block))

    def format(self):
        output = '  <resources>\n'

        output += '    <phys_mem machine="%s" name="%s" pool="%s">\n' % \
                  (self.phys_mem['machine'],
                   self.phys_mem['cpu'],
                   self.phys_mem['poolname'])
        for phys in self.phys_mem['list']:
            output += '      <section id="%s" address="0x%x" size="0x%x" ' \
                      '/>\n' % phys
        output += '    </phys_mem>\n'

        output += '    <section_list>\n'
        for sect in self.section_list:
            output += '      <section id="%(id)s" address="0x%(address)x" ' \
                      'size="0x%(size)x" />\n' % sect
        output += "    </section_list>\n"

        output += "    <segment_list>\n"
        for seg in self.segment_list:
            output += '      <segment sections="'
            for sect in seg['sections']:
                output += sect + ' '
            output += '"\n         phys="0x%(phys)x" virt="0x%(virt)x" ' \
                      'filesz="0x%(filesz)x" memsz="0x%(memsz)x"/>\n' % seg
        output += '    </segment_list>\n'

        output += '    <sas_list>\n'
        output += '      <sas id="%s" />\n' % Resources.sas_id
        output += '    </sas_list>\n'

        output += '    <ids>\n'
        for pool in self._ids:
            output += pool.format()
        output += '    </ids>\n'

        output += '    <pools>\n'
        for pool in self._pools:
            if (type(pool) == tuple) and (pool[0] == "mem_pool"):
                heap = pool[1]
                output += heap.format()
            elif type(pool) == dict:
                output += '      <virt_mem id="%s" sas_id="%s">\n' % \
                          (pool['id'], Resources.sas_id)
                for vmsec in pool['list']:
                    output += '        <vm_section id="%s" address="0x%x" ' \
                              'size="0x%x" />\n' % vmsec
                output += '      </virt_mem>\n'
        output += '    </pools>\n'

        output += "  </resources>\n"
        return output

class IdPool(object):
    def __init__ (self, name, source, total):
        self._name = name
        self._source = source
        self._total = total
        self._ids = IdList(name)
        self._ranges = []
        self.parent = None

    def add_range(self, start, size):
        new_range = (self._ids.id(), start, size)
        self._ranges.append(new_range)
        return new_range

    def format(self):
        if self.parent is not None:
            output = '      <id_pool name="%s" parent="%s" source="%s" ' \
                     'total="%s">\n' % \
                     (self._name, self.parent, self._source, self._total)
        else:
            output =  '      <id_pool name="%s" source="%s" total="%s">\n' % \
                     (self._name, self._source, self._total)
        for rnge in self._ranges:
            output += '        <range id="%s" start="%d" size="%d" />\n' % rnge
        output += '      </id_pool>\n'
        return output

class Heap(object):
    ids = IdList("Heap")

    @classmethod
    def reset(cls):
        cls.ids = IdList("Heap")

    def __init__ (self, sec_id, chunksize, blocksize, groups = "full"):
        self._sec_id = sec_id
        self.id = Heap.ids.id()
        self._cs = chunksize
        self._bs = blocksize

        if groups == "full":
            self._groups = {"mutex"        : ["small", 0, 0],
                            "space"        : ["small", 0, 0],
                            "tcb"          : ["small", 0, 0],
                            "l0_allocator" : ["small", 0, 0],
                            "l1_allocator" : ["small", 0, 0],
                            "clist"        : ["normal", 0],
                            "clistids"     : ["normal", 0],
                            "root_clist"   : ["normal", 0],
                            "ll"           : ["normal", 0],
                            "misc"         : ["normal", 0],
                            "mutexids"     : ["normal", 0],
                            "pgtab"        : ["normal", 0],
                            "resource"     : ["normal", 0],
                            "spaceids"     : ["normal", 0],
                            "stack"        : ["normal", 0],
                            "trace"        : ["normal", 0],
                            # NOTE: utcb is not actually listed in code as
                            #       a small allocation.  But because it works
                            #       in pages (mapped into user space) its
                            #       behavior is the same as the small
                            #       allocators.
                            "utcb"         : ["small", 0, 0],
                            "irq"          : ["normal", 0],
                            "physseg_list" : ["normal", 0]}
        else:
            self._groups = {"pgtable" : ["normal", 0],
                            "tcb"     : ["normal", 0],
                            "queue"   : ["normal", 0],
                            "futex"   : ["normal", 0]}

    def utcb_size(self):
        if self._groups.has_key("utcb"):
            return self._groups["utcb"]
        else:
            return None

    def allocate(self, groupname, size):
        group = self._groups[groupname]
        if group[0] == "small":
            group[2] += size

            if group[2] > group[1]:
                group[1] += self._bs
        else:
            final_size = (((size - 1)/ self._cs) * self._cs) + self._cs
            group[1] += final_size

    def format(self):
        output =  '      <mem_pool id="%s" node="%s">\n' % \
                 (self.id, self._sec_id)

        for key in self._groups.keys():
            val = self._groups[key]
            output += '        <group name="%s" type="%s" size="%s" />\n' % \
                      (key, val[0], val[1])

        output += '      </mem_pool>\n'

        return output

class Block(object):
    """
    Represents a named block that is layed out during initialisation (similar
    to what Nano thinks of as a heap)
    """
    def __init__(self, id, source_id):
        self._details = (id, source_id)

    def format(self):
        output =  '      <mem_pool id="%s" node="%s" />\n' % self._details
        return output
