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
Implement a command to link a relocatable ELF file, no symbol matching is done
only application of reloc sections.  This should work the same as as using ld.
"""

import os
import sys
from optparse import OptionParser

from elf.abi import KERNEL_SOC_LINKER_SCRIPT
from elf.core import UnpreparedElfFile, SectionedElfSegment
from elf.constants import PF_W, PF_R, PF_X, SHF_WRITE, SHF_EXECINSTR, \
        SHT_SYMTAB, PT_LOAD, ET_EXEC, SHT_REL, SHT_RELA
from elf.segnames import get_segment_names, add_segment_names

from weaver.linker_commands import SEGMENT_ALIGN, merge_sections
from weaver.linker_script import perform_link, standard_link

def link_cmd(args):
    """
    Link a relocatable ELF
    """

    # Link command shouldn't be run by end users!
    print >> sys.stderr, "********"
    print >> sys.stderr, "WARNING: elfweaver link is for internal debugging " \
                         "and testing purposes only."
    print >> sys.stderr, "********"
    print

    parser = OptionParser("%prog link [options] <file1.o> [<file2.o>, ...]",
                          add_help_option=0)
    parser.add_option("-H", "--help", action="help")
    parser.add_option("-o", "--output", dest="output_file", metavar="FILE",
                      help="Destination filename")
    parser.add_option("--section-start", action="callback", nargs=1,
                      type="string", dest="section_vaddr",
                      callback=parse_options, metavar="SECT_NAME=SECT_ADDR",
                      help="Specify the virtual address of a given section")
    parser.add_option("-T", action="callback", nargs=1, type="string",
                      dest="section_vaddr", callback=parse_options,
                      metavar="(bss|data|text) SECT_ADDR",
                      help="Specify the virtaul address of the bss, data or"
                           "text sections")
    parser.add_option("--kernel-soc", action="store_true", default=False,
                      dest="kernel_soc",
                      help="Specify that we are linking the kerenl and soc")
    parser.add_option("--rvct", action="store_true", default=False,
                      dest="rvct", help="Use rvct specific linker script")
    parser.add_option("-v", "--verbose", action="store_true", default=False,
                      dest="verbose", help="Print out processing messages")
    parser.add_option("--verbose-merge", action="store_true", default=False,
                      dest="verbose_merge",
                      help="Print out very detailed merging information")
    parser.add_option("--verbose-script", action="store_true", default=False,
                      dest="verbose_script",
                      help="Print out linker script handling information")
    parser.add_option("--verbose-relocs", action="store_true", default=False,
                      dest="verbose_relocs", help="Print out all reloc info")

    (options, args) = parser.parse_args(args)

    if len(args) < 1:
        parser.error("Invalid number of arguments")

    if options.output_file == None:
        parser.error("Must specify output file")

    elf = link(args, options.section_vaddr, options.kernel_soc, options.rvct,
               options.verbose, options.verbose_merge, options.verbose_script,
               options.verbose_relocs)

    if options.verbose:
        print "Adding default segnames"

    seglist = get_segment_names(elf, None, [], 'gcc')
    add_segment_names(elf, seglist)

    if options.verbose:
        print "Writing out file"

    out_file = elf.prepare(elf.wordsize, elf.endianess)
    out_file.to_filename(options.output_file)

    os.chmod(options.output_file, 0750)

    if options.verbose:
        print "Done"

def link(files, section_vaddr = None, kernel_soc = True, rvct = False,
         verbose = False, verbose_merge = False, verbose_script = False,
         verbose_relocs = False):
    """
    Perform the actual link, split so that elfweaver merge can call this easily.
    """

    # Handle merging of multiple files.
    # Use the first provided elf file as the base file.  For each additonal file
    # merge the sections (.text -> .text) but do no other merging i.e. do not
    # merge any .text.foo into .text.  Update the symbol table and any relocs
    # to take into account the merging then merge in the symbol table and
    # relocation sections.
    base_elf = UnpreparedElfFile(files[0])
    base_elf.sections = []

    if verbose:
        print "Using %s as base file" % files[0]

    base_sym_tab = None

    for merge_file in files:
        merge_elf = UnpreparedElfFile(merge_file)

        if verbose:
            print "Merging in file %s" % merge_file

        sym_tab = [sym_tab for sym_tab in merge_elf.sections
                   if sym_tab.type == SHT_SYMTAB]

        # Things get really, really ugly if there is more than one symbol table,
        # fortunately sane compilers / linkers appear to only have one anyway.
        assert len(sym_tab) == 1

        sym_tab = sym_tab[0]

        merged_sects = []
        reloc_sects = []

        ind = 1
        if base_elf.sections == []:
            ind = 0

        for sect in merge_elf.sections[ind:]:
            # Symbol table and relocations require more specific magic and get
            # handled later on
            if sect.type == SHT_SYMTAB:
                continue
            elif sect.type in (SHT_REL, SHT_RELA):
                reloc_sects.append(sect)
                continue

            found_sect = base_elf.find_section_named(sect.name)
            if found_sect == None:
                # Don't need to merge this section as there is no corrosponding
                # entry in the base file, so just go ahead and add it.
                base_elf.add_section(sect)

                if verbose_merge:
                    print "\tAdding section %s" % sect.name

                continue

            merge_sections(found_sect, sect, merged_sects, None,
                           verbose_merge)

        # Update any symbols or relocations that relied on a merged section
        # to correctly point at the new section at the correct offset
        if verbose:
            print "\tUpdating relocation sections with merged data"

        sym_tab.update_merged_sections(merged_sects)
        for sect in reloc_sects:
            sect.update_merged_sections(merged_sects)

        # Merge the symbol tables, this is more just tricky than any deep magic
        # * For each undefined symbol in the base file try to find a match
        #   in the input file.  If we find one then replace the base file's
        #   symbol with the defined one.  Keep a list of the mappings from the
        #   input files symbols to the new base file symbol index.
        # * Merge the two symbol tables.  For each symbol in the input file's
        #   symbol table;
        #   * If it is undefined, try to find a match in the base file's symbol
        #     table.  If found record the mapping from old symbol to new index.
        #   * If it is defined or there is no match copy it over, again keeping
        #     a mapping from old symbol to new index.
        # * Update all the relocations in the input file to correctly point at
        #   the new symbol table and the correct symbol index.  And merge in
        #   the relocations sections if a section already exists or add them.
        if base_sym_tab:
            if verbose:
                print "\tMerging symbol tables"

            merged_syms = base_sym_tab.resolve(sym_tab)
            merged_syms += base_sym_tab.merge(sym_tab)

            for sect in reloc_sects:
                sect.update_merged_symbols(base_sym_tab, merged_syms)
        else:
            if verbose:
                print "\tAdding symbol table"

            base_elf.add_section(sym_tab)
            base_sym_tab = sym_tab

        for sect in reloc_sects:
            found_sect = base_elf.find_section_named(sect.name)

            if found_sect == None:
                base_elf.add_section(sect)

                if verbose_merge:
                    print "\tAdding relocation section %s" % sect.name
            else:
                found_sect.append_relocs(sect.get_relocs())

                if verbose_merge:
                    print "\tMerging in relocation section %s" % sect.name

    # Now before we lay everything out we need to adjust the size of any
    # sections (such as the .bss or .got) that may increase in size due
    # to allocation of symbols, etc.
    if verbose:
        print "Allocating symbol and relocation data"

    base_sym_tab.allocate_symbols()

    reloc_sects = []
    for sect in base_elf.sections:
        if sect.type in (SHT_REL, SHT_RELA):
            #pylint: disable-msg=E1103
            sect.set_verbose(verbose_relocs)
            sect.allocate_relocs()
            reloc_sects.append(sect)

    # Do any linker scripty things we need to do.  For the moment we either do
    # a standard link or a kernel+soc link, the actions performed are in python
    # functions currently but may be moved into external scripts later.
    if kernel_soc:
        if verbose:
            print "Performing a kernel + soc link, rvct", rvct

        kernel_link_func_types = KERNEL_SOC_LINKER_SCRIPT[base_elf.machine]
        if not rvct:
            kernel_link_func = kernel_link_func_types['gnu']
        else:
            kernel_link_func = kernel_link_func_types['rvct']

        segments, merged_sects, discarded_sects = \
                perform_link(base_elf, base_sym_tab, kernel_link_func,
                             section_vaddr, verbose_script)
    else:
        if verbose:
            print "Performing standard link"

        segments, merged_sects, discarded_sects = \
                perform_link(base_elf, base_sym_tab, standard_link,
                             section_vaddr, verbose_script)

    # Remove any symbols relating to discarded sections and update for any of
    # the merged sections
    if verbose:
        print "Updating symbols for discarded and merged sections"

    discarded_syms = base_sym_tab.update_discarded_sections(discarded_sects)
    base_sym_tab.update_merged_sections(merged_sects)

    if verbose:
        print "Updating relocation sections with new symbols"

    for sect in reloc_sects:
        sect.update_discarded_symbols(discarded_syms)
        sect.update_discarded_sections(discarded_sects)
        sect.update_merged_sections(merged_sects)

    # Segments don't have the correct flags yet, go through and update them
    # based on the types of the included sections.
    for seginfo in segments:
        flags = PF_R

        for sect in seginfo[2]:
            if sect.flags & SHF_WRITE:
                flags |= PF_W
            if sect.flags & SHF_EXECINSTR:
                flags |= PF_X

        seginfo[0] = flags

    for flags, base_addr, sects in segments:
        if len(sects) == 0:
            continue

        new_seg = SectionedElfSegment(base_elf, PT_LOAD, base_addr, base_addr,
                                 flags, SEGMENT_ALIGN, sections=sects)
        base_elf.add_segment(new_seg)

        if verbose:
            print "Adding segment to file"
            print "\tFlags %d (R %d, W %d, X %d)" % (flags, flags & PF_R > 0,
                                                     flags & PF_W > 0,
                                                     flags & PF_X > 0)
            print "\tBase address %x" % base_addr
            print "\tSections", [sect.name for sect in sects]

    # All sections are laid out and at their final size, go through and update
    # symbol values to their final position
    if verbose:
        print "Updating symbols"

    base_sym_tab.update_symbols()

    relocated_all = True
    # Apply all the relocs we know how to handle
    relocs_remove = []

    if verbose:
        print "Applying relocations"

    for sect in reloc_sects:
        if sect.apply():
            relocs_remove.append(sect)
        else:
            relocated_all = False

    if verbose:
        print "Applied all relocations", relocated_all

    for sect in relocs_remove:
        base_elf.remove_section(sect)

    if relocated_all:
        # Set the ELF entry point to _start
        base_elf.entry_point = base_elf.find_symbol("_start").value
        # Set ELF type to an executable
        base_elf.elf_type = ET_EXEC

        if verbose:
            print "Setting entry point to %x" % base_elf.entry_point

    return base_elf

def parse_options(option, opt, value, parser):
    """
    Implement ld's interface for section naming
    --section-start section=addr
    -Tbss addr
    -Tdata addr
    -Ttext addr
    """

    if getattr(parser.values, option.dest) == None:
        setattr(parser.values, option.dest, {})

    if opt == "--section-start":
        args = value.split('=')
        getattr(parser.values, option.dest)[args[0]] = int(args[1], 16)
    elif opt == "-T":
        if value in ("bss", "data", "text"):
            # Trickery to reuse -T for two types of operation.  While the
            # parser only has one argument for this type nothing stops us
            # manually eating additional arguments.
            getattr(parser.values, option.dest)['.' + value] = \
                    int(parser.rargs.pop(0), 16)
