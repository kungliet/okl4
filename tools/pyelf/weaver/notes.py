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
Class for an image description notes section.
"""

import pickle
from elf import section, ByteArray

class NotesSection(object):
    """
    Represents the 'elfweaver.notes' section in the elf file.

    This section details aspects of the image that are known when
    merging and required by memstats but cannot otherwise be derived
    from the image.

    As it is only ever used by python code, it uses pickle to
    convert to/from a byte stream for putting in the elf section.
    """

    #
    # Some constants
    #
    ROM = 0
    RAM = 1

    def __init__(self, kernel = None):
        self.cpu = ""
        self.memsecs = []
        self.cell_names = []
        self.space_names = []
        self.mappings = []
        self.poolname = None

        if kernel is not None:
            self.cpu = kernel.machine.cpu

            for key in kernel.machine.physical_mem.keys():
                desc = kernel.machine.physical_mem[key][0]
                self.memsecs.append((key, desc[0], desc[1]))

            for key in kernel.cells.keys():
                self.cell_names.append(key)

                for space in kernel.cells[key].spaces:
                    self.space_names.append(space.ns_node.name)

                    for mapping in space.mappings:
                        attrs = mapping[2]
                        self.mappings.append((attrs.abs_name(),
                                              attrs.phys_addr,
                                              attrs.virt_addr,
                                              attrs.size))

    def set_mempool(self, pool):
        """Record the name of the default memory pool."""
        self.poolname = pool

    def encode(self):
        """
        Encode the python structure for inclusion in the notes
        section.
        """
        return pickle.dumps(self)

    def encode_section(self, elf):
        """
        Generate the contents of the notes section.
        """
        note = section.UnpreparedElfNote(elffile=elf,
                                         name='elfweaver.notes')
        note.note_name = "Memstats Details"
        note.note_type = 1
        encoded = self.encode()
        note._desc_data = ByteArray.ByteArray(encoded)

        return note

    @classmethod
    def decode(cls, notes):
        """Decode the python structure in the notes section."""
        data = notes.get_desc_data()
        decoded = data.tostring()

        return pickle.loads(decoded)


def find_elfweaver_notes(elf):
    """
    Find the elfweaver notes section in an ELF.
    Return None if it cannot be found.
    """
    notes = elf.find_section_named("elfweaver.notes")

    if notes:
        notes = NotesSection.decode(notes)

    return notes
