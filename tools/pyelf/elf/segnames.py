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
Functions for the segment name section.

The segment name section maps strings to segment indexes.
"""

from elf.section import PreparedElfSection, UnpreparedElfStringTable
from elf.constants import PF_R, PF_W, PF_X, PT_LOAD

import parsers.rvct.rvct_parser as rvct_parser
import parsers.gnu.gnu_parser as gnu_parser

def get_segment_names(elf, seglist = None, scripts = None, linker_name = None,
        orphans = None):
    """
    Return a list of names that correspond to the segments in the ELF
    file.

    The names can come from the command line, a file, the program
    headers listed in a linker script or generated from the flag
    values of the segments themselves.
    """

    if scripts is not None and len(scripts) != 0:
        scripts = scripts[0]

    if orphans is None:
        orphans = []

    # segments specifed in a file
    if seglist is not None:
        seglist = _pad_non_loaded(elf, seglist)
    # get segments from linker script
    elif scripts is not None and len(scripts) != 0 and linker_name == "rvct":
        segments_to_sections_dict = rvct_parser.extract_segment_names(scripts)
        # sections may appear in more than one segment
        # the correct segment in the linker script
        seglist = _seg_list_rvct(elf, segments_to_sections_dict)

    # get segments from linker script
    elif scripts is not None and len(scripts) != 0 and linker_name == "gcc":
        seglist = gnu_parser.extract_segment_names(scripts)
    # otherwise we use the defaults
    else:
        seglist = []

    # If no segment names are found, then generate default names.
    # Loadable segments are named according to their ELF flags.  Other
    # segments are unnamed because they are unused by elfweaver.
    if len(seglist) < len(elf.segments):
        seglist_enum = []
        segcount = 0

        for seg in elf.segments[len(seglist):]:
            seg_name = ""

            if seg.has_sections():
                for sect in seg.get_sections():
                    if sect.name in orphans:
                        seg_name = sect.name

            if seg_name == "":
                if seg.flags & PF_R:
                    seg_name += 'r'

                if seg.flags & PF_W:
                    seg_name += 'w'

                if seg.flags & PF_X:
                    seg_name += 'x'

                if seg_name == "":
                    seg_name = "??"

            seglist_enum.append((segcount, seg_name))
            segcount += 1

        # Find and flag all segments which have non-unique flags
        counts = {}
        for seg in elf.segments:
            counts[seg.flags] = counts.get(seg.flags, 0) + 1
        duplicates = [counts[seg.flags] > 1 for seg in elf.segments]

        # For all segments which have duplicate names:
        # First try to append the name of their first section
        # Failing that, append their segment number
        # (which is actually its position in the segment lsit)
        # Finally, construct the seglist that will be returned
        for i, seg_name in seglist_enum:
            is_dup = duplicates[i]
            if is_dup == True and seg_name not in orphans:
                seg_i = elf.segments[i]
                if seg_i.has_sections():
                    first_sect_name = seg_i.get_sections()[0].name
                    if first_sect_name.startswith("."):
                        seg_name += "-" + first_sect_name.split(".")[1]
                    else:
                        seg_name += "-" + first_sect_name
                else:
                    seg_name += "-%02d" % i
            seglist.append(seg_name)

    return seglist

class SegnameElfStringTable(UnpreparedElfStringTable):
    """
    A customised string table implementation that allows nul strings
    to appear in places other than the start of the table.  This will
    mean that there will be exactly <n> string if there are <n>
    segments, even if some of the segments are unnamed.

    The calculation of the offset is very inefficient if the number of
    segments is large
    """
    prepares_to = PreparedElfSection

    def __init__(self, elffile):
        UnpreparedElfStringTable.__init__(self, elffile, ".segment_names")

    def add_string(self, string):
        """Add a new string to the table. Return the data offset."""
        offset = sum([len(s) for s in self.strings])

        self.strings.append(string + '\x00')
        self.offsets[string] = offset
        self.string_dict[offset] = string
        return offset

def add_segment_names(elf, seglist):
    """
    Add a segment name string table to an elf file, containing the given list
    of segment names
    """
    # create the string table
    segname_tab = SegnameElfStringTable(elf)

    # add the segment names
    for segname in seglist:
        segname = segname.strip()
        segname_tab.add_string("%s" % segname)

    # add the table to the file
    elf.add_section(segname_tab)

def _seg_list_rvct(elf, segments_to_sections_dict):
    return [_seg_from_script_guess(seg, segments_to_sections_dict) for seg in elf.segments]

def _seg_from_script_guess(seg, segments_to_sections_dict):
    seg_score = _score_segments(seg, segments_to_sections_dict)
    max_score = 0
    best_guess = "ERROR: NO GUESS"
    for scored_seg in seg_score.keys():
        if seg_score[scored_seg] > max_score:
            max_score = seg_score[scored_seg]
            best_guess = scored_seg

    return best_guess

def _score_segments(seg, segments_to_sections_dict):
    """ """
    seg_score = {}
    for sec in seg.get_sections():
        script_seg = _seg_from_sec_script(sec, segments_to_sections_dict)

        if script_seg not in [seg_score.keys()]:
            seg_score[script_seg] = 0
        seg_score[script_seg] += 1
    return seg_score

def _seg_from_sec_script(sec, segments_to_sections_dict):
    """Goes through the dictionary of segment to section mappings from the
    linker script and returns the segment that contains a given section
    """
    for segment, script_sections in segments_to_sections_dict.items():
        if sec.name in script_sections:
            return segment


def _pad_non_loaded(elf, names):
    """
    Create a list of names for all segments in an ELF file from a list
    of names for the loadable segments.  Non-loadable segments are
    named ''.
    """
    names = names[:] # Make a copy to manipulate.
    seglist = []

    for seg in elf.segments:
        if seg.type == PT_LOAD:
            seglist.append(names[0])
            del names[0]
        else:
            seglist.append("")

    return seglist
