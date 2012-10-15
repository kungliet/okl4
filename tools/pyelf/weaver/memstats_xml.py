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
Processing of the memstats XML element.
"""

from weaver.ezxml import Element, str_attr, size_attr

#############################################################################
#
# Declare the XML elements
#
#############################################################################
RevisionStats_el = Element("revision",
                           repository = (str_attr, "optional"),
                           changeset = (str_attr, "optional"))

#
# Construction of elements leading to environment
#
ID_Range_el = Element("id_range",
                      name = (str_attr, "required"),
                      node = (str_attr, "required"))

Pool_el = Element("pool",
                  id   = (str_attr, "required"),
                  vmid = (str_attr, "required"))

Thread_el = Element("thread",
                    pool_id   = (str_attr, "required"),
                    handle_id = (str_attr, "required"),
                    clist_id  = (str_attr, "required"))

Mutex_el = Element("mutex",
                   pool_id = (str_attr, "required"),
                   clist_id  = (str_attr, "required"))

PSec_el = Element("psec",
                  name = (str_attr, "required"),
                  phys = (str_attr, "required"),
                  virt = (str_attr, "required"))

PSecList_el = Element("psec_list",
                      PSec_el)

Object_el = Element("object",
                     type = (str_attr, "required"),
                     name = (str_attr, "required"),
                     size = (size_attr, "required"))

ObjectList_el = Element("object_list",
                        Object_el,
                        num_text = (size_attr, "required"),
                        num_data = (size_attr, "required"))

Program_el = Element("program",
                      PSecList_el,
                      ObjectList_el,
                      name = (str_attr, "required"))

Space_el = Element("space",
                   ID_Range_el,
                   Pool_el,
                   Program_el,
                   Thread_el,
                   Mutex_el,
                   name = (str_attr, "required"))

Cell_el = Element("cell",
                  ID_Range_el,
                  Pool_el,
                  Space_el,
                  name = (str_attr, "required"))

Kernel_el = Element("kernel",
                  ID_Range_el,
                  Pool_el,
                  Space_el)

Environment_el = Element("environment",
                         Kernel_el,
                         Cell_el,
                         type = (str_attr, "optional"))

#
# Construction of elements leading to resources
#
Group_el = Element("group",
                   name = (str_attr, "required"),
                   type = (str_attr, "optional"),
                   size = (size_attr, "required"))

MemPool_el = Element("mem_pool",
                     Group_el,
                     id         = (str_attr, "required"),
                     node       = (str_attr, "required"),
                     block_size = (str_attr, "optional"))

VM_Section_el = Element("vm_section",
                        id      = (str_attr, "required"),
                        address = (size_attr, "required"),
                        size    = (size_attr, "required"))

VirtMem_el = Element("virt_mem",
                     VM_Section_el,
                     id      = (str_attr, "required"),
                     sas_id  = (str_attr, "required"))

Pools_el = Element("pools",
                   VirtMem_el,
                   MemPool_el)

Range_el = Element("range",
                   id    = (str_attr, "required"),
                   start = (size_attr, "required"),
                   size  = (size_attr, "required"))

ID_Pool_el = Element("id_pool",
                    Range_el,
                    name   = (str_attr, "required"),
                    parent = (str_attr, "optional"),
                    source = (str_attr, "required"),
                    total  = (size_attr, "required"))

IDs_el = Element("ids",
                 ID_Pool_el)

SAS_el = Element("sas",
                 id = (str_attr, "required"))

SAS_List_el = Element("sas_list",
                      SAS_el)

Segment_el = Element("segment",
                     sections = (str_attr, "required"),
                     phys     = (size_attr, "required"),
                     virt     = (size_attr, "required"),
                     filesz   = (size_attr, "required"),
                     memsz    = (size_attr, "required"))

SegmentList_el = Element("segment_list",
                         Segment_el)

Section_el = Element("section",
                     id      = (str_attr, "required"),
                     type    = (str_attr, "optional"),
                     address = (size_attr, "required"),
                     size    = (size_attr, "required"))

SectionList_el = Element("section_list",
                          Section_el)

PhysMem_el = Element("phys_mem",
                     Section_el,
                     machine = (str_attr, "required"),
                     name    = (str_attr, "required"),
                     pool    = (str_attr, "required"))

Resources_el = Element("resources",
                       PhysMem_el,
                       SectionList_el,
                       SegmentList_el,
                       SAS_List_el,
                       IDs_el,
                       Pools_el)

#
# And now put it altogether into memstats
#
Memstats_el = Element("memstats",
                       RevisionStats_el,
                       Environment_el,
                       Resources_el)
