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
Build system classes for the kernel pseudo-cell.

The kernel itself is not a cell, but its behaviour is sufficiently
close, that is useful to model it as one.
"""

from kenge import KengeCell
from util import xmlbool

class KernelCell (KengeCell):
    """
    A pseudo-cell for the kernel.
    """
    def __init__(self, name, dom, SYSTEM, CUST, MACHINE, TOOLCHAIN,
                 BUILD_DIR, xml_tag = None, **kargs):
        KengeCell.__init__(self, name, dom, SYSTEM, CUST, MACHINE,
                           TOOLCHAIN, BUILD_DIR,
                           xml_tag = 'kernel', **kargs)

        self.kernel = None

    def DefaultMemoryPools(self):
        """
        Convert the memory ranges specified in machines.py into iguana
        memory pools.

        Iterate through the memory types for the current machine (as
        found in self.machines.memory) and process them in the
        following ways:

        1) If the memory type is in the skipped_memory list then skip

        2) If the memory is called 'virtual' create a virtual pool

        3) For other memory types, create a physical memory pool

        The root program default pools are set to be the pools called
        'virtual' and 'physical'.
        """
        pools = []
        top_elem = self.dom.documentElement
        if len(top_elem.childNodes) > 1:
            before_elem = top_elem.childNodes[1]
        else:
            before_elem = None

        for mem in self.machine.memory.keys():
            if len(self.machine.memory[mem]) is 0:
                continue

            if mem == 'virtual':
                pool_elem = self.dom.createElement("virtual_pool")
                pool_elem.setAttribute('name', mem)
            else:
                pool_elem = self.dom.createElement("physical_pool")
                pool_elem.setAttribute('name', mem)
                pool_elem.setAttribute('direct', 'true')

            elem = self.dom.createElement("memory")
            elem.setAttribute('src', mem)
            pool_elem.appendChild(elem)

            # Insert the pool element before the kernel element.
            top_elem.insertBefore(pool_elem, before_elem)

            pool = self.scons_env.Value(pool_elem.toxml())
            self.set_scons_attr(pool, 'xml', pool_elem)

            pools.append(pool)

        self.set_default_pools('virtual', 'physical')
        return pools

    def DefaultSplitMemoryPools(self, num):
        """
        Convert the memory ranges specified in machines.py into iguana
        memory pools.

        Iterate through the memory types for the current machine (as
        found in self.machines.memory) and process them in the
        following ways:

        1) If the memory type is in the skipped_memory list then skip

        2) If the memory is called 'virtual' create a virtual pool

        3) For other memory types, create a physical memory pool

        The root program default pools are set to be the pools called
        'virtual' and 'physical'.

        XXX: As above but we split up the physical pool into n number of
        entries
        """
        pools = []
        top_elem = self.dom.documentElement
        if len(top_elem.childNodes) > 1:
            before_elem = top_elem.childNodes[1]
        else:
            before_elem = None

        for mem in self.machine.memory.keys():
            if len(self.machine.memory[mem]) is 0:
                continue

            if mem == 'virtual':
                pool_elem = self.dom.createElement("virtual_pool")
                pool_elem.setAttribute('name', mem)
            elif mem == 'physical':
                node = self.machine.memory[mem]
                base = node[0].get_base()
                size = node[0].get_size()
                num_pages = size / self.machine.page_sizes[0]
                size = num_pages / num * self.machine.page_sizes[0]
                index = 0
                while index < num:
                    pool_elem = self.dom.createElement('physical_pool')
                    pool_elem.setAttribute('name', mem + str(index))
                    pool_elem.setAttribute('direct', 'true')

                    elem = self.dom.createElement("memory")
                    elem.setAttribute('base', "%#x" % (base + index * size))
                    elem.setAttribute('size', "%#x" % size)
                    pool_elem.appendChild(elem)

                    # Insert the pool element before the kernel element.
                    top_elem.insertBefore(pool_elem, before_elem)

                    pool = self.scons_env.Value(pool_elem.toxml())
                    self.set_scons_attr(pool, 'xml', pool_elem)
                    pools.append(pool)

                    index += 1

                break
            else:
                pool_elem = self.dom.createElement("physical_pool")
                pool_elem.setAttribute('name', mem)
                pool_elem.setAttribute('direct', 'true')

            elem = self.dom.createElement("memory")
            elem.setAttribute('src', mem)
            pool_elem.appendChild(elem)

            # Insert the pool element before the kernel element.
            top_elem.insertBefore(pool_elem, before_elem)

            pool = self.scons_env.Value(pool_elem.toxml())
            self.set_scons_attr(pool, 'xml', pool_elem)

            pools.append(pool)

        self.set_default_pools('virtual', 'physical0')
        return pools

    def set_default_pools(self, virtpool, physpool):
        """Set the system default pools."""
        self.elem.setAttribute('virtpool', virtpool)
        self.elem.setAttribute('physpool', physpool)

    def set_kernel_config(self,
                          max_threads    = None,
                          heap_size      = None,
                          heap_phys_addr = None,
                          heap_align     = None):
        """
        Set the kernel configuration properties.

        This must be called after set_kernel().
        """
        if max_threads is not None:
            config = self.dom.createElement("config")
            self.elem.appendChild(config)

            elem = self.dom.createElement("option")
            elem.setAttribute('key', 'threads')
            elem.setAttribute('value', str(max_threads))

            config.appendChild(elem)
            elem_node = self.scons_env.Value(elem.toxml())
            self.set_scons_attr(elem_node, 'xml', elem)
            self.apps.append(elem_node)

        if heap_size is not None or heap_phys_addr is not None or \
           heap_align is not None:
            elem = self.dom.createElement("heap")

            if heap_size is not None:
                elem.setAttribute('size', "%#x" % heap_size)

            if heap_phys_addr is not None:
                elem.setAttribute('phys_addr', "%#x" % heap_phys_addr)

            if heap_align is not None:
                elem.setAttribute('align', "%#x" % heap_align)

            self.elem.appendChild(elem)

            elem_node = self.scons_env.Value(elem.toxml())
            self.set_scons_attr(elem_node, 'xml', elem)
            self.apps.append(elem_node)

        val = self.scons_env.Value(self.elem.toxml())
        self.set_scons_attr(val, 'xml', self.elem)
        self.apps.append(val)

    def set_kernel(self, node, patches = None, segments = None,
                   addressing = None):
        """
        Set the details of the kernel.
        """
        if addressing is None:
            if env.machine.platform == "pc99":
                kernel_phys_addr = self.ADDR_AS_BUILT
            else:
                kernel_phys_addr = self.ADDR_ALLOC

            addressing = \
                       self.Addressing(
                virt_addr = 0x0,
                align = self.ALIGN_PREFERRED,
                phys_addr = kernel_phys_addr)

        self.kernel = node
        self.apps.append(node)

        self.program_like(node, self.elem, addressing,
                          patches = patches,
                          segments = segments)

