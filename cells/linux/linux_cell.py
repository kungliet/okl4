#####################################################################
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
#####################################################################

"""
Build system classes for the linux cell.
"""

from kenge import OKL4Cell
import SCons
import os

class LinuxCell(OKL4Cell):
    """Cell for OKLinux."""
    def __init__(self, name, dom, SYSTEM, CUST, MACHINE, TOOLCHAIN,
                 BUILD_DIR, xml_tag = None, **kargs):
        OKL4Cell.__init__(self, name, dom, SYSTEM, CUST, MACHINE,
                            TOOLCHAIN, BUILD_DIR,
                            xml_tag = "linux", **kargs)

    def set_program(self, node, stack = None, heap = None,
                    patches = None, segments = None,
                    irqs = None, args = None,
                    addressing = None):
        OKL4Cell.set_program(self, node, stack, heap, patches, segments, irqs,
                             addressing)

        if args is not None:
            if not isinstance(args,  SCons.Node.Node):
                args = self.CommandLine(args=args)

            self.elem.appendChild(args.attributes.xml)
            self.apps.append(args)

    def CommandLine(self, args = None):
        """
        Create a description of a program's command line arguments.
        """
        if args is None:
            args = []

        elem = self.dom.createElement("commandline")

        for arg in args:
            arg_elem = self.dom.createElement("arg")

            arg_elem.setAttribute('value', str(arg))

            elem.appendChild(arg_elem)

        cmd = self.scons_env.Value(elem.toxml())
        self.set_scons_attr(cmd, 'xml', elem)

        return cmd

    def add_file(self, prog, node, name = None, addressing = None):
        """
        Add a file to the final boot image.  Based on the original
        Iguana implementation.
        """
        if addressing is None:
            addressing = self.Addressing(align = self.ALIGN_PREFERRED)

        if name is None:
            name = os.path.basename(node[0].abspath)

        elem = self.dom.createElement("memsection")

        elem.setAttribute('name', name)
        elem.setAttribute('file', node[0].abspath)

        addressing.set_attrs(elem)

        self.elem.appendChild(elem)

        ms = self.scons_env.Value(elem.toxml())
        self.set_scons_attr(ms, 'xml', elem)

        return ms

