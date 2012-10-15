##############################################################################z
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

"""The merge command is the main purpose of elf weaver. It provides a
way for merging many ELF files into one for the purpose of creating a
bootable image.

The merge_cmd function is the input from weaver.main. It parses command
line arguments and calls the merge() function which does most of the work.

The merge() command can also be called from another python library.
"""
import os
from optparse import OptionParser
from weaver import MergeError
from weaver.parse_spec import parse_spec_xml, parse_spec_xml_string, XmlCollector
from weaver.ezxml import size_attr
import weaver.image
import weaver.pools
import weaver.namespace
import re
from weaver.machine import Machine
from weaver.kernel import Kernel, get_kernel_version
from weaver.pools import VirtualPool, PhysicalPool
from weaver.notes import NotesSection

def collect_image_objects(parsed, ignore_name, kernel_heap_size,
                          namespace, image):
    """
    Extract the information in the XML elements and shove them into
    various data structures for later memory layout and image
    processing.
    """

    # FIXME: collect and handle extension namespaces
    machine_el = parsed.find_child("machine")
    machine = XmlCollector.COLLECTORS[machine_el.tag]()
    machine.collect_xml(machine_el, ignore_name)

    pools = weaver.pools.Pools()
    pools.new_virtual_pool("direct", machine)

    for el in parsed.find_children("virtual_pool"):
        pool = XmlCollector.COLLECTORS[el.tag](pools, machine)
        pool.collect_xml(el, machine, pools)

    for el in parsed.find_children("physical_pool"):
        pool = XmlCollector.COLLECTORS[el.tag](pools, machine)
        pool.collect_xml(el, machine, pools)

    kernel_el = parsed.find_child("kernel")
    kernel = XmlCollector.COLLECTORS[kernel_el.tag](machine)
    if kernel_el is not None:
        kernel.collect_xml(kernel_el, kernel_heap_size, namespace, image,
                           machine, pools)

    # Get the kernel release version.
    kern_ver = get_kernel_version(kernel_el, machine)

    # The tage names of the non-cell elements of an image.
    SYSTEM_TAGS = (Machine.element.name, VirtualPool.element.name,
                   PhysicalPool.element.name, Kernel.element.name)

    cell_els = [cell_el for cell_el in parsed.children if cell_el.tag not in SYSTEM_TAGS]

    # All top level elements should be known.
    assert len([cell_el for cell_el in cell_els if cell_el.tag not in XmlCollector.COLLECTORS]) == 0

    # Create Cell objects to handle the remains elements.
    found_cells = []

    for cell_el in cell_els:
        cell = XmlCollector.COLLECTORS[cell_el.tag]()
        cell.collect_xml(cell_el, ignore_name, namespace, machine, pools,
                         kernel, image)
        found_cells.append(cell)

        # Get the cell release version for each cell.
        cell_ver = cell.get_cell_api_version()

        if cell_ver != kern_ver:
            raise MergeError, \
                  "Cell \"%s\" is built against SDK version 0x%x " \
                  "but the kernel expects version 0x%x." % \
                  (cell.name, cell_ver, kern_ver)

    return machine, kernel, pools, found_cells

def merge(spec_file, options):
    """Based on a given spec file, process it and merge into
    the output_file."""
    image = weaver.image.Image(options.program_header_offset)

    namespace = weaver.namespace.ObjectNameSpace(None, '/')

    # parsed is an in memory tree of Element Objects
    # which is created from spec_file

    Machine.register()
    VirtualPool.register()
    PhysicalPool.register()
    Kernel.register()

    if options.spec_is_string:
        parsed = parse_spec_xml_string(spec_file)
    else:
        parsed = parse_spec_xml(spec_file)

    machine, kernel, pools, found_cells = \
        collect_image_objects(parsed, options.ignore_name,
                              options.kernel_heap_size,
                              namespace, image)

    # Resolve unknown cap names.  This will generate references to
    # kernel caps, which are needed by kernel.layout_cells_pre().
    for cell in found_cells:
        cell.env.snap(cell)

    kernel.layout_cells_pre()

    # For each found cell generate any dynamic segments that need processing
    for cell in found_cells:
        cell.generate_dynamic_segments(namespace, machine, pools, kernel, image)

    # Kernel dynamic elements need to be performed after all the cells
    # have been completed in order for the cell's enviornment memsections
    # to be populated before we generate the kernel's init script
    kernel.create_dynamic_segments(namespace, image, machine, pools)

    image.layout(machine, pools)
    notes = NotesSection(kernel)
    notes.set_mempool(image.phys_pool_stack.bot())
    image.get_elf().add_section(notes.encode_section(image.get_elf()))

    # The ELF file must be prepared to calculate the offset value of
    # the segment info records in the environment.
    image.prepare()

    kernel.layout_cells_post(image)

    # Perform any generation code that needs to be run for each found cell
    for cell in found_cells:
        cell.generate_init(machine, pools, kernel, image)
    kernel.generate_init_script(image, machine)

    image.apply_patches()
    image.write_out_image(options.output_file, image, kernel, machine)

    # If wanted, print out the final tree.
    if options.dump_layout:
        image.dump()

    if options.last_phys:
        pools.print_last_phys()

def merge_cmd(args):
    """Merge command call from main. This parses command line
    arguments and calls merge, which does all the main work."""
    parser = OptionParser("%prog merge [options] specfile", add_help_option=0)
    parser.add_option("-H", "--help", action="help")
    parser.add_option("-o", "--output", dest="output_file", metavar="FILE",
                      help="Destination filename")
    parser.add_option("-l", "--lastphys", dest="last_phys",
                      action="store_true", default=False,
                      help="After merging, print the next available " \
                      "physical address in each pool.")
    parser.add_option('-k', "--kernel-heap-size", dest="kernel_heap_size",
                      action="store", default="0x0L",
                      help="Specify the size of the kernel heap, " \
                      "overridding the value in the specfile.")
    parser.add_option('-i', "--ignore", dest="ignore_name",
                      action="store", default="^$",
                      help="A regex specifying programs to be ignored.")
    parser.add_option('-S', "--string", dest="spec_is_string",
                      action="store_true",
                      help="Treat the specfile argument as an XML string.")
    parser.add_option('-m', "--map", dest="dump_layout",
                      action="store_true",
                      help="Dump a memory map of the final image.")
    parser.add_option('-p', "--program-header-offset",
                      dest="program_header_offset",
                      action="store", type="long", default=None,
                      help="Set the program header offset in the ELF file.")
    parser.add_option('-y', "--verify", dest="verify",
                      action="store_true", default=False,
                      help="Verify that the specfile conforms to the DTD.")
    parser.add_option('-c', "--custom-path", dest="custom_path",
                      action="store", default="",
                      help="The directory containing custom cell, extension and ABI packages")

    (options, args) = parser.parse_args(args)


    custom_path = options.custom_path
    if not custom_path:
        custom_path = os.environ.get("ELFWEAVER_CUSTOM_PATH")
    weaver.cells.import_custom_cells(custom_path)
    weaver.extensions.import_custom_extensions(custom_path)

    options.kernel_heap_size = size_attr(options.kernel_heap_size)
    options.ignore_name = re.compile(options.ignore_name)

    if options.output_file is None and not options.last_phys:
        parser.error("Output must be specified")

    if len(args) != 1:
        parser.error("Expecting a spec file.")

    # During elfweaver-extensions development, verify the document.
    if options.verify and not options.spec_is_string:
        xmllint = os.system('xmllint --path "tools/pyelf/dtd ../../dtd ../dtd" -o /dev/null --valid %s' % args[0])
        if xmllint != 0:
            return xmllint

    spec_file = args[0]
    merge(spec_file, options)

    return 0
