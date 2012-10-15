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

"""Collect data from the kernel element."""

from weaver import MergeError
from weaver import util
from weaver.ezxml import Element, long_attr, str_attr
from weaver.segments_xml import Segment_el, Patch_el, Heap_el

DEFAULT_KERNEL_HEAP_SIZE = 4 * 1024 * 1024
DEFAULT_KERNEL_MAX_THREADS = 1024

UseDevice_el = Element("use_device", name = (str_attr, "required"))

Option_el = Element("option",
                    key   = (str_attr, "required"),
                    value = (long_attr, "required"))

Config_el = Element("config", Option_el)

# Compound Elements
Kernel_el = Element("kernel", Segment_el, Patch_el, Heap_el,
                    Config_el, UseDevice_el,
                    file          = (str_attr, "optional"),
                    sdk           = (str_attr, "optional"),
                    configuration = (str_attr, "optional"),
                    platform      = (str_attr, "optional"),
                    linker        = (str_attr, "optional"),
                    kernel        = (str_attr, "optional"),
                    soc           = (str_attr, "optional"),
                    libs          = (str_attr, "optional"),
                    virtpool      = (str_attr, "required"),
                    physpool      = (str_attr, "required"),
                    seg_names     = (str_attr, "optional"))

Rootserver_el = Element("rootserver", Segment_el,
                        filename = (str_attr, "required"))

def get_symbol(elf, symbol, may_not_exist = False):
    """
    Return the address, and size in bytes, of a symbol from the ELF
    file.

    If may_not_exist is true then failing to find the symbol is not a
    fatal error.
    """
    try:
        return util.get_symbol(elf, symbol, may_not_exist)
    except MergeError:
        raise MergeError, \
              'Symbol "%s" not found in kernel ELF file.'  % \
              (symbol)
