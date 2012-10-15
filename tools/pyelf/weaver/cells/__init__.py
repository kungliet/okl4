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
Package description and base class for elfweaver cell support.
"""

import glob
import os


from weaver.parse_spec import XmlCollector
from weaver.util import import_custom

def import_custom_cells(path):
    return import_custom(path, "cells", "cell.py")


class Cell(XmlCollector):
    #pylint: disable-msg=R0913
    """
    Abstract base class for all cell support objects.

    An instance of this class is created for each cell element that is
    found in the XML file.  This object is responsible for parsing the
    subelements of the cell element and generating the relevant data
    structures.

    The protocol driving this class is as followed:

    1) When a new cell element is found an object of the relevant
       subclass will be created.

    2) cell.collect_xml() will be called to subelements of of the
       cell XML.

    3) After all elements have been processed,
       generate_dynamic_segments() will be called to allow the cell to
       create any further objects (such as additional memory regions)
       whose values and size depend on access to all of the processed
       data.

    4) image.layout() is called to place all objects in memory.

    5) cell.generate_init() will be called to place any cell specific
       initialisation data into buffers.

    When a subclass is imported is must call Cell.register() to map a
    cell tag to the relevant class.
    """

    element = NotImplemented

    def __init__(self):
        pass

    def collect_xml(self, elem, ignore_name, namespace, machine,
                    pools, kernel, image):
        """
        Parse XML elements and create an intermediate representation of
        the system.
        """
        raise NotImplementedError

    def generate_dynamic_segments(self, namespace, machine, pools,
                                  kernel, image):
        """
        Create any segments, such as bootinfo buffers, whose size depends
        on the parsed XML.
        """
        raise NotImplementedError

    def generate_init(self, machine, pools, kernel, image):
        """
        Generate the init script for the cell, placing it into a segment.
        """
        raise NotImplementedError

    def get_cell_api_version(self):
        """
        Return the libOKL4 API versions that the cell initial program
        was build against.
        """
        raise NotImplementedError

    def group_elf_segments(self, image):
        """
        Group ELF segments together intelligently, avoiding potential
        domain conflicts.
        """
        pass

