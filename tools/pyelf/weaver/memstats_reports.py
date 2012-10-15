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

"""
Classes for the various types of reports which can be produces from the
memstats data.
"""

import re, sys
from weaver.memstats_model import page_size


SECTION_DROPS = re.compile(r".*(weaved|debug|segment_names|comment|bootinfo).*")

class MemorySetup(object):
    """
    Utility class to prep and report on segments and sections that came
    from the ELF image originally
    """
    def __init__(self, resources):
        self.segments = []
        self.unallocated_sections = []

        for segment in resources.find_child("segment_list").find_children("segment"):
            seg_sections = set()
            for sec in segment.sections.split(' '):
                seg_sections.add(sec)

            self.segments.append({'secs'   : seg_sections,
                                  'phys'   : segment.phys,
                                  'memsz'  : segment.memsz,
                                  'filesz' : segment.filesz})

        self.section_list = \
                          resources.find_child("section_list").find_children("section")

    def find_segment(self, sec_id):
        for seg in self.segments:
            if sec_id in seg['secs']:
                return seg
        return None

    def find_section(self, phys_id):
        for section in self.section_list:
            if section.id == phys_id:
                return section

        return None

    def process_section(self, phys_section):
        curr_seg = self.find_segment(phys_section.id)

        if curr_seg is None:
            self.unallocated_sections.append(phys_section)

        return curr_seg


class Report(object):
    """ Common report functionality"""
    def __init__(self, parsed_xml):
        self.revision = parsed_xml.find_child("revision")

    @classmethod
    def print_revision(cls, revision):
        print "\tRepository: %s" % revision.repository
        print "\tChangeset: %s" % revision.changeset
        print

    def compare_revisions(self, old):
        fail = False
        if self.revision is not None and old.revision is not None:
            if self.revision.repository != old.revision.repository:
                print "Images built from different repositories"
                fail = True
            elif self.revision.changeset != old.revision.changeset:
                print "Revision details differ"
                fail = True
            if fail:
                print "Old:"
                Report. print_revision(old.revision)
                print"New:"
                Report.print_revision(self.revision)

        return fail

    @classmethod
    def names_union(cls, list_a, list_b, func):
        """
        Take the supplied lists and produce a single list with the
        union of the names in each.  func() is applied to each list
        element to obtain the name.
        """
        union = [func(a) for a in list_a]
        tmp = [func(b) for b in list_b]
        union.extend([x for x in tmp if x not in union])
        return union

    @classmethod
    def cell_check(cls, old_cells, new_cells, cname):
        def cell_in_list(clist, name):
            for cell in clist:
                if cell[0] == name:
                    return cell
            return None

        new_cell = cell_in_list(new_cells, cname)
        old_cell = cell_in_list(old_cells, cname)

        if new_cell is None: # only in old
            print "Cell %s removed" % cname
            return None
        elif old_cell is None: #only in new
            print "Cell %s added" % cname
            return None
        else:
            return (new_cell, old_cell)

class FixedUsage(Report):
    """
    Report on the memory usage by objects produced directly from the compiled
    code.
    """
    def __init__(self, parsed_xml):
        #
        # This is still grouped by program.  Need to find programs
        # in kernel space and cell spaces, then extract full info from
        # resources section.
        #
        Report.__init__(self, parsed_xml)
        self._programs = []
        environ = parsed_xml.find_child("environment")
        resources = parsed_xml.find_child("resources")

        self._coll = MemorySetup(resources)

        kspace = environ.find_child("kernel").find_child("space")
        self.process_space(kspace)

        for cell in environ.find_children("cell"):
            for space in cell.find_children("space"):
                self.process_space(space)

    def print_program_stats(self, prog, n_objs, verbose):
        """ For the given program, output the size values"""

        print "Statistics for program %s:\n" % prog['name']
        print "\tSections (with debug and empty sections dropped):\n"

        prog_static_size = 0

        for (name, size) in prog['sections']:
            print "\t%-40s%d Bytes" % (name, size)
            prog_static_size += size
        print

        prog_rounded_size = 0

        for seg in prog['segments']:
            prog_rounded_size += seg['memsz']

        prog_rounded_size += (page_size - (prog_rounded_size % page_size))
        rounded_size = prog_rounded_size / 1024.0
        pages = prog_rounded_size / page_size

        print "\tTotal static size by sections (unrounded) is %.2f KB\n" \
              % (prog_static_size / 1024.0)
        print "\tSize by rounded segments (%d total) is %.2f KB (%d pages)\n" \
              % (len(prog['segments']), rounded_size, pages)

        if (verbose):
            self.object_lists(prog['text_objects'],
                              prog['data_objects'],
                              n_objs)

    def object_lists(self, func_list, data_list, n_objs):
        if not func_list == []:
            func_list.sort(lambda x, y: cmp(y[1], x[1]))
            print "\tTop %d functions (from %d) by size:\n" \
                  % (len(func_list[:n_objs]), len(func_list))

            for (name, size) in func_list[:n_objs]:
                print "\t%-40s%d Bytes" % (name, size)

        if not  data_list == []:
            data_list.sort(lambda x, y: cmp(y[1], x[1]))
            print "\n\tTop %d global data elements (from %d) by size:\n" \
                  % (len(data_list[:n_objs]), len(data_list))

        for (name, size) in data_list[:n_objs]:
            print "\t%-40s%d Bytes" % (name, size)

    def generate(self,
                 n_objs,
                 verbose,
                 old = None):
        """
        Produce the report on stdout
        """
        if old:
            #
            # Produce a difference report instead of a "normal" report.
            #
            print

            if verbose:
                print "Note: diff report does not process text/data objects"
                print

            differences = self.compare_revisions(old)

            prognames = Report.names_union(self._programs,
                                           old._programs,
                                           lambda x: x['name'])
            def prog_in_list(programs, name):
                for prog in programs:
                    if prog['name'] == name:
                        return prog

                return None

            for pname in prognames:
                in_new = prog_in_list(self._programs, pname)
                in_old = prog_in_list(old._programs, pname)

                if in_new is None: # only in old
                    print "Removed Program %s" % pname
                    self.print_program_stats(in_new, n_objs, verbose)
                    differences = True
                elif in_old is None: # only in new
                    print "Added Program %s" % pname
                    self.print_program_stats(in_new, n_objs, verbose)
                    differences = True
                else:
                    #
                    # Work our way through the contents of each program
                    # and compare.
                    #
                    new_sections = in_new['sections']
                    old_sections = in_old['sections']
                    secnames = Report.names_union(new_sections,
                                                  old_sections,
                                                  lambda x: x[0])
                    def sec_in_list(sections, name):
                        for sec in sections:
                            if sec[0] == name:
                                return sec
                        return None

                    size_string = "Size difference by %s in %s: old %d " \
                                  "Bytes, new %d Bytes"
                    old_size_by_section = 0
                    new_size_by_section = 0

                    for name in secnames:
                        sec_new = sec_in_list(new_sections, name)
                        sec_old = sec_in_list(old_sections, name)

                        if sec_new is None: # only in old
                            print "Removed Section: %s: %d Bytes" % sec_old
                            old_size_by_section += sec_old[1]
                            differences = True
                        elif sec_old is None: # only in new
                            print "Removed Section: %s: %d Bytes" % sec_new
                            new_size_by_section += sec_new[1]
                            differences = True
                        else:
                            old_size_by_section += sec_old[1]
                            new_size_by_section += sec_new[1]

                            if sec_old[1] != sec_new[1]:
                                print "Section %s varies in size " \
                                      "(old %d, new %d)" % \
                                      (name, sec_old[1], sec_new[1])
                                differences = True

                    if old_size_by_section != new_size_by_section:
                        print size_string % ("section",
                                             pname,
                                             old_size_by_section,
                                             new_size_by_section)
                        # differences already set

                    def sum_segments(seg_list):
                        retval = 0
                        for seg in seg_list:
                            retval += seg['filesz']

                        return retval


                    new_size_by_segment = sum_segments(in_new['segments'])
                    old_size_by_segment = sum_segments(in_old['segments'])

                    if old_size_by_segment != new_size_by_segment:
                        print size_string % ("segment",
                                             pname,
                                             old_size_by_segment,
                                             new_size_by_segment)
                        differences = True

            if not differences:
                print "No differences."

            print
            #
            # End of difference report here.
            #
            return

        if self.revision is not None:
            print "Mercurial Revision:"
            self.print_revision(self.revision)

        for prog in self._programs:
            self.print_program_stats(prog, n_objs, verbose)

        print

    def process_space(self, space):

        for program in space.find_children("program"):
            #print "processing program " + program.name
            program_desc = {'name' : program.name,
                            'segments' : [],
                            'sections' : [],
                            'text_objects' : [],
                            'data_objects' : []}

            for psec in program.find_child("psec_list").find_children("psec"):
                #print "    processing " + psec.name
                if SECTION_DROPS.match(psec.name):
                    continue

                phys_section = self._coll.find_section(psec.phys)
                assert(phys_section is not None)
                #print "      phys_section id is " + phys_section.id
                seg = self._coll.process_section(phys_section)

                if seg is not None:
                    #print "      found in segment at addr %#x" % seg['phys']
                    program_desc['sections'].append((psec.name,
                                                     phys_section.size))
                    found = False

                    for existing_seg in program_desc['segments']:
                        if existing_seg == seg:
                            found = True
                            #print "      (segment already allocated)"
                            break

                    if not found:
                        program_desc['segments'].append(seg)

            for obj in program.find_child("object_list").find_children("object"):
                if obj.type == "text":
                    program_desc["text_objects"].append((obj.name, obj.size))
                else:
                    program_desc["data_objects"].append((obj.name, obj.size))

            self._programs.append(program_desc)

class InitialUsage(Report):
    """
    Report on the physical memory which is in use at the end of the
    OKL4 kernel initialisation.
    """
    def __init__(self, parsed_xml):
        Report.__init__(self, parsed_xml)
        self.resources = parsed_xml.find_child("resources")
        self.environ = parsed_xml.find_child("environment")

        self._coll = MemorySetup(self.resources)

        for phys_section in self._coll.section_list:
            self._coll.process_section(phys_section)

        self._heaps = []
        mempools = self.resources.find_child("pools").find_children("mem_pool")

        for heap in mempools:
            for sec in self.resources.find_all_children("section"):
                if sec.id == heap.node:
                    self._heaps.append(sec)

                    for usec in self._coll.unallocated_sections:
                        if usec.id == sec.id:
                            self._coll.unallocated_sections.remove(usec)

    def generate(self,
                 n_objs,
                 verbose,
                 old = None):
        """
        Produce the report on stdout
        """
        def sec_to_psec(environ, sec_id):
            for psec in environ.find_all_children("psec"):
                if psec.phys == sec_id:
                    return psec.name
            return ""

        if old:
            differences = self.compare_revisions(old)

            #
            # Keeping this fairly simple - any variation in the number
            # of segments results in a quick exit.
            #
            if len(self._coll.segments) != len(old._coll.segments):
                print "Number of segments differ: %d vs %d" % \
                    (len(self._coll.segments), len(old._coll.segments))
                differences = True
            else:
                segs = len(self._coll.segments)
                count = 0

                while (count < segs):
                    if self._coll.segments[count]['memsz'] != \
                            old._coll.segments[count]['memsz']:
                        print "Segment %d has size variation; old %d Bytes, " \
                              "new %d Bytes" % \
                              (count,
                               old._coll.segments[count]['memsz'],
                               self._coll.segments[count]['memsz'])
                        differences = True

                    count += 1
            #
            # Unallocated sections
            #
            def id_in_list(sec_list, sec_id):
                for sec in sec_list:
                    if sec.id == sec_id:
                        return sec

                return None

            unallocated_ids = \
                Report.names_union(self._coll.unallocated_sections,
                                   old._coll.unallocated_sections,
                                   lambda x: x.id)
            if verbose:
                for sec_id in unallocated_ids:
                    new_sec = id_in_list(self._coll.unallocated_sections,
                                         sec_id)
                    old_sec = id_in_list(old._coll.unallocated_sections,
                                         sec_id)

                    if new_sec is None:
                        print "%s removed" % sec_to_psec(old.environ,
                                                         sec_id)
                        differences = True
                    elif old_sec is None:
                        print "%s added" % sec_to_psec(self.environ,
                                                       sec_id)
                        differences = True
            else:
                for sec_id in unallocated_ids:
                    # Apologies for fomatting of next statement
                    if (id_in_list(self._coll.unallocated_sections,
                                   sec_id) is None) or \
                       (id_in_list(old._coll.unallocated_sections,
                                   sec_id) is None):
                        print "Init script sections differ"
                        differences = True
                        break

            if not differences:
                print "No differences."

            print
            #
            # End of difference report here.
            #
            return

        seg_len = len(self._coll.segments)
        seg_size = 0

        for seg in self._coll.segments:
            seg_size += seg['memsz']

        seg_size = seg_size / 1024

        print "\nMemory in Use at end of initialization"
        print "=======================================\n"
        print "Memory claimed by ELF image:"

        if (verbose):
            seg_count = 1

            for segment in self._coll.segments:
                print "ELF Segment %d: %#8x - %#8x (%4dKB)" % \
                    (seg_count, segment['phys'],
                     segment['phys'] + segment['memsz'] - 1,
                     segment['memsz'] / 1024)
                seg_count += 1

            print

        print "%4d Segments giving %6dKB" % (seg_len, seg_size)

        print "\nSAS1:"

        sec_len = len(self._coll.unallocated_sections)
        sec_size = 0

        for sec in self._coll.unallocated_sections:
            sec_size += sec.size

            if (verbose):
                name = sec_to_psec(self.environ, sec.id)

                if name == "":
                    #
                    # The only _expected_ case for this is the global
                    # thread handles.
                    #
                    for pool in self.resources.find_all_children("id_pool"):
                        if pool.source == sec.id:
                            name = pool.name

                print "%-40s%#8x - %#8x (%4dKB)" % (name,
                                                     sec.address,
                                                     sec.address + sec.size - 1,
                                                     sec.size / 1024)
        sec_size = sec_size / 1024

        print "%4d additional sections giving %6dKB" % (sec_len, sec_size)

        cell_num = 1
        cell_sizes = 0

        for heap in self._heaps:
            print "\nCell %d:" % cell_num
            hsz = heap.size / 1024
            if verbose:
                print "  Heap\t%#8x - %#8x (%4dKB)" % \
                      (heap.address,
                       heap.address + heap.size - 1,
                       hsz)
            else:
                print "  Heap:\t%6dKB" % hsz

            cell_num += 1
            cell_sizes += hsz

        final_size = seg_size + sec_size + cell_sizes
        phys_size = 0
        pm = self.resources.find_child("phys_mem")
        poolname = pm.pool

        for sec in pm.find_children("section"):
            if sec.id == poolname:
                phys_size = sec.size
                break

        phys_size = phys_size / 1024

        print "\nTotal Memory claimed for use: %8dKB\n" % final_size

        if phys_size != 0:
            print "Physical memory remaining: %dKB of %dKB\n" % \
                  ((phys_size - final_size), phys_size)

class HeapUsage(Report):
    """
    Report on the number of heaps, and how much of their storage has been used,
    at the end of OKL4 kernel initialisation.
    """
    def __init__(self, parsed_xml):
        Report.__init__(self, parsed_xml)
        env = parsed_xml.find_child("environment")
        resources = parsed_xml.find_child("resources")

        if env.type == "Nano":
            print >> sys.stderr, "Heap Usage report not valid for Nano Kernel"
            sys.exit(1)

        pools = resources.find_child("pools")
        sections = resources.find_child("section_list")
        heaps = {}
        self.cells = []

        for pool in pools.find_children("mem_pool"):
            for sec in sections.find_children("section"):
                if sec.id == pool.node:
                    heaps[pool.id] = (pool, sec)

        for cell in parsed_xml.find_child("environment").find_children("cell"):
            for pool in cell.find_children("pool"):
                if heaps.has_key(pool.vmid):
                    self.cells.append((cell.name, heaps[pool.vmid]))

    @classmethod
    def sum_heap(cls, heap):
        retval = 0

        for group in heap.find_children("group"):
            retval += group.size

        return retval

    def generate(self,
                 _, #n_objs,
                 verbose,
                 old = None):
        """
        Produce the report on stdout
        """
        if old:
            differences = self.compare_revisions(old)

            cell_names = Report.names_union(self.cells,
                                            old.cells,
                                            lambda x: x[0])
            def group_sizes(heap):
                retval = {}

                for group in heap.find_children("group"):
                    retval[group.name] = group.size

                return retval

            for cname in cell_names:
                check = Report.cell_check(old.cells, self.cells, cname)

                if check is None:
                    differences = True
                else:
                    new_cell, old_cell = check
                    # Compare heaps
                    new_heap, new_sec = new_cell[1]
                    old_heap, old_sec = old_cell[1]

                    if new_heap.id != old_heap.id:
                        print "Cell %s heap id changed from %s to %s" % \
                            (cname, old_heap.id, new_heap.id)
                        differences = True

                    if new_sec.size != old_sec.size:
                        print "Cell %s heap size changed from %#x to %#x" % \
                            (cname, old_sec.size, new_sec.size)
                        differences = True

                    old_used = HeapUsage.sum_heap(old_heap)
                    new_used = HeapUsage.sum_heap(new_heap)

                    if old_used != new_used:
                        print "Cell %s heap usage from %d Bytes to %d Bytes" % \
                            (cname, old_used, new_used)
                        differences = True

                        if verbose:
                            new_groups = group_sizes(new_heap)
                            old_groups = group_sizes(old_heap)

                            for group in new_groups.keys():
                                if new_groups[group] != old_groups[group]:
                                    print"\t%s: %d Bytes to %d Bytes" % \
                                        (group,
                                         old_groups[group],
                                         new_groups[group])

            if not differences:
                print "No differences."

            print
            #
            # End of difference report here.
            #
            return

        print "\nHeap Usage"
        print "===========\n"
        count = 1

        for cell in self.cells:
            (heap, sec) = cell[1]
            used = HeapUsage.sum_heap(heap)
            print "Cell %d:" % count

            size = sec.size / 1024
            used = used / 1024
            print "Heap is %s - size %4dKB with %4dKB used" % \
                  (heap.id, size, used)

            if verbose:
                print "Allocations:"

                for group in heap.find_children("group"):
                    if group.size > 0:
                        print "  %16s:\t%6dKB" % (group.name, group.size / 1024)

            count += 1

            print

        print

class IdUsage(Report):
    """
    Report on the ID pools in existance, and the number of ids used,
    at the end of OKL4 kernel initialisation.
    """
    def __init__(self, parsed_xml):
        Report.__init__(self, parsed_xml)
        #
        # Get all the id pools, then go back through the environment
        # to work out what has been allocated, and what has been used.
        #
        self.id_pools = []
        self.cells = []

        env = parsed_xml.find_child("environment")

        if env.type == "Nano":
            print >> sys.stderr, "IDs Usage report not valid for Nano Kernel."
            sys.exit(1)

        ids = parsed_xml.find_child("resources").find_child("ids")

        for pool in ids.find_children("id_pool"):
            if pool.__dict__.has_key("parent"):
                self.assign_slot(pool.parent)

            ranges = []

            for rnge in pool.find_children("range"):
                ranges.append({'id' : rnge.id, 'size' : rnge.size, 'used' : 0})

            self.id_pools.append((pool.name, pool.total, ranges))

        for cell in env.find_children("cell"):
            cell_ids = {}

            for c_rnge in cell.find_children("id_range"):
                cell_ids[c_rnge.name] = c_rnge.node

            spaces = []

            for space in cell.find_children("space"):
                name = space.name
                space_ids = {}

                for s_rnge in space.find_children("id_range"):
                    space_ids[s_rnge.name] = s_rnge.node

                self.assign_slot(space_ids['spaces'])

                for thread in space.find_children("thread"):
                    self.assign_thread(thread.handle_id, thread.clist_id)

                # Need to add mutexes.
                spaces.append((name, space_ids))

            self.cells.append((cell_ids, spaces))

    def assign_thread(self, handle, clist):
        """
        A thread has been created using the handle list and capability
        list provided.  Locate them and increment their usage count.
        """
        for pool in self.id_pools:
            for rnge in pool[2]:
                if (rnge['id'] == handle) or (rnge['id'] == clist):
                    rnge['used'] += 1

    def find_slot(self, list_id):
        """
        Find the id_range resource element identified by the id.
        """
        for pool in self.id_pools:
            for rnge in pool[2]:
                if rnge['id'] == list_id:
                    return rnge

    def assign_slot(self, list_id):
        """
        A new entity has been created using an id from the list identified
        by the parameter.  Increment its usage count.
        """
        rnge = self.find_slot(list_id)
        rnge['used'] += 1

    def generate(self,
                 _, #n_objs,
                 verbose,
                 old = None):
        """
        Produce the report on stdout
        """
        if old:
            if verbose:
                print "(No verbose option when comparing ID reports)"

            differences = self.compare_revisions(old)

            if len(self.id_pools) != len(old.id_pools):
                print "Different number of pools in old and new"
                print "List in old: "
                print ' '.join([pool[0] for pool in old.id_pools])

                print "List in new: "
                print ' '.join([pool[0] for pool in self.id_pools])

                differences = True
            else:
                pools = max(len(self.id_pools), len(old.id_pools))

                for index in range(0, pools):
                    (new_name, new_total, new_range) = self.id_pools[index]
                    (old_name, old_total, old_range) = old.id_pools[index]

                    if old_name != new_name:
                        differences = True
                        print "Name change for list %d: was %s now %s" % \
                              (index, old_name, new_name)
                    else:
                        if old_total != new_total:
                            differences = True
                            print "List %s has different length: was %d " \
                                  "now %d" % (old_name, old_total, new_total)

                        if old_range != new_range:
                            differences = True
                            print "List %s has a different amount used: " \
                                  "was %d now %d" % (old_name, old_range, new_range)

            if not differences:
                print "No differences."

            print
            #
            # End of difference report here.
            #
            return

        def process_id_lists(ids, format_str):
            """
            ID lists can occur at several levels - logic factored out
            for reuse.
            """
            for list_type in ids.keys():
                rnge_id = ids[list_type]
                rnge  = self.find_slot(rnge_id)
                # Take the range, and strip the sequence number off
                # the end of the name, to give name of the enclosing
                # id pool
                id_pool = rnge_id[:-1]
                pad2 = ' ' * (16 - len(id_pool))
                print format_str %  (list_type,
                                     rnge['size'],
                                     id_pool,
                                     pad2,
                                     rnge['used'])

        print "\nID usage"
        print "=========\n"

        for (name, total, ranges) in self.id_pools:
            size = 0
            used = 0

            for rnge in ranges:
                size += rnge['size']
                used += rnge['used']

            if verbose:
                print "Type %-16s %4d available, %4d allocated to spaces" % \
                    (name, total, size)
            else:
                print "Type %-16s %4d available, %4d allocated to spaces, " \
                      "%4d used" % (name, total, size, used)
        print

        if verbose:
            ctype_str = "  Type %-10s %4d allocated from '%s',%s   %4d used"
            stype_str = "    Type %-10s %4d allocated from '%s',%s %4d used"
            count = 1

            for (cell_ids, spaces) in self.cells:
                print "Cell %d:" % count
                process_id_lists(cell_ids, ctype_str)

                for (name, ids) in spaces:
                    print "  Space '%s':" % name
                    process_id_lists(ids, stype_str)

                count += 1

class EnvSummary(object):
    """
    Produce a quick summary of the environment.  Added as a debugging tool,
    but may be useful as an overview without reading the XML
    """
    def __init__(self, parsed_xml):
        def process_cell_contents(cell, rep_obj):
            for space in cell.find_children("space"):
                rep_obj['spaces'].append(space.name)

                for prog in space.find_children("program"):
                    rep_obj['programs'].append((prog.name,
                                                len(prog.find_child("psec_list").find_children("psec"))))

                for id_range in space.find_children("id_range"):
                    count = 0

                    for src_range in resources.find_all_children("range"):
                        if src_range.id == id_range.node:
                            count = src_range.size

                    rep_obj['id_ranges'].append((count, id_range.name))

                for pool in space.find_children("pool"):
                    rep_obj['pools'].append((pool.id, pool.vmid))


        environment = parsed_xml.find_child("environment")
        resources = parsed_xml.find_child("resources")
        kernel_x = environment.find_child("kernel")
        cells_x = environment.find_children("cell")

        self.kernel = {'spaces'    : [],
                       'programs'  : [],
                       'id_ranges' : [],
                       'pools'     : []}
        process_cell_contents(kernel_x, self.kernel)

        self.cells = []
        for cell in cells_x:
            new_cell = {'spaces'   : [],
                       'programs'  : [],
                       'id_ranges' : [],
                       'pools'     : []}
            process_cell_contents(cell, new_cell)
            self.cells.append((cell.name, new_cell))

    def generate(self,
                 n_objs,
                 _, #verbose,
                 old = None):
        """
        Produce the report on stdout
        """
        if old:
            print "Environment Summary report does not provide a " \
                  "difference report."
            sys.exit(1)

        def cell_out(cell):
            for space in cell['spaces']:
                print "  Space called " + space

            print "  Id Ranges:"
            for id_rng in cell['id_ranges']:
                print "    %d for %s" % id_rng

            print "  Pools:"
            for pool in cell['pools']:
                print "    %s on %s" % pool

            print "  Programs:"
            for prog in cell['programs']:
                print "    %s with %d sections" % prog
            print

        print "\nQuick summary of environment:\n"
        print "In kernel:"
        cell_out(self.kernel)

        for cell in self.cells:
            print "In Cell %s:" % cell[0]
            cell_out(cell[1])

REPORTS_SET = {'fixed'       : FixedUsage,
               'initial'     : InitialUsage,
               'heap'        : HeapUsage,
               'ids'         : IdUsage,
               'env-summary' : EnvSummary,
               }
