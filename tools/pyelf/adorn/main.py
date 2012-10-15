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
The Elfadorn script turns orphaned sections in object files into segments
in the output file by adding to the linker script.
"""

import sys
import os
import optparse
from elf.core import UnpreparedElfFile
from elf.constants import ET_EXEC
from elf.segnames import get_segment_names, add_segment_names

from adorn.arguments import get_script_names, remove_arguments, \
     get_linker_name
from adorn.sections import get_section_names, remove_sections_wildcard
from adorn.linkerscript import get_linker_script_text, \
     linker_script_sections, write_linker_script


def main(args):
    """Main program entry point."""

    # We should be able ot use 'elfadorn ld' as a drop-in replacement for 'ld'
    # Here we detect if this is the case, and patch the command line args
    # appropriately. This way we avoid maintainid two different methods of
    # dealing with args
    if "--" not in args:
        args.insert(1, "--")

    parser = optparse.OptionParser("%prog [options] -- <linker> [linker_options]",
                                   add_help_option=0)
    parser.add_option("-H", "--help", action="help")
    parser.add_option("-o", "--output", dest="target", metavar="FILE",
                        help="Linker will create FILE.")
    parser.add_option("-f", "--file-segment-list", dest="file_segment_list",
                      metavar="FILE",
                      help="File containing segment names to be added to \
                      .segment_names, one per line")
    parser.add_option("-c", "--cmd-segment-list", dest="cmd_segment_list",
                        help="quoted list of comma separated segment names to \
                        be added to .segment_names,")

    parser.add_option("-s", "--create-segments", dest="create_segments",
                      action="store_true",
                      help="Set to enable gathering orphaned sections and \
                      placing each in a new segment")

    (options, args) = parser.parse_args(args)

    # we need to parse the options
    # we are interested in any -T, --scatter or --script= options
    # plus the ordinary files specified on the command line    

    create_segments = options.create_segments
    file_segment_list = options.file_segment_list
    cmd_segment_list = options.cmd_segment_list
    if cmd_segment_list:
        cmd_segment_list = cmd_segment_list.split(r',')
    target = options.target
    linker_cmd = args[1:]    
    run_adorn(linker_cmd, target, create_segments, file_segment_list,
              cmd_segment_list)

def run_adorn(linker_cmd, target=None, create_segments=False,
              file_segment_list=None, cmd_segment_list=None):
    """
    The programming interface to elf adorn.

    linker_cmd: A list of strings representing the linker command to be executed
    target: A target file into which to place the final output. If not
            specified, a "-o" option is expected in the linker_cmd
    create_segment: A boolean indicating whether orphaned sections should be
                    gathered and each placed in a new segment
    file_segment_list: The filename of a file containing segment names to be
                       added to .segment_names. (Mutually exclusive with
                       cmd_segment_list, this takes precedence if both given)
    cmd_segment_list: A list of segment names to be added to .segment_names.
                      (Mutually exclusive with file_segment_names)
    """
    
    (linker_name, linker_args,
     linker_type, scripts, object_files) = _parse_linker_cmd(linker_cmd)
    target = _get_target(target, linker_args)

    if linker_type == "rvct":
        if create_segments:
            print "Warning: creating segments from sections not applicable " \
                  "to RVCT. Disabling option."
        create_segments = False

    # segments specifed in a file
    if file_segment_list:
        segfile = open(file_segment_list, 'r')
        seglist = segfile.readlines()
    # segments specified on command line
    elif cmd_segment_list:
        seglist = cmd_segment_list
    else:
        seglist = None

    # next get section names
    (sections, additional_scripts) = get_section_names(object_files)
    scripts += additional_scripts

    # then get the text of the linker script
    # FIXME: why no linker args?
    script_text = get_linker_script_text(linker_name, scripts,
                                         additional_scripts, [])

    if create_segments:
        # get rid of any sections named in the script_text
        mentioned_sections = linker_script_sections(script_text)
        orphaned_sections = sections
        for section in mentioned_sections:
        # Our grammar is not perfect, sometimes it gets confused and gives back
        # * as a section but it is actually a filename
            if section != "*":
                remove_sections_wildcard(orphaned_sections, section)

        # mips-ld treats .reginfo sections somewhat magically, we do not want
        # to treat this as an orphan and create a segment for him, else ld will
        # drop the text data and bss sections completely. Magic.
        if '.reginfo' in orphaned_sections:
            orphaned_sections.remove('.reginfo')

        # work out the new linker command line
        if scripts == []:
            default = get_linker_script_text(linker_name, [], [], linker_args)
            open("default.lds", "w").write(default)
            scripts.append("default.lds")
            if len(orphaned_sections) != 0:
                linker_cmd += ["--script=default.lds"]

        # write out an additional linker script file to pass to the linker
        if orphaned_sections:
            write_linker_script(orphaned_sections, "additional.lds")
            scripts.append("additional.lds")
            linker_cmd += ["--script=additional.lds"]
    else:       
        # if we dont care about these, just say there are none.
        orphaned_sections = []

    # execute the linker
    if os.spawnvp(os.P_WAIT, linker_name, linker_cmd) != 0:
        sys.exit(1)

    _update_elf_file(target, seglist, scripts, linker_type, orphaned_sections)

def _parse_linker_cmd(linker_cmd):
    linker_name = linker_cmd[0]
    linker_args = linker_cmd[1:]
    linker_type = get_linker_name(linker_cmd, linker_name)
    scripts = get_script_names(linker_cmd)
    object_files = remove_arguments(linker_args)
    return linker_name, linker_args, linker_type, scripts, object_files


def _update_elf_file(target, seglist, scripts, linker_type, orphaned_sections):
    # load the elf file
    elf = UnpreparedElfFile(filename=target)

    if elf.elf_type != ET_EXEC:
        return

    seglist = get_segment_names(elf, seglist, scripts, linker_type,
                                 orphaned_sections)
    add_segment_names(elf, seglist)

    elf = elf.prepare(elf.wordsize, elf.endianess)
    # write the file
    elf.to_filename(target)



def _get_target(target, args):
    if not target:
        if "-o" in args:
            i = args.index("-o")
            target = args[i+1]

        if not target:
            print "Error: -o flag must be supplied." 
            sys.exit(1)
    return target
        

