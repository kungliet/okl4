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
Generic class for caps in the elfweaver namespace.
"""

from weaver import MergeError

class Cap:
    """
    Generic class for caps in the elfweaver namespace.
    """

    rights = {}

    def __init__(self, name, obj, rights=None):
        self.name = name
        self.object = obj
        self.rights = []

    def get_name(self):
        """Return the name of the cap."""
        return self.name

    def add_right(self, right):
        """Add a right to the cap."""
        # Need some error checking here.

        if right not in Cap.rights:
            raise MergeError, "'%s' not a supported right." % right

        self.rights.append(right)

#--------------------------------------------------------------------------
# Elfweaver namespace caps for kernel objects.

class LocalCap(object):
    """
    The interface to cap in the destination clist that refers to the
    target object.  The cap_id is returned by get_value() and can be
    placed in the environment.
    """
    def get_cap_slot(self):
        """
        Return the cap slot of the cap that refers to the target object.
        """
        raise NotImplementedError

class CellCap(Cap):
    """
    Describes namespace cap entries that refer to kernel concepts such
    as spaces, threads and physical segments.
    """
    def __init__(self, name, object, rights=None):
        Cap.__init__(self, name, object, rights)

        self.dest_cap = None

    def export(self, space):
        """
        Return a local cap, that has been placed in the space's clist,
        which refers to the target object with this cap's permissions.
        """
        raise NotImplementedError

class MutexCellCap(CellCap):
    """
    Cap for Mutexes.

    This caps all have the same permissions.
    """
    def export(self, space):
        """
        Return a local cap, that has been placed in the space's clist,
        which refers to the target mutex.
        """
        return space.add_mutex_cap(self.object)

class ThreadCellCap(CellCap):
    """
    Cap for threads.  The destination space only has IPC rights to the
    thread.
    """
    def export(self, space):
        """
        Return a local cap, that has been placed in the space's clist,
        which refers to the target thread.
        """
        return space.add_ipc_cap(self.object)

class PhysSegCellCap(CellCap):
    """
    Cap for physical segments.  The physical segment is duplicated in
    the destination's segment list.

    The object passed to the constructor must be the tuple returned by
    register_mapping().
    """
    class PhysSegLocalCap(LocalCap):
        """
        Pseudo cap for physical segments.  The segment number is used
        instead of the cap slot number.
        """
        def __init__(self, cap_slot):
            self.cap_slot = cap_slot

        def get_cap_slot(self):
            """
            Return the segment number for the shared memory region.
            """
            return self.cap_slot

    def export(self, space):
        """
        Return a local cap, that gives the segment number of the local
        copy of the mapping.
        """
        # Create a new mapping only if one doesn't exist already.
        # This takes care of cells that add extra environment entries
        # for their own memsections.
        existing_mapping = [mapping for mapping in space.mappings
                            if self.object[2] == mapping[2]]

        if len(existing_mapping) != 0:
            new_mapping = existing_mapping[0]
        else:
            new_mapping = space.register_mapping(self.object[2])

        return self.PhysSegLocalCap(new_mapping[0])

class PhysAddrCellCap(CellCap):
    """
    A cap to the physical address of a memsection.

    The physical address is only needed for DMA and device drivers
    it is not included in the segment table that is placed in the
    environment by default.
    """

    class PhysAddrLocalCap(LocalCap):
        """
        Pseudo cap for physical addresses.  The physical address is
        used instead of the cap slot number.
        """
        def __init__(self, attrs):
            self.attrs = attrs

        def get_cap_slot(self):
            """
            Return the segment number for the shared memory region.
            """
            return self.attrs.phys_addr

    def export(self, space):
        """
        Return a local cap which contains the physical address of the
        memory object.
        """

        return self.PhysAddrLocalCap(self.object)
