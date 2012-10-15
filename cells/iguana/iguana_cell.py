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
Build system classes for the iguana cell.
"""

import os
import SCons
from kenge import OKL4Cell

class IguanaCell(OKL4Cell):
    def __init__(self, name, dom, SYSTEM, CUST, MACHINE, TOOLCHAIN,
                 BUILD_DIR, xml_tag = None, **kargs):
        OKL4Cell.__init__(self, name, dom, SYSTEM, CUST, MACHINE,
                          TOOLCHAIN, BUILD_DIR,
                          xml_tag = "iguana", **kargs)

        self.vdev_clients = {}
        self.spawn_vdevs = {}
        self.vdev_counter = {}
        self.devices = []
        self.device_clients = []
        

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


    def Thread(self, name, entry = None, priority = None,
               args = None, stack = None, physpool = None):
        """
        Create a description of a program's thread.
        """
        elem = self.dom.createElement("thread")

        elem.setAttribute('name', name)

        if entry is not None:
            elem.setAttribute('start', str(entry))

        if priority is not None:
            elem.setAttribute('priority', str(priority))

        if physpool is not None:
            elem.setAttribute('physpool', physpool)

        if stack is not None:
            if not isinstance(stack,  SCons.Node.Node):
                stack = self.Stack(size = stack)

            elem.appendChild(stack.attributes.xml)
            self.apps.append(stack)

        if args is not None:
            if not isinstance(args,  SCons.Node.Node):
                args = self.CommandLine(args=args)

            elem.appendChild(args.attributes.xml)
            self.apps.append(args)


        seg = self.scons_env.Value(elem.toxml())
        self.set_scons_attr(seg, 'xml', elem)

        return seg

    def add_program(self, node, name = None, priority = None,
                    args = None, stack = None,
                    heap = None, patches = None, extra_threads = None,
                    segments = None, addressing = None,
                    platform_control = False,
                    env_consts=None):
        """
        Add an iguana program to the cell.

        The node is the return value of KengeProgram.
        """
        if addressing is None:
            addressing = self.Addressing(virt_addr = self.ADDR_ALLOC,
                                         align = self.ALIGN_PREFERRED)

        elem = self.dom.createElement('program')

        self.program_like(node, elem, addressing, stack, heap,
                          patches, segments, priority)
        
        if name is None:
            name = os.path.basename(node[0].abspath)

        elem.setAttribute('name', name)

        if platform_control:
            elem.setAttribute('platform_control', xmlbool(platform_control))

        if args is not None:
            if not isinstance(args,  SCons.Node.Node):
                args = self.CommandLine(args=args)

            elem.appendChild(args.attributes.xml)
            self.apps.append(args)

        if extra_threads is not None:
            for thread in extra_threads:
                elem.appendChild(thread.attributes.xml)
                self.apps.append(thread)

        prog = self.scons_env.Value(elem.toxml())
        self.set_scons_attr(prog, 'xml', elem)
        self.set_scons_attr(node, 'xml', elem)
        self.set_scons_attr(node, 'env', None)
        self.apps.append(node)

        self.elem.appendChild(elem)

        if env_consts is not None:
            for (key, value) in env_consts:
                self.env_append(node, key, value=value)

        return elem


    def add_server(self, node, server_name, name = None, priority = None,
                   args = None, stack = None, heap = None,
                   patches = None, extra_threads = None,
                   segments = None, addressing = None,
                   heap_user_map = None,
                   platform_control = False, env_consts = None):
        """
        Add an iguana server to the cell.  A server is similar to a
        program, but also contains a server name, which is added to
        the environment of every program.
        """
        elem = self.add_program(node, name, priority, args, stack,
                                heap, patches, extra_threads,
                                segments, addressing,
                                platform_control, env_consts)

        elem.setAttribute('server', server_name)

        return elem

    def add_file(self, prog, node, name = None, addressing = None):
        """
        Add a memsection, whose contents comes from a file, to a
        program or a zone.
        """
        if addressing is None:
            addressing = self.Addressing(align = self.ALIGN_PREFERRED)

        if name is None:
            name = os.path.basename(node[0].abspath)

        elem = self.dom.createElement("memsection")

        elem.setAttribute('name', name)
        elem.setAttribute('file', node[0].abspath)

        addressing.set_attrs(elem)

        prog[0].attributes.xml.appendChild(elem)

        ms = self.scons_env.Value(elem.toxml())
        self.set_scons_attr(ms, 'xml', elem)

        self.apps.append(node)

        return ms

    def add_memsection(self, prog, name, size, addressing = None):
        """
        Add a memsection to a program or a zone.
        """
        if addressing is None:
            addressing = self.Addressing()

        elem = self.dom.createElement("memsection")

        elem.setAttribute('name', name)
        elem.setAttribute('size', "%#x" % size)

        addressing.set_attrs(elem)

        if isinstance(prog, SCons.Node.NodeList):
            prog = prog[0]
        prog.attributes.xml.appendChild(elem)

        ms = self.scons_env.Value(elem.toxml())
        self.set_scons_attr(ms, 'xml', elem)

        return ms

    def add_zone(self, prog, name, virtpool = None, segments = None):
        """
        Create a zone within a program.
        """
        elem = self.dom.createElement("zone")

        elem.setAttribute('name', name)

        if virtpool is not None:
            elem.setAttribute('virtpool', virtpool)

        if segments is not None:
            for s in segments:
                elem.appendChild(s.attributes.xml)
                self.apps.append(s)

        prog[0].attributes.xml.appendChild(elem)

        zone = self.scons_env.Value(elem.toxml())
        self.set_scons_attr(zone, 'xml', elem)

        return zone

    def env_append(self, prog, key, attach = None,
                   cap=None, value = None):
        """
        Add an entry to the program's environment.
        """
        if isinstance(prog, SCons.Node.NodeList):
            prog = prog[0]
        
        if prog.attributes.env is None:
            env = self.dom.createElement('environment')
            prog.attributes.env = env
            prog.attributes.xml.appendChild(env)
        else:
            env = prog.attributes.env

        elem = self.dom.createElement('entry')
        elem.setAttribute('key', key)

        if cap is not None:
            elem.setAttribute('cap', cap)
        elif value is not None:
            elem.setAttribute('value', str(value))

        if attach is not None:
            elem.setAttribute('attach', attach)

        env.appendChild(elem)

        val = self.scons_env.Value(elem.toxml())
        self.set_scons_attr(val, 'xml', elem)

        return val

    def register_device(self, prog, raw_device, virtual_device):
        """
        Register an iguana device server.
        """
        res = "_".join([raw_device, "resource"])
        dev = "/dev/" + "_".join([raw_device, "dev"])
        res = res.upper()

        self.env_append(prog, key = res, cap = dev)

        self.devices.append((virtual_device, prog))

    def require_devices(self, prog, devices):
        """
        Register the list of devices required by the program.
        """
        self.set_scons_attr(prog, 'required_vdevs', devices)

        self.device_clients.append(prog)

        for vdev in devices:
            if not self.spawn_vdevs.has_key(vdev):
                self.spawn_vdevs[vdev] = 1
            else:
                self.spawn_vdevs[vdev] += 1

            self.vdev_counter[vdev] = 0


    def layout_devices(self):
        """
        Establish the connections between programs and the devices
        they need.
        """
        for (dev, prog) in self.devices:
            if self.spawn_vdevs.has_key(dev):
                for i in range(self.spawn_vdevs[dev]):
                    elem = self.dom.createElement('virt_device')
                    elem.setAttribute('name', "%s%d" % (dev, i))

                    prog[0].attributes.xml.appendChild(elem)

        for prog in self.device_clients:
            for vdev in prog[0].attributes.required_vdevs:
                self.env_append(prog, key=vdev.upper(),
                                cap = "/dev/%s%d" % (vdev, self.vdev_counter[vdev]))                
                self.vdev_counter[vdev] += 1

    def vdevs_setup(self, obj, vserial_obj, vserial_ms_size=0x1000):
        """
        Set up per-client resources for virtual device servers.
        """
        name = obj[0].attributes.xml.getAttribute('name')

        for vdev in getattr(obj[0].attributes, 'required_vdevs', []):
            if (vdev == "vserial"):
                # Round up to the nearest page size
                remainder = vserial_ms_size % 0x1000
                if (remainder):
                    vserial_ms_size -= remainder
                    vserial_ms_size += 0x1000

                # Create vserial memsection and add to client space
                self.add_memsection(obj, name="VSERIAL_MS",
                                    size=vserial_ms_size)

                # Increment vdev's client count
                if ("vserial" not in self.vdev_clients):
                    self.vdev_clients["vserial"] = 0
                self.vdev_clients["vserial"] += 1

                # Make memsection available to vserial server
                client_num = self.vdev_clients["vserial"]
                self.env_append(vserial_obj,
                                key = 'CLIENT_MS%d' % (client_num-1),
                                attach="rw",
                                cap="../"+name+"/VSERIAL_MS/rw")
