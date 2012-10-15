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

"""Implements a command for displaying memory usage statistics"""

import sys
import re
import os.path
from optparse import OptionParser
from elf.core import PreparedElfFile
from weaver.ezxml import EzXMLError
from weaver.kernel import check_api_versions
from weaver.kernel_nano import NANO_KERNEL_API_VERSION
from weaver.kernel_micro import MICRO_KERNEL_API_VERSION
from weaver.memstats_xml import Memstats_el
from weaver.memstats_reports import REPORTS_SET
from weaver.memstats_model import ElfProgram, Memstats, IdList, Heap
from weaver import notes

section_drops = re.compile(r".*(debug|segment_names|comment|bootinfo).*")
func_sec = re.compile(r"^[a-zA-Z0-9_]*\.(text).*")
data_sec = re.compile(r"^[a-zA-Z0-9_]*\.(bss|data).*")
code = re.compile(r"Code")
data = re.compile(r"Data")
dollar_drops = re.compile(r"^[^$].*") # Any function starting with a $

iguana_stats_strings = (
    "EASSize",
    "MemsectionNodeSize",
    "PTESize",
    "ActivePDSize",
    "ObjtablePageSize",
    "ObjtableOrder",
    "ObjtableEntrySize",
    "PhysmemListSize",
    "PhysmemSize",
    "PhysMemorySize",
    "VirtmemSize",
    "VirtMemorySize",
    "PoolSize",
    "PoolMemorySize",
    "PDNodeSize",
    "ActiveMSSize",
    "CListSize",
    "CBBufferHandleSize",
    "SessionSize",
    "SessionNodeSize",
    "ThreadSize",
)


def memstats_cmd(args):
    """Memory Usage Statistics program implementation"""
    parser = OptionParser("%prog memstats [options] file [file]",
                          add_help_option=0)
    parser.add_option("-H", "--help", action="help")
    parser.add_option("-x", "--xml", action="store_true", dest="output_xml",
                      help="Produces an XML file containing memory statistics" \
                           " in the same directory as the supplied ELF.")
    parser.add_option("-y", "--verify", action="store_true", dest="verify_xml",
                      help="Verify the XML file.")
    parser.add_option("-r", "--report", action="store", dest="report",
                      metavar="NAME",
                      help="Parse a memory statistics XML file and output "\
                           "the named report.  Valid reports are: '%s'." %
                      "', '".join(REPORTS_SET.keys()))
    parser.add_option("-v", "--verbose", action="store_true",
                      dest="verbose_report",
                      help="Produce the verbose form of the requested report.")
    parser.add_option("-l", "--largest-num", action="store",
                      dest="n_objs", type="int",
                      help="Print, at most, the largest N_OBJS " \
                           "text and code objects.")
    parser.add_option("-d", "--diff", action="store_true", dest="output_diff",
                      help="Used on a report, to indicate there are two "
                      "files and the differences should be reported.")
    parser.add_option("-R", "--repository", action="store",
                      dest="repository", default=None,
                      help="Include the version control repository in the "
                      "XML file.")
    parser.add_option("-c", "--changeset", action="store",
                      dest="changeset", default=None,
                      help="Include the version control changeset in the "
                      "XML file.")

    (options, args) = parser.parse_args(args)

    IdList.reset()
    Heap.reset()

    if len(args) < 1 or args[0] is "":
        parser.error("Missing file argument(s)")

    input_file = args[0]

    if not os.path.exists(input_file):
        print >> sys.stderr, 'Error: File "%s" does not exist.' % input_file
        return 1

    if options.output_xml:
        elf = PreparedElfFile(filename=input_file)

        if not elf.segments:
            print >> sys.stderr, \
                  'Error: ELF file "%s" contains no segments.' % input_file
            return 1

    if options.output_diff and not options.report:
        parser.error("Diff only makes sense on a report.")
        return

    # -x implies input is an ELF
    # -t implies input is XML
    if options.output_xml:
        notes_sec = notes.find_elfweaver_notes(elf)
        memstats = Memstats(elf.machine,
                            notes_sec.cpu,
                            notes_sec.poolname,
                            notes_sec.memsecs,
                            notes_sec.cell_names,
                            notes_sec.space_names,
                            notes_sec.mappings)
        programs = []
        parse_segments(elf, programs, elf.segments, memstats)

        xml_file = os.path.join(os.path.dirname(input_file), "memstats.xml")

        hg_stats = (options.repository, options.changeset)
        memstats.set_revision(hg_stats)

        kern_ver = check_api_versions(elf)

        if kern_ver == MICRO_KERNEL_API_VERSION:
            memstats.env.env_type = "Micro"
            import weaver.kernel_micro_elf
            script = weaver.kernel_micro_elf.find_init_script(elf, memstats)
        elif kern_ver == NANO_KERNEL_API_VERSION:
            memstats.env.env_type = "Nano"
            import weaver.kernel_nano_elf
            heap = weaver.kernel_nano_elf.find_nano_heap(elf)
            heap.decode(memstats)
        else:
            print >> sys.stderr, "Unknown kernel type."
            return 1

        xml_f = open(xml_file, "w")
        xml_f.write(memstats.format())
        xml_f.close()

        if options.verify_xml:
            xml_verify(xml_file)

    if options.report:
        if options.output_xml:
            input_file = xml_file

        if options.output_diff:
            if len(args) != 2:
                parser.error("Report diffs require two files.")

            diff_file = args[1]

            if options.verify_xml:
                xml_verify(input_file)
                xml_verify(diff_file)

            gen_diff_report(options.report,
                            input_file,
                            diff_file,
                            options.n_objs,
                            options.verbose_report)
        else:
            if options.verify_xml:
                xml_verify(input_file)

            gen_report(options.report,
                       input_file,
                       options.n_objs,
                       options.verbose_report)


def parse_segments(elf, programs, segments, memstats):
    """Create a list of ProgramStatistics objects by parsing the
    segments of an ELF"""

    seg_count = 0

    # All segments which have sections
    for seg in [x for x in segments if x.has_sections()]:
        stats = None
        # All sections which are non-zero in size
        sections = [x for x in seg.get_sections() if x.get_size != 0]

        if sections:
            # Section names have form: <program_name>.<section_name>
            program_name = sections[0].name.split(".")[0]

            if not section_drops.match(sections[0].name):
                #
                # One special case - "initscript" gets put with the kernel
                #
                if program_name == "initscript":
                    program_name = "kernel"
                stats = find_program(programs, program_name, memstats)
            else:
                continue

            stats.add_segment(seg_count,
                              seg.paddr,
                              seg.vaddr,
                              seg.get_filesz(),
                              seg.get_memsz(),
                              sections)
            seg_count += 1

            for sect in sections:
                stats.add_section(sect.name,
                                  sect.address,
                                  sect.get_size())
                # Keep a list of functions
                if func_sec.match(sect.name):
                    for sym in elf.section_symbols(sect):
                        if code.match(sym.get_type_string()) and \
                               dollar_drops.match(sym.name):
                            stats.add_func_obj(sym.name, sym.size)

                # Keep a list of data symbols
                if data_sec.match(sect.name):
                    for sym in elf.section_symbols(sect):
                        if data.match(sym.get_type_string()) and \
                               dollar_drops.match(sym.name):
                            stats.add_data_obj(sym.name, sym.size)


def find_program(programs, name, memstats):
    """Given a list of programs, search for one with the name 'name'
    If it does not exist, create it.  Return the program"""

    if programs:
        for prog in programs:
            if prog.name == name:
                return prog

    # Can't find it so create a new object
    prog = ElfProgram(name, memstats)
    programs.append(prog)

    return prog

def xml_verify(target):
    """Pass the XML file through xmllint"""
    os.system('xmllint --path "tools/pyelf/dtd ../../dtd ../dtd" '\
               '-o /dev/null --valid %s' % target)

def gen_report(reportname, xml_file, n_objs, verbose):
    """Generate the named report from a given XML file"""
    try:
        parsed = Memstats_el.parse_xml_file(xml_file)
    except EzXMLError, text:
        print >> sys.stderr, text
        sys.exit(1)

    if REPORTS_SET.has_key(reportname):
        report = REPORTS_SET[reportname](parsed)
        report.generate(n_objs, verbose)
    else:
        print >> sys.stderr, 'Error: "%s" is not a valid report name.' % \
              reportname
        print >> sys.stderr, 'Valid report names are:'

        for rep in REPORTS_SET:
            print >> sys.stderr, '  ' + rep

        sys.exit(1)

def gen_diff_report(reportname, xml_file, diff_xml_file, n_objs, verbose):
    """Generate a diff version of the named report from the given XML files"""
    try:
        parsed_main = Memstats_el.parse_xml_file(xml_file)
        parsed_old  = Memstats_el.parse_xml_file(diff_xml_file)
    except EzXMLError, text:
        print >> sys.stderr, text

        sys.exit(1)

    if REPORTS_SET.has_key(reportname):
        report_main = REPORTS_SET[reportname](parsed_main)
        report_old = REPORTS_SET[reportname](parsed_old)
        report_main.generate(n_objs, verbose, report_old)
    else:
        print >> sys.stderr, 'Error: "%s" is not a valid report name' % \
              reportname
        print >> sys.stderr, 'Valid report names are:'

        for rep in REPORTS_SET:
            print >> sys.stderr, '  ' + rep

        sys.exit(1)

def iguana_stats(elf):
    """
    Find the symbol containing Iguana statistics, print it's contents in
    TeX.
    """
    wordsize = elf.wordsize / 8
    stats_sym = elf.find_symbol("ig_server-iguana_stats")

    if stats_sym is None:
        return

    stats_sec = elf.find_section_named(stats_sym.section.name)

    i = 0
    j = 0
    offset = stats_sym.value - stats_sec.address

    while i < stats_sym.size:
        stats_val = stats_sec.get_data().get_data(offset + i,
                                                  wordsize,
                                                  elf.endianess)
        print "\setcounter{%s}{%d}" % (iguana_stats_strings[j], stats_val)
        i += wordsize
        j += 1
