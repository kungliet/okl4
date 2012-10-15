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
Define os_bootinfo_el.
"""
from elf.core import UnpreparedElfFile
from elf.constants import ET_EXEC
from weaver import MergeError
from weaver.ezxml import ParsedElement, Element, long_attr, bool_attr, str_attr
from weaver.segments_xml import collect_patches, collect_elf_segments, \
     attach_to_elf_flags, Segment_el, Heap_el, Patch_el, \
     start_to_value, make_pager_attr
from weaver.memobjs_xml import Stack_el
from weaver.cells.iguana.memobjs_xml import Memsection_el, \
     collect_memsection_element, create_standard_caps, create_alias_cap

from weaver.cells.iguana.bootinfo import Thread, Zone, PD, Environment, Cap
import os

Argv_el = Element("arg",
                  value = (str_attr, "required"))

CommandLine_el = Element("commandline", Argv_el)

Thread_el = Element("thread", Stack_el, CommandLine_el,
                    name     = (str_attr, "required"),
                    start    = (str_attr, "required"),
                    priority = (long_attr, "optional"),
                    physpool = (str_attr, "optional"),
                    virtpool = (str_attr, "optional"))

Entry_el = Element("entry",
                        key    = (str_attr, "required"),
                        value  = (long_attr, "optional"),
                        cap    = (str_attr, "optional"),
                        attach = (str_attr, "optional"))

Environment_el = Element("environment", Entry_el)

Zone_el = Element("zone", Segment_el, Memsection_el,
                  name     = (str_attr, "required"),
                  physpool = (str_attr, "optional"),
                  virtpool = (str_attr, "optional"))

VirtualDevice_el = Element("virt_device",
                           name = (str_attr, "required"))

Program_el = Element("program",
                     Segment_el, Patch_el, Stack_el, Heap_el,
                     CommandLine_el, Memsection_el, Thread_el,
                     Environment_el, Zone_el, VirtualDevice_el,
                     name     = (str_attr, "required"),
                     file     = (str_attr, "required"),
                     priority = (long_attr, "optional"),
                     physpool = (str_attr, "optional"),
                     virtpool = (str_attr, "optional"),
                     direct   = (bool_attr, "optional"),
                     pager    = (str_attr, "optional"),
                     server   = (str_attr, "optional"),
                     platform_control = (bool_attr, "optional"),
                     )

PD_el = Element("pd",
                Segment_el, Patch_el, Memsection_el, Thread_el,
                Environment_el, Zone_el,
                name     = (str_attr, "required"),
                file     = (str_attr, "optional"),
                physpool = (str_attr, "optional"),
                virtpool = (str_attr, "optional"),
                pager    = (str_attr, "optional"),
                direct   = (bool_attr, "optional"),
                platform_control = (bool_attr, "optional"),
                )


def collect_thread(elf, el, ignore_name, namespace, image, machine,
                   pools, space, entry, name = None,
                   namespace_thread_name = None, cell_create_thread = False):
    """Collect the attributes of a thread element."""
    if entry is None:
        raise MergeError, "No entry point specified for thread %s" % name

    # el can be a program element or a thread element.
    if name is None:
        name = el.name

    user_main = getattr(el, 'start', None)

    # HACK:  There is no easy way for a PD to specify the main thread,
    # so is the user entry point is '_start' (the traditional entry
    # point symbol), then start there rather then the thread entry
    # point.
    if user_main == "_start":
        user_main = None

    entry = start_to_value(entry, elf)
    user_main = start_to_value(user_main, elf)

    # XXX: Iguana only runs on Micro, so default prio is 100!
    priority = getattr(el, 'priority', 100)
    physpool = getattr(el, 'physpool', None)
    virtpool = getattr(el, 'virtpool', None)

    # New namespace for objects living in the thread.
    if namespace_thread_name is None:
        namespace_thread_name = name
    new_namespace = namespace.add_namespace(namespace_thread_name)

    # Push the overriding pools for the thread.
    image.push_attrs(virtual = virtpool,
                     physical = physpool)

    # Create the thread desription object.
    thread = Thread(name, entry, user_main, priority)

    utcb = image.new_attrs(new_namespace.add_namespace("utcb"))
    # Create the cell thread and assign the entry point
    cell_thread = space.register_thread(entry, user_main, utcb,
                                        priority,
                                        create = cell_create_thread)
    cell_thread.ip = entry

    # Add the standard caps for the thread.
    create_standard_caps(thread, new_namespace)

    # Collect the stack.  Is there no element, create a fake one for
    # the collection code to use.
    stack_el = el.find_child('stack')

    if stack_el is None:
        stack_el = ParsedElement('stack')

    stack_ms = collect_memsection_element(stack_el, ignore_name,
                                          new_namespace, image,
                                          machine, pools)
    thread.attach_stack(stack_ms)
    # Setup the stack for the new cell thread
    cell_thread.stack = stack_ms.get_ms().attrs

    # Collect any command line arguments.
    commandline_el = el.find_child('commandline')

    if commandline_el is not None:
        for arg_el in commandline_el.find_children("arg"):
            thread.add_argv(arg_el.value)

    image.pop_attrs()

    return thread

def collect_environment_element(env_el, namespace, machine, pools,
                                image, bootinfo):
    """Collect the object environment for a program or PD."""
    env = Environment(namespace, machine, pools, image, bootinfo) 
    extra_ms = []

    # Collect any custom entries in the environment.
    if env_el is not None:
        for entry_el in env_el.find_children('entry'):
            value       = None
            cap_name    = None
            attach      = None

            if hasattr(entry_el, 'value'):
                value = entry_el.value

                env.add_entry(entry_el.key, value = value,
                              cap_name = cap_name, attach = attach)
            else:
                if not hasattr(entry_el, 'cap'):
                    raise MergeError, 'Value or cap attribute required.'

                cap_name = entry_el.cap

                if hasattr(entry_el, 'attach'):
                    attach = attach_to_elf_flags(entry_el.attach)

                # If the cap is not an AliasCap we add it as before
                env.add_entry(entry_el.key, value = value,
                              cap_name = cap_name, attach = attach)

    return env, extra_ms

def collect_zone_element(zone_el, ignore_name, namespace, pools,
                         image, bootinfo, machine):
    """Collect the contents of a zone."""
    non_zone_ms = []

    # New namespace for objects living in the memsection.
    zone_namespace = namespace.add_namespace(zone_el.name)

    physpool = getattr(zone_el, 'physpool', None)

    attrs = image.new_attrs(zone_namespace.add_namespace(zone_el.name))

    zone = Zone(attrs, pools, image, machine)

    bootinfo.add_zone(zone)

    # Push the overriding attributes for the zone.
    image.push_attrs(virtual  = zone_el.name,
                     physical = physpool)

    master = Cap("master", ["master"])
    zone.add_cap(master)
    zone_namespace.add(master.get_name(), master)

    for ms_el in zone_el.find_children('memsection'):
        if not ignore_name.match(ms_el.name):
            ms = collect_memsection_element(ms_el, ignore_name,
                                            zone_namespace, image,
                                            machine, pools)

            # Memsections that aren't allocated in the zone belong to the PD.
            if ms.get_attrs().virtpool == zone_el.name:
                zone.add_memsection(ms)
                image.add_group(None, [ms.get_ms()])
            else:
                non_zone_ms.append(ms)

    # All of the segments have already been processed by the program/pd, but
    # record the ones probably should belong to the zone.  These will be added
    # to the zone by PD.add_zone()
    segment_els = zone_el.find_children("segment")
    zone.claimed_segments = [segment_el.name for segment_el in segment_els]

    image.pop_attrs()

    return (zone, non_zone_ms)

def collect_thread_elements(el, ignore_name, elf, namespace, image, machine,
                            pools, env, pd, space):
    threads = []
    for thread_el in el.find_children('thread'):
        if not ignore_name.match(thread_el.name):
            thread = collect_thread(elf, thread_el, ignore_name,
                                    namespace, image, machine, pools,
                                    space, entry = "_thread_start")
            pd.add_thread(thread)

            # Record the thread and its stack in the environment.
            env.add_entry(key      = thread.get_name(),
                          cap_name = thread.get_name() + '/master')
            env.add_entry(key      = thread.get_name() + "/STACK",
                          cap_name = thread.get_name() + '/stack/master',
                          attach   = thread.get_stack().get_attrs().attach)
            threads.append(thread)
    return threads

def collect_memsection_elements(el, ignore_name, namespace, image, machine,
                                pools, env, pd):
    for ms_el in el.find_children('memsection'):
        if not ignore_name.match(ms_el.name):
            ms = collect_memsection_element(ms_el, ignore_name,
                                            namespace, image,
                                            machine, pools)

            pd.attach_memsection(ms)
            image.add_group(0, [ms.get_ms()])

            env.add_entry(key      = ms.get_name(),
                          cap_name = ms.get_name() + '/master',
                          attach   = ms.get_attrs().attach)


def collect_zone_elements(el, ignore_name, namespace, image, machine, pools,
                          bootinfo, env, pd):
    for zone_el in el.find_children('zone'):
        (zone, non_zone_ms) = \
               collect_zone_element(zone_el, ignore_name,
                                    namespace, pools, image,
                                    bootinfo, machine)

        pd.attach_zone(zone)

        # Attach memsections that aren't part of the zone to the program.
        for ms in non_zone_ms:
            pd.attach_memsection(ms)
            image.add_group(0, [ms.get_ms()])
            env.add_entry(key      = ms.get_name(),
                          cap_name = ms.get_name() + '/master',
                          attach   = ms.get_attrs().attach)


def setup_pools(el, image, pd, bootinfo):

    virtpool = getattr(el, 'virtpool', None)
    physpool = getattr(el, 'physpool', None)
    direct = getattr(el, 'direct', None)
    pd.set_platform_control(getattr(el, "platform_control", False))

    if hasattr(el, 'pager'):
        pager = make_pager_attr(el.pager)
    else:
        pager = None

    # Push the overriding attributes for the program.
    image.push_attrs(virtual  = virtpool,
                     physical = physpool,
                     pager    = pager,
                     direct   = direct)

    pd.set_default_pools(image, bootinfo)


def _init_program_pd(el, image, bootinfo, machine, pools, namespace):

    # New namespace for objects living in the PD.
    pd_namespace = namespace.add_namespace(el.name)
    pd = PD(el.name, pd_namespace)
    setup_pools(el, image, pd, bootinfo)
    pd.add_callback_ms(image, pd_namespace, machine, pools)

    # Collect the object environment
    pd.add_env_ms(image, pd_namespace, machine, pools)
    env, extra_ms = collect_environment_element(el.find_child('environment'),
                                                pd_namespace, machine, pools,
                                                image, bootinfo)

    return pd, env, pd_namespace, extra_ms

def _init_elf_file(pd_el, image, pd_namespace, machine, pools, bootinfo, pd, kernel):

    elf = UnpreparedElfFile(filename=os.path.join(pd_el._path, pd_el.file))

    if elf.elf_type != ET_EXEC:
        raise MergeError, "All the merged ELF files must be of EXEC type."

    segment_els = pd_el.find_all_children("segment")
    segs = collect_elf_segments(elf, image.PROGRAM, segment_els,
                                pd_el.name, [], pd_namespace,
                                image, machine, pools)

    segs_ms = [bootinfo.record_segment_info(pd_el.name, seg,
                                            image, machine, pools)
               for seg in segs]
    for seg_ms in segs_ms:
        pd.attach_memsection(seg_ms)

    # Record any patches being made to the program.
    patch_els = pd_el.find_children("patch")
    collect_patches(elf, patch_els, os.path.join(pd_el._path, pd_el.file), image)

    # Collect threads in the PD.
    elf = pd.elf = elf.prepare(image.wordsize, image.endianess)

    return elf, segs

def collect_program_element(program_el, ignore_name, namespace, image,
                            machine, bootinfo, pools, kernel, cell):
    """Collect the attributes of a program element."""

    pd, env, prog_namespace, extra_ms = \
        _init_program_pd(program_el, image, bootinfo, machine, pools, namespace)

    space = cell.register_space(prog_namespace, program_el.name,
                                create = False)

    elf, segs = _init_elf_file(program_el, image, prog_namespace, machine,
                               pools, bootinfo, pd, kernel)

    # Collect remaining threads.
    threads = collect_thread_elements(program_el, ignore_name, elf,
                                      prog_namespace, image, machine, pools,
                                      env, pd, space)


    bootinfo.add_elf_info(name = os.path.join(program_el._path, program_el.file),
                          elf_type = image.PROGRAM,
                          entry_point = elf.entry_point)

    # Collect the main thread.
    thread = collect_thread(elf, program_el, ignore_name, prog_namespace,
                            image, machine, pools, space,
                            entry = elf.entry_point,
                            name = program_el.name,
                            namespace_thread_name = "main")

    # Record the main thread and its stack in the environment.
    env.add_entry(key      = "MAIN",
                  cap_name = 'main/master')
    env.add_entry(key      = "MAIN/STACK",
                  cap_name = 'main/stack/master',
                  attach  = thread.get_stack().get_attrs().attach)

    pd.add_thread(thread)
    pd.server_thread = thread

    # Add the virtual device elements
    # Virtual devices always get added to the global namespace because they
    # should be globally unique
    server_spawn_nvdevs = 0
    dev_ns = namespace.root.get_namespace("dev")

    if dev_ns is None:
        raise MergeError, "Device namespace does not exist!"

    for v_el in program_el.find_children('virt_device'):
        virt_dev = pd.add_virt_dev(v_el.name, program_el.name, pd,
                                   thread, server_spawn_nvdevs)
        create_alias_cap(virt_dev, dev_ns)
        server_spawn_nvdevs += 1

    # If marked, sure that the program is exported to every
    # environment so that it can be found by other programs.
    #
    if hasattr(program_el, 'server'):
        bootinfo.add_server(
            key = program_el.server,
            cap_name = prog_namespace.abs_path('main') + '/stack/master')

    # Collect any other memsections in the program.
    collect_memsection_elements(program_el, ignore_name, prog_namespace, image,
                                machine, pools, env, pd)

    # Collect any zones in the program.
    collect_zone_elements(program_el, ignore_name, prog_namespace, image,
                          machine, pools, bootinfo, env, pd)

    # Collect the heap.  Is there no element, create a fake one for
    # the collection code to use.
    heap_el = program_el.find_child('heap')

    if heap_el is None:
        heap_el = ParsedElement('heap')

    heap_ms = collect_memsection_element(heap_el, ignore_name, prog_namespace,
                                         image, machine, pools)
    pd.attach_heap(heap_ms)

    threads.append(thread)
    image.add_group(None, [segs[0], pd.env_ms.get_ms(), pd.callback.get_ms(),
                           heap_ms.get_ms()] +
                    [thread.get_stack().get_ms() for thread in threads] +
                    [ms.get_ms() for ms in extra_ms])
    # Fill env with default values.
    env.add_entry(key      = "HEAP",
                  cap_name = 'heap/master',
                  attach   = heap_ms.get_attrs().attach)

    env.add_entry(key      = "HEAP_BASE",
                  base     = heap_ms)
    env.add_entry(key      = "HEAP_SIZE",
                  value    = heap_ms.get_attrs().size)

    pd.add_environment(env)

    bootinfo.add_pd(pd)

    image.pop_attrs()


def collect_pd_element(pd_el, ignore_name, namespace, image, machine,
                       bootinfo, pools, cell):
    """Collect the attributes of a PD element."""

    pd, env, pd_namespace, extra_ms = \
        _init_program_pd(pd_el, image, bootinfo, machine, pools, namespace)

    space = cell.register_space(pd_namespace, pd_el.name, create = False)

    threads, segs = [], []
    if hasattr(pd_el, "file"):
        elf, segs = _init_elf_file(pd_el, image, pd_namespace, machine, pools,
                                   bootinfo, pd)

        threads = collect_thread_elements(pd_el, ignore_name, elf, pd_namespace,
                                          image, machine, pools, env, pd, space)

    image.add_group(None, segs[:1] +
                    [pd.env_ms.get_ms(), pd.callback.get_ms()] +
                    [thread.get_stack().get_ms() for thread in threads] +
                    [ms.get_ms() for ms in extra_ms])

    # Collect memsections in the PD.
    collect_memsection_elements(pd_el, ignore_name, pd_namespace, image,
                                machine, pools, env, pd)

    # Collect any zones in the program.
    collect_zone_elements(pd_el, ignore_name, pd_namespace, image, machine,
                          pools, bootinfo, env, pd)

    pd.add_environment(env)

    bootinfo.add_pd(pd)
    image.pop_attrs()

def collect_program_pd_elements(parsed, ignore_name, namespace, image,
                                machine, bootinfo, pools, kernel, cell):
    """Go through and handle all the iguana program objects that we have."""

    for el in parsed.children:
        if el.tag == "program":
            if not ignore_name.match(el.name):
                collect_program_element(el, ignore_name, namespace,
                                        image, machine, bootinfo, pools, kernel,
                                        cell)
        elif el.tag == "pd":
            if not ignore_name.match(el.name):
                collect_pd_element(el, ignore_name, namespace, image,
                                   machine, bootinfo, pools, cell)

