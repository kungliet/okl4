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
Hacky elfweaver command to link an ELF using either elfadorn or elfweaver.
"""

import sys
import subprocess
import optparse

from adorn.arguments import get_script_names
from adorn.main import run_adorn

from weaver.link import link_cmd

# FIXME: RVCT is a horrible, terrible mess.  Kill it.
# The --partial command appers to do nothing compared to the GNU linker.  We end
# up with 2000+ (!!!!) sections with many duplicates (10+ .text, .data, etc.).
# Obviously it doesn't even attempt to merge same named sections from the files
# passed on the commandline.  Need to have better section merging before we can
# cope with this.  And fun section names like 'i.L4_FpageLog2', what are we
# meant to do with them? O.o  Also we have sections of type GROUP that need to
# be dealt with.

def hacky_link_cmd(args):
    """
    Link an ELF using either elfweaver or elfadorn.
    """

    print "********"
    print "WARNING: elfweaver hacky_link is for internal debugging and testing purposes only."
    print "********"
    print

    # We should be able ot use 'elfadorn ld' as a drop-in replacement for 'ld'
    # Here we detect if this is the case, and patch the command line args
    # appropriately. This way we avoid maintainig two different methods of
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

    create_segments = options.create_segments
    file_segment_list = options.file_segment_list
    cmd_segment_list = options.cmd_segment_list
    if cmd_segment_list:
        cmd_segment_list = cmd_segment_list.split(r',')
    target = options.target

    scripts = get_script_names(args)

    # If there are any scripts let elfadorn handle it
    if len(scripts) != 0 or args[0].lower().find('rvds') != -1:
        run_adorn(args, target, create_segments, file_segment_list,
                  cmd_segment_list)
        return

    # Okay, now we want elfweaver to link things
    # First, run the linker command after inserting an -r into the commands
    args.insert(1, '-r')

    if subprocess.Popen(args).wait() != 0:
        sys.exit(1)

    # Construct the link command's arguments
    target = args[args.index('-o') + 1]
    text_loc = None
    data_loc = None

    # Because apparantly one can use both -Ttext addr and -Ttext=addr as valid
    # syntax.  Blarg.
    if '-Ttext' in args:
        text_loc = args[args.index('-Ttext') + 1]
    if '-Tdata' in args:
        data_loc = args[args.index('-Tdata') + 1]
    for arg in args:
        if len(arg) == 6:
            continue
        if arg.find('-Ttext') == 0:
            text_loc = arg.split('=')[1]
        if arg.find('-Tdata') == 0:
            data_loc = arg.split('=')[1]

    link_args = ['-o', target, target]
    if text_loc != None:
        link_args.append('-Ttext')
        link_args.append(text_loc)
    if data_loc != None:
        link_args.append('-Tdata')
        link_args.append(data_loc)

    link_cmd(link_args)
