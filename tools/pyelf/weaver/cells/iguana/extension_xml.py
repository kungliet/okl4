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

from elf.core import UnpreparedElfFile
from elf.constants import ET_EXEC
from weaver import MergeError
from weaver.segments_xml import collect_elf_segments, collect_patches, \
     start_to_value
import weaver.util
import os

def get_symbol(elf, symbol, may_not_exist = False):
    try:
        return weaver.util.get_symbol(elf, symbol, may_not_exist)
    except MergeError:
        print "warn: cannot find symbol ", symbol
        return None

def collect_extension_element(extension_el, pd, namespace, ig_elf, image,
                              machine, bootinfo, pools):
    # New namespace for objects living in the extension.
    extn_namespace = namespace.add_namespace(extension_el.name)

    elf   = None
    start = None
    name  = None
    physpool = getattr(extension_el, 'physpool', None)
    pager    = getattr(extension_el, 'pager', None)
    direct   = getattr(extension_el, 'direct', None)

    # Push the overriding pools for the extension.
    image.push_attrs(physical = physpool,
                     pager    = pager,
                     direct   = direct)

    if hasattr(extension_el, "file"):
        elf = UnpreparedElfFile(filename=os.path.join(extension_el._path, extension_el.file))

        if elf.elf_type != ET_EXEC:
            raise MergeError, "All the merged ELF files must be of EXEC type."

        segment_els = extension_el.find_children("segment")
        segs = collect_elf_segments(elf,
                                    image.EXTENSION,
                                    segment_els,
                                    extension_el.name,
                                    [],
                                    extn_namespace,
                                    image,
                                    machine,
                                    pools)
        
        segs_ms = [bootinfo.record_segment_info(extension_el.name,
                   seg, image, machine, pools) for seg in segs]

        for seg_ms in segs_ms:
            pd.attach_memsection(seg_ms)

        # Record any patches being made to the program.
        patch_els = extension_el.find_children("patch")
        collect_patches(elf, patch_els, os.path.join(extension_el._path, extension_el.file), image)

        start = elf.entry_point
        name  = os.path.join(extension_el._path, extension_el.file)

    if hasattr(extension_el, "start"):
        start = extension_el.start
        name  = extension_el.name

        # If no file is supplied, look for symbols in the root
        # program.
        if elf is None:
            elf = ig_elf

    elf = elf.prepare(elf.wordsize, elf.endianess)
    start = start_to_value(start, elf)

    bootinfo.add_elf_info(name = name,
                               elf_type = image.EXTENSION,
                               entry_point = start)

    image.pop_attrs()

