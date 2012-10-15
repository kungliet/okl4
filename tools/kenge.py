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

import os
import copy
import sets
import time
import pexpect
from stat import ST_SIZE
from cStringIO import StringIO
from math import ceil, log
import commands
from xml.dom.minidom import getDOMImplementation, DocumentType

from elf.util import align_up
from elf.core import PreparedElfFile
from elf.constants import PF_W, PT_LOAD

import machines
import toolchains
from opts import get_int_arg, get_bool_arg, get_arg, get_option_arg
from util import markup, xmlbool

from SCons.Util import flatten
import tarfile
import re

# Make it easier to raise UserErrors
import SCons.Errors
UserError = SCons.Errors.UserError


def get_toolchain(name, machine):
    """
    Return a toolchain object and its name.

    If name is not None, then it must be the name of a valid toolchain.
    If name is None, the default toolchain for the machine is used (assuming it exists).
    """
    if name is not None:
        available_toolchains = toolchains.get_all_toolchains()
        if name not in available_toolchains:
            raise UserError, ("'%s' is not a valid toolchain. Must be one of: %s" % (name, available_toolchains.keys()))
        toolchain = available_toolchains[name]
    else:
        try:
            toolchain = machine.default_toolchain
            name = "default"
        except AttributeError:
            raise UserError, "No TOOLCHAIN specified. You did not specify " + \
                  "a toolchain, and the machine you chose \n\tdoes not " + \
                  "have a default one. Must be one of: %s" % available_toolchains.keys()
    return toolchain, name

class KengeBuild:
    """Top level class in the hierarchy of instantiated classes. This
    represents the top of the build."""
    def __init__(self, project = None, **kargs):
        # Any default arguments from the SConstruct are overridden by the user
        kargs.update(conf.dict)

        def handle_arg(name, default):
            ret = kargs.get(name, default)
            if name in kargs: del kargs[name]
            return ret

        self.build_dir = handle_arg("BUILD_DIR", "#build")
        if not self.build_dir.startswith('#'):
            self.build_dir = '#' + self.build_dir

        # Here we setup the sconsign file. This is the database in which
        # SCons holds file signatures. (See man scons for more information)
        if not os.path.exists(Dir(self.build_dir).abspath):
            os.mkdir(Dir(self.build_dir).abspath)
        SConsignFile(Dir(self.build_dir).abspath + os.sep + "sconsign")

        available_machines = machines.get_all_machines()
        sorted_avail_machines = available_machines.keys()
        sorted_avail_machines.sort()

        machine = handle_arg("MACHINE", None)
        if machine is None:
            raise UserError, "You must specify the MACHINE you " + \
                  "wish to build for.  Must be one of: %s" % sorted_avail_machines
        if machine not in available_machines:
            raise UserError, ("'%s' is not a valid machine. Must be one of: %s" % \
                              (machine, sorted_avail_machines))
        self.machine = available_machines[machine]
        self.machine_name = machine
        self.project = project

        toolchain = handle_arg("TOOLCHAIN", None)
        if toolchain is not None:
            self.toolchain_name = toolchain
        else:
            self.toolchain_name = "gnu_arm_eabi_toolchain"
        self.toolchain, toolchain = get_toolchain(toolchain, self.machine)

        self.image_type = handle_arg("IMAGE_TYPE", "generic")

        # Any remaining options are saved, as they may be passed down to
        # KengeEnvironment's created.
        self.args = kargs

        # Work out which kernel we want to build:
        # First check the commandline
        self.kernel = handle_arg("KERNEL", None)
        if self.kernel:
            self.args['kernel'] = self.kernel
        # If not on the cmd line, use project's SConstruct kargs
        elif self.args.has_key('kernel'):
            self.kernel = self.args['kernel']
        else:
            # Failing that, just set it to micro
            self.kernel = 'micro'
            self.args['kernel'] = 'micro'

        if self.args.has_key('compatible_with'):
            self.compatible_with = self.args['compatible_with']
        else:
            self.compatible_with = ['micro', 'nano']
        if self.kernel not in self.compatible_with:
            raise UserError, "The specified kernel (%s) is not compatible " \
                    "with this project. Must be one of: %s" % \
                    (self.kernel, self.compatible_with)

        # Initialise the expected test output list.
        self.expect_test_data = []

        doc_type = DocumentType("image")
        doc_type.systemId = "weaver-2.0.dtd"
        self.dom = getDOMImplementation().createDocument(None, "image", doc_type)
        self.populate_machine(self.dom)

        if get_bool_arg(self, "PYFREEZE", True):
            self.elfweaver = None
            self.elfadorn = None
            weaver_env = self.KengeEnvironment('elfweaver')
            self.elfweaver, self.elfadorn = \
                            weaver_env.Package("tools/pyelf", duplicate=1)
        else:
            self.elfweaver = File("#tools/pyelf/elfweaver.py")
            self.elfadorn = File("#tools/pyelf/elfadorn.py")

    def populate_machine(self, dom):
        machine = dom.createElement("machine")
        dom.documentElement.appendChild(machine)

        # CPU:
        elem = dom.createElement('cpu')
        elem.setAttribute('name', self.machine.cpu)
        machine.appendChild(elem)

        # Word size:
        elem = dom.createElement('word_size')
        elem.setAttribute('size', "%#x" % self.machine.wordsize)
        machine.appendChild(elem)

        if len(self.machine.memory['virtual']) > 0:
            virt = dom.createElement('virtual_memory')
            virt.setAttribute('name', 'virtual')
            machine.appendChild(virt)

            for region in self.machine.memory['virtual']:
                elem = dom.createElement('region')
                elem.setAttribute('base', '%#x' % region.base)
                elem.setAttribute('size', '%#x' % region.size)
                virt.appendChild(elem)

        for mem in self.machine.memory.keys():
            if mem in ['virtual'] or len(self.machine.memory[mem]) is 0:
                continue

            phys = dom.createElement('physical_memory')
            phys.setAttribute('name', mem)
            machine.appendChild(phys)

            for region in self.machine.memory[mem]:
                elem = dom.createElement('region')
                elem.setAttribute('base', '%#x' % region.base)
                elem.setAttribute('size', '%#x' % region.size)
                elem.setAttribute('rights', region.rights)

                if region.cache_policy is not None:
                    elem.setAttribute('cache_policy', region.cache_policy)

                phys.appendChild(elem)

        if hasattr(self.machine, 'v2_drivers'):
            for (driver, vdev, mems, irqs) in self.machine.v2_drivers:
                phys_dev = dom.createElement('phys_device')
                phys_dev.setAttribute('name', "%s_dev" % vdev[1:]) # strip the first char which should be 'v'
                machine.appendChild(phys_dev)

                for i, region in enumerate(mems):
                    phys = dom.createElement('physical_memory')
                    phys.setAttribute('name', "%s_mem%d" % (vdev[1:], i))
                    phys_dev.appendChild(phys)

                    elem = dom.createElement('region')
                    elem.setAttribute('base', '%#x' % region.base)
                    elem.setAttribute('size', '%#x' % region.size)
                    elem.setAttribute('rights', region.rights)

                    if region.cache_policy is not None:
                        elem.setAttribute('cache_policy', region.cache_policy)

                    phys.appendChild(elem)

                for i, irq in enumerate(irqs):
                    elem = dom.createElement('interrupt')
                    elem.setAttribute('name', 'int_%s%d' % (vdev[1:], i))
                    elem.setAttribute('number', '%d' % irq)
                    phys_dev.appendChild(elem)

        for size in self.machine.page_sizes:
            elem = dom.createElement('page_size')
            elem.setAttribute('size', "%#x" % size)
            machine.appendChild(elem)


    def KengeEnvironment(self, name, cell = None, *args, **kargs):
        """Create a new environment in this build.
        Name is the name of the environment to create.
        *args are passed on to the KengeEnvironment class.
        **kargs are any options configuration options."""
        # Create a copy of our extra args, and then update first
        # with any overriders from the kargs, and then from the user's
        # conf
        new_args = copy.copy(self.args)
        new_args.update(kargs)

        # If the user has specified any <name>.<foo> options pass
        # them through.
        if hasattr(conf, name):
            new_args.update(getattr(conf, name).dict)

        # Finally update these required fields, with sane defaults,
        # only if they haven't already been overridden.
        new_args["SYSTEM"] = new_args.get("SYSTEM", name)
        new_args["CUST"] = new_args.get("CUST", self.machine.cust)
        new_args["MACHINE"] = new_args.get("MACHINE", self.machine)
        new_args["TOOLCHAIN"] = new_args.get("TOOLCHAIN", self.toolchain)
        new_args["BUILD_DIR"] = new_args.get("BUILD_DIR", self.build_dir +
                                             os.sep + name)
        new_args["is_rootserver"] = new_args.get("is_rootserver", False)
        new_args["ELFWEAVER"] = self.elfweaver
        new_args["ELFADORN"] = self.elfadorn

        return KengeEnvironment(name, cell, *args, **new_args)

    def KengeCell(self, cell_cls, name, *args, **kargs):
        """Create a cell environment in this build.
        cell_cls is the class of the cell to create.  The build name
        is the name of the cell, but can be overridden by buildname.
        *args are passed on to the KengeEnvironment class.
        **kargs are any options configuration options."""
        # Create a copy of our extra args, and then update first
        # with any overriders from the kargs, and then from the user's
        # conf
        new_args = copy.copy(self.args)
        new_args.update(kargs)

        # If the user has specified any <name>.<foo> options pass
        # them through.
        if hasattr(conf, name):
            new_args.update(getattr(conf, name).dict)

        # Finally update these required fields, with sane defaults,
        # only if they haven't already been overridden.
        new_args["SYSTEM"] = new_args.get("SYSTEM", name)
        new_args["CUST"] = new_args.get("CUST", self.machine.cust)
        new_args["MACHINE"] = new_args.get("MACHINE", self.machine)
        new_args["TOOLCHAIN"] = new_args.get("TOOLCHAIN", self.toolchain)
        new_args["BUILD_DIR"] = new_args.get("BUILD_DIR", self.build_dir +
                                             os.sep + name)
        new_args["is_rootserver"] = new_args.get("is_rootserver", True)
        new_args["ELFWEAVER"] = self.elfweaver
        new_args["ELFADORN"] = self.elfadorn

        return cell_cls(name, self.dom, *args, **new_args)

    def Cell(self, cell, **kargs):
        # Call the apps SConstruct file to build it
        paths = (
            os.path.join('cust', self.args.get("CUST", self.machine.cust), cell),
            cell,
        )
        build = self
        args  = kargs
        for path in paths:
            sconscript_path = os.path.join(path, 'SConscript')
            if os.path.exists(sconscript_path):
                return SConscript(sconscript_path,
                                  exports = ['build', 'args'], duplicate=0)

        raise UserError("Unable to find cell: %s" % cell)

    def Kernel(self, **kargs):
        return self.Cell("cells/%skernel" % self.kernel, **kargs)

    def Image(self, kernel, pools, cells):
        return self.Cell("cells/image", kernel = kernel,
                         pools = pools, cells = cells)

    def get_run_method(self):
        if len(self.machine.run_methods.keys()) > 1:
            key = conf.dict.get('RUN', self.machine.default_method)
        else:
            key = self.machine.run_methods.keys()[0]
        return self.machine.run_methods[key]

    def RunImage(self, run_image):
        def simulate(target, source, env):
            run_method = self.get_run_method()
#            if self.project is not None:
#                if (self.project == "ktest" or self.project == "l4test") and self.machine_name == "versatile_uboot":
#                    print "using high baud rate"
#                    cmdline = run_method(target, source, self.machine,
#                                        self.args.get("XIP", False), "-a")
#                else:
#                print "using normal baud rate"
#                cmdline = run_method(target, source, self.machine,
#                                        self.args.get("XIP", False))
#            else:
#                print "using normal baud rate"
            cmdline = run_method(target, source, self.machine,
                                 get_bool_arg(self, "XIP", False))
            os.system(cmdline)

        Command("#simulate", run_image,
                Action(simulate, "[SIM ] $SOURCE"))

    def SetCoveragePackages(self, project):
        if project == "iguana":
            self.packages_to_assess = "iguana/server"
            self.test_libs = conf.dict.get("TEST_LIBS", "")
            if "all" in self.test_libs:
                if not os.path.exists("libs"):
                    libs_dir = os.path.join('..', '..', 'libs')
                else:
                    libs_dir = "libs"
                for ldir in os.listdir(libs_dir):
                    self.packages_to_assess += "," + os.path.join("libs", ldir)
            else:
                self.packages_to_assess += "," + re.sub(r'(^|,){1}', r'\1libs/', self.test_libs)
        elif project == "ktest":
            self.packages_to_assess = "pistachio,ktest"
        elif project == "ctest":
            self.packages_to_assess = "libs/okl4,ctest"
        elif project == "ntest":
            self.packages_to_assess = "nanokernel"
        else:
            raise UserError, "Do not know how to make target \"coverage\" for project %s" % project

    def test(self, target, source, env):
        def print_log(log):
            print '--- SIMULATOR LOG ' + '-'*60
            print log.getvalue()
            print '-'*78

        def print_coverage(cov_stdout, tracelog_file):
            if str(target[0]) == "coverage":
                for line in cov_stdout:
                    if str(line) == "</summary>\n": print line
                    else: print line,
                os.unlink(tracelog_file)

        # If coverage target is specified, run test code coverage script in
        # parallel with skyeye
        cov_stdout = ""
        tracelog_file = ""
        if str(target[0]) == "coverage":
            run_arg = conf.dict.get('RUN', self.machine.default_method)
            if run_arg != "skyeye":
                raise UserError, "Can not run target \"coverage\" with " + \
                                 "the specified run method \"%s\"." % run_arg + \
                                 "\nThe target \"coverage\" currently supports " + \
                                 "only skyeye simulator. Stop."
            try:
                self.machine.skyeye += ".tracelog"
            except:
                raise UserError, "The specified machine does not support " + \
                                 "target \"coverage\".\nThe target \"coverage\" " + \
                                 "currently supports only skyeye simulator. Stop."
            f = open("tools/sim_config/" + self.machine.skyeye, "r")
            tracelog_content = f.read()
            m = re.search('logfile=(?P<logfile>[^\s(,\s)]*)', tracelog_content)
            f.close()
            if m == None:
                raise Error, "Can not find \"logon\" option in Skyeye configuration file."
            tracelog_file = m.group('logfile')
            if os.path.exists(tracelog_file):
                os.unlink(tracelog_file)
            os.mkfifo(tracelog_file)
            cov_stdout = os.popen("python tools/coverage.py -p %s -b %s -t %s -s" % (self.packages_to_assess, Dir(self.build_dir), tracelog_file), 'r')

        # Get the testing value
        timeout = float(conf.TESTING.dict.get("TIMEOUT", 30.0))
        run_method = self.get_run_method()
#        if self.project is not None:
#            if (self.project == "ktest" or self.project == "l4test") and self.machine_name == "versatile_uboot":
#                print "using high baud rate"
#                cmdline = run_method(target, source, self.machine,
#                                     self.args.get("XIP", False), "-a")
#            else:
#                cmdline = run_method(target, source, self.machine,
#                                     self.args.get('XIP', False))
#        else:
        cmdline = run_method(target, source, self.machine,
                             get_bool_arg(self, "XIP", False))

        if len(self.expect_test_data) == 0:
            raise UserError, "No expected test output supplied " + \
                    "for this build"
        log = StringIO()
        if pexpect.__version__ >= '2.0':
            tester = pexpect.spawn(cmdline, timeout=timeout, logfile=log)
        else:
            tester = pexpect.spawn(cmdline, timeout=timeout)
            tester.setlog(log)
        for in_, out_ in self.expect_test_data:
            try:
                i = tester.expect(in_)
            except pexpect.ExceptionPexpect, e:
                if isinstance(e, pexpect.TIMEOUT):
                    print "Timed out waiting for: <%s>" % in_
                elif isinstance(e, pexpect.EOF):
                    print "Simulator exited while waiting for: <%s>" % in_

                print_log(log)
                tester.terminate(True)
                print_coverage(cov_stdout, tracelog_file)
                raise UserError, "Failed test."
            if i >= 1:
                print_log(log)
                tester.terminate(True)
                print_coverage(cov_stdout, tracelog_file)
                raise UserError, "Failed test, matched <%s>" % in_[i]
            if not out_ is None:
                for char in out_:
                    time.sleep(0.2)
                    tester.send(char)
                tester.send('\n')
                #tester.sendline(out_)
        tester.terminate(True)
        if conf.TESTING.dict.get("PRINT_LOG", False):
            print_log(log)
        print_coverage(cov_stdout, tracelog_file)

    def TestImage(self, test_image):
        Command("#simulate_test", test_image,
                Action(self.test, "[TEST] $SOURCE"))

    def CoverageImage(self, cov_image):
        Command("#coverage", cov_image,
                Action(self.test, "[COV ] $SOURCE"))

    # Layout Macho files by doing a relocation link
    def MachoLayoutVirtual( self, apps ):

        kernel = apps[0]
        apps = apps[1:]
        reloc_apps = []

        # default base address
        base_addr = 0x00d00000

        # Create a relocation for each app
        for app in apps:
            rtarget = Flatten( app )[0].abspath + ".reloc"
            print "rtarget is %s" % rtarget
            rcmd = "ld -image_base %s -o $TARGET $SOURCE" % hex(base_addr)
            print "rcmd is '%s'" % rcmd
            rapp =  self.Command( rtarget,
                             app,
                             rcmd )

            reloc_apps += rapp

            # XXX: could do this better, but too lazy right now
            base_addr += 0x100000

        return kernel + reloc_apps

    # Mach-O binaries are bundled with the 'legion' utility
    def CreateLegionImage(self, apps):

        # actual command to execute
        modules = ""
        for mod in Flatten(apps[1:]):
            modules += " -m %s " % str(mod.abspath)

        # XXX: why the hell won't this look in the PATH?
        legion_cmd = "tools/legion/legion -k %s %s -o $TARGET" % (Flatten(apps)[0].abspath, modules)

        print "apps: %s" % apps
        print "legion_cmd is '%s'" % legion_cmd

        # command object
        boot_image = self.Command( os.path.join(self.build_dir, "mach_kernel"),
                              apps,
                              Action( legion_cmd , "[LEGN] $TARGET") )

        return boot_image

def env_inner_class(cls):
    """
    Grant a class access to the attributes of a KengeEnvironment
    instance.  This is similar to Java inner classes.

    The weaver objects need access to the machine properties, which
    are stored in KengeEnvironment.machine.  Rather then requiring the
    env to be passed to every constructor,  this function converts
    them into inner classes, so that env.Foo() will create an instance
    of Foo that contains a __machine__ class attribute.
    """
    # Sanity Check
    if hasattr(cls, '__machine__'):
        raise TypeError('Existing attribute "__machine__" '
                        'in inner class')

    class EnvDescriptor(object):
        def __get__(self, env, envclass):
            if env is None:
                raise AttributeError('An enclosing instance for %s '
                                     'is required' % (cls.__name__))
            classdict = cls.__dict__.copy()
            # Add the env's machine attr to the class env.
            classdict['__machine__'] = property(lambda s: env.machine)

            return type(cls.__name__, cls.__bases__, classdict)

    return EnvDescriptor()

class KengeEnvironment:
    """The KengeEnvironment creates the environment in which to build
    applications and libraries. he main idea is that everything in an
    environment shares the same toolchain and set of libraries.
    """
    c_ext = ["c"]
    cxx_ext = ["cc", "cpp", "c++"]
    asm_ext = ["s"]
    cpp_asm_ext = ["spp"]
    src_exts = c_ext + cxx_ext + asm_ext + cpp_asm_ext

    def __init__(self, name, cell, SYSTEM, CUST, MACHINE, TOOLCHAIN,
                 BUILD_DIR, ELFWEAVER, ELFADORN, **kargs):
        """Create a new environment called name for a given SYSTEM, CUST,
        MACHINE and TOOLCHAIN. It will be built into the specified BUILD_DIR.
        Any extra kargs are saved as extra options to pass on to Packages."""

        self.scons_env = Environment() # The underlying SCons environment.
        self.name = name
        self.cell = cell
        self.system = SYSTEM
        self.cust = CUST
        self.machine = MACHINE
        self.toolchain = copy.deepcopy(TOOLCHAIN)
        self.builddir = BUILD_DIR
        self.elfweaver = ELFWEAVER
        self.elfadorn = ELFADORN
        self.args = kargs
        self.test_libs = []
        self.oklinux_dir = 'linux'

        self.scons_env['ELFWEAVER'] = self.elfweaver
        self.scons_env['ELFADORN']  = self.elfadorn

        if kargs.has_key('OKLINUX_DIR'):
            self.oklinux_dir = kargs['OKLINUX_DIR']
            del kargs['OKLINUX_DIR']

        self.kernel = kargs['kernel']

        def _installString(target, source, env):
            if env.get("VERBOSE_STR", False):
                return "cp %s %s" % (source, target)
            else:
                return "[INST] %s" % target
        self.scons_env["INSTALLSTR"] = _installString

        # Setup toolchain.

        if type(self.toolchain) == type(""):
            self.toolchain, _ = get_toolchain(self.toolchain, self.machine)
        for key in self.toolchain.dict.keys():
            self.scons_env[key] = copy.copy(self.toolchain.dict[key])

        def _libgcc(env):
            """Used by some toolchains to find libgcc."""
            return os.popen(env.subst("$CC $CCFLAGS --print-libgcc-file-name")). \
                   readline()[:-1]
        self.scons_env["_libgcc"] = _libgcc

        def _libgcc_eh(env):
            """Used by some toolchains to find libgcc_eh."""
            return os.popen(env.subst("$CC $CCFLAGS --print-libgcc-file-name")). \
                   readline()[:-1].replace("libgcc", "libgcc_eh")
        self.scons_env["_libgcc_eh"] = _libgcc_eh

        def _machine(arg):
            """Return a machine attribute. Used by the toolchains to get
            compiler/machine specific flags."""
            return getattr(self.machine, arg)
        self.scons_env["_machine"] = _machine

        self.headers = []
        self.libs = []

        # Pass through the user's environment variables to the build
        self.scons_env['ENV'] = copy.copy(os.environ)

        # Setup include path
        self.scons_env.Append(CPPPATH = self.builddir  + os.sep + "include")
        self.scons_env["LIBPATH"] = [self.builddir  + os.sep + "lib"]

        # Setup machine defines. Don't really like this. Maybe config.h
        self.scons_env.Append(CPPDEFINES =
                              ["ARCH_%s" % self.machine.arch.upper()])
        if hasattr(self.machine, "arch_version"):
            self.scons_env.Append(CPPDEFINES =
                    [("ARCH_VER", self.machine.arch_version)])
        # FIXME: deal with variant machine names (philipo)
        name = self.machine.__name__.upper()
        if name.endswith('_HW'):
            name = name[:-3]
        self.scons_env.Append(CPPDEFINES = ["MACHINE_%s" % name])
        self.scons_env.Append(CPPDEFINES =
                              ["ENDIAN_%s" % self.machine.endian.upper()])
        self.scons_env.Append(CPPDEFINES =
                              ["WORDSIZE_%s" % self.machine.wordsize])
        self.scons_env.Append(CPPDEFINES = self.machine.cpp_defines)
        self.scons_env.Append(CPPDEFINES = self.machine.cpp_defines)
        if self.machine.smp:
            self.scons_env.Append(CPPDEFINES = ["MACHINE_SMP"])

        self.scons_env.Append(CPPDEFINES =
                              ["KENGE_%s" % self.name.upper()])

        # The soc-sdk puts things in various directories other than pistachio
        # so we hack in a KENGE_(everything up to an _) so that KENGE_PISTACHIO
        # is defined for various kernel internals that need to know
        self.scons_env.Append(CPPDEFINES =
                              ["KENGE_%s" % self.name.upper().split("_")[0]])

        # Driver code still relies on having this in the #defines or it will
        # assume it should be using iguana drivers
        if kargs.has_key('is_rootserver'):
            self.scons_env.Append(CPPDEFINES = "KENGE_L4_ROOTSERVER")

        # These should use the Scons SubstitutionEnvironment functions !!
        def _linkaddress(prefix, linkfile):
            addr = eval(open(linkfile.abspath).read().strip())
            return "%s%x" % (prefix, addr)
        self.scons_env["_linkaddress"] = _linkaddress

        def _findlib(inp):
            ret = [self.scons_env.FindFile(
                self.scons_env.subst("${LIBPREFIX}%s${LIBSUFFIX}" % x),
                self.scons_env.subst("$LIBPATH"))
                   for x in inp]
            return ret
        self.scons_env["_findlib"] = _findlib

        def _as_string(inp):
            """Convert a list of things, to a list of strings. Used by
            for example the LINKSCRIPTS markup to convert a list of
            files into a list of strings."""
            return map(str, inp)
        self.scons_env["_as_string"] = _as_string

        def _platform(type, flags):
            """Return the compile flag for the requested compiler"""
            if type == "gnu":
                if flags == "c_flags":
                    return self.machine.c_flags
                # do we want separate cxx_flags?
                if flags == "cxx_flags":
                    return self.machine.c_flags
                if flags == "as_flags":
                    return self.machine.as_flags
                if flags == "link_flags":
                    return self.machine.link_flags
            if type == "rvct":
                if flags == "c_flags":
                    return self.machine.rvct_c_flags
                # do we want separate cxx_flags?
                if flags == "cxx_flags":
                    return self.machine.rvct_c_flags
                if flags == "as_flags":
                    return self.machine.rvct_c_flags
            if type == "ads":
                if flags == "c_flags":
                    return self.machine.ads_c_flags
                # do we want separate cxx_flags?
                if flags == "cxx_flags":
                    return self.machine.ads_c_flags
                if flags == "as_flags":
                    return self.machine.ads_c_flags

            return ""
        self.scons_env["_platform"] = _platform

        # Record revision control information.  Assume that it is from
        # mercurial if the .hg directory is present.
        if os.access('../../.hg', os.F_OK):
            hg_config = commands.getoutput("hg showconfig")

            for line in hg_config.split("\n"):
                params = line.split("=")

                if params[0] == "paths.default":
                    self.scons_env['REVISION'] = params[1]
                    self.scons_env['TAG'] = commands.getoutput("hg identify -i")
                    break
        else:
            self.scons_env['REVISION'] = "Unknown"
            self.scons_env['TAG']      = "Unknown"

        ########################################################################
        # Setup and define Magpie tool
        ########################################################################
        def magpie_scanner(node, env, pth):
            node = node.rfile()

            if not node.exists():
                return []

            res = []

            if str(node).endswith(".h"):
                CScan.scan(node, pth)
                if node.includes:
                    r = [SCons.Node.FS.find_file(x[1], pth)
                         for x in node.includes]
                    if None in r:
                        #raise UserError, "Couldn't find a header %s when " \
                        print "Couldn't find a header %s when " \
                              "scanning %s" % (node.includes[r.index(None)][1],
                                               node)
                    r = [f for f in r if f]
                    res = Flatten(r)
            else:
                contents = node.get_contents()
                for line in contents.split("\n"):
                    if line.startswith("import"):
                        f = line.split()[-1][1:-2]
                        f = SCons.Node.FS.find_file(f, pth)
                        if f:
                            res.append(f)
            return res

        def magpie_path(env, dir):
            return tuple([Dir(x) for x in env["CPPPATH"]])

        def current_check(node, env):
            c = not node.has_builder() or node.current(env.get_calculator())
            #print "CURRENT CHECK", node, c, node.has_builder()
            return c

        magpie_scan = Scanner(magpie_scanner, recursive=True, skeys=[".idl4"],
                              name="magpie_scanner",
                              path_function = \
                              SCons.Scanner.FindPathDirs("CPPPATH"),
                              scan_check=current_check)

        self.scons_env.Append(SCANNERS=magpie_scan)

        self.scons_env["MAGPIE"] = "PYTHONPATH=tools/magpie-parsers/src:tools/magpie " \
                                   "tools/magpie-tools/magpidl4.py"
        self.scons_env["MAGPIE_INTERFACE"] = "v4nicta_n2"
        self.scons_env["MAGPIE_PLATFORM"] = "okl4"
        builddir = str(Dir(self.builddir))
        magpie_cache_dir = os.path.join(builddir, ".magpie/cache")
        magpie_template_dir = os.path.join(builddir, ".magpie/templates")
        self.scons_env["MAGPIE_BASE_COM"] =  \
          "$MAGPIE --with-cpp=$CPP --cpp-flags \"$_CPP_COM_FLAGS $CPPFLAGS $_CPP_STDIN_EXTRA_FLAGS\" $_CPPINCFLAGS $_CPPDEFFLAGS -h $TARGET -i " + \
          "$MAGPIE_INTERFACE -p $MAGPIE_PLATFORM --magpie-cache-dir=%s --magpie-template-cache-dir=%s --word-size=%d" % (magpie_cache_dir, magpie_template_dir, self.machine.wordsize)

        self.scons_env["MAGPIE_SERVER_COM"] = "$MAGPIE_BASE_COM -s $SOURCE"
        self.scons_env["MAGPIE_SERVERHEADERS_COM"] = "$MAGPIE_BASE_COM --service-headers $SOURCE"
        self.scons_env["MAGPIE_CLIENT_COM"] = "$MAGPIE_BASE_COM -c $SOURCE"

        self.scons_env["BUILDERS"]["MagpieServerHeader"] = Builder(
            action=Action("$MAGPIE_SERVERHEADERS_COM", "[IDL ] $TARGET"),
            suffix = "_serverdecls.h",
            src_suffix = ".idl4",
            source_scanner = magpie_scan)

        self.scons_env["BUILDERS"]["MagpieServer"] = Builder(
            action=Action("$MAGPIE_SERVER_COM", "[IDL ] $TARGET"),
            suffix = "_serverloop.c",
            src_suffix = ".idl4",
            source_scanner = magpie_scan)

        self.scons_env["BUILDERS"]["MagpieClient"] = Builder(
            action=Action("$MAGPIE_CLIENT_COM", "[IDL ] $TARGET"),
            suffix = "_client.h",
            src_suffix = ".idl4",
            source_scanner = magpie_scan)

        self.scons_env["BUILDERS"]["AsmFile"] = Builder(
            action=Action("$ASMCOM", "$ASMCOMSTR"),
            suffix = ".s",
            source_scanner = SCons.Defaults.CScan,
            )
        self.scons_env["BUILDERS"]["CppFile"] = Builder(
            action=Action("$CPPCOM", "$CPPCOMSTR"),
            source_scanner = SCons.Defaults.CScan
            )

        ########################################################################
        # Setup and define Reg tool
        ########################################################################
        self.scons_env["REG"] = "python tools/dd_dsl.py"
        self.scons_env["REG_COM"] = "$REG -o ${TARGET.dir} $SOURCES"
        self.scons_env["BUILDERS"]["Reg"] = Builder(action = "$REG_COM",
                                                    src_suffix = ".reg",
                                                    suffix = ".reg.h")

        ########################################################################
        # Setup and define dx tool
        ########################################################################
        self.scons_env["DX"] = "python tools/dd_dsl_2.py"
        self.scons_env["DX_COM"] = "$DX $SOURCES $TARGET " + str(MACHINE.arch)
        self.scons_env["BUILDERS"]["Dx"] = Builder(action = "$DX_COM",
                                                    src_suffix = ".dx",
                                                    suffix = ".h")


        ########################################################################
        # Setup and define di tool
        ########################################################################
        self.scons_env["DI"] = "python tools/di_dsl.py"
        self.scons_env["DI_COM"] = "$DI $SOURCES $TARGET"
        self.scons_env["BUILDERS"]["Di"] = Builder(action = Action("$DI_COM", "[DI  ] $TARGET"),
                                                    src_suffix = ".di",
                                                    suffix = ".h")

        ########################################################################
        # Pre-processed builder
        ########################################################################
        self.scons_env["CPPI"] = "python tools/dd_dsl.py"
        self.scons_env["BUILDERS"]["CppI"] = Builder(action = "$CPPCOM",
                                                    suffix = ".i")

        ########################################################################
        # Macro expanding pre-processed builder
        ########################################################################
        self.scons_env["BUILDERS"]["MExp"] = Builder(
            action = Action("$CPPCOM", "[MEXP] $TARGET"),
            suffix = ".mi",
            source_scanner = SCons.Defaults.CScan
            )

        ########################################################################
        # Setup and define markup of templates
        ########################################################################
        def build_from_template(target, source, env, for_signature = None):
            assert len(target) == 1
            assert len(source) == 1
            target = str(target[0])
            source = str(source[0])

            template = file(source).read()
            output = StringIO()
            result = markup(template, output, env["TEMPLATE_ENV"])
            assert result is True
            file(target, 'wb').write(output.getvalue())

        self.scons_env["BUILDERS"]["Template"] = Builder(action = build_from_template,
                                                         src_suffix = ".template",
                                                         suffix = "")
        self.scons_env["TEMPLATE_ENV"] = {}

        ########################################################################
        # Setup and define Flint tool
        ########################################################################

        self.scons_env["FLINT"] = "flint"
        self.scons_env["FLINT_OPTIONS"] = ["tools/settings.lnt"]
        self.scons_env["FLINT_OPTIONS"].append("\"+cpp(%s)\"" % ",".join(self.cxx_ext))
        self.scons_env["FLINT_OPTIONS"].append("+sp%d" % (self.machine.wordsize/8))
        # We ensure that long is always the same size as pointer
        self.scons_env["FLINT_OPTIONS"].append("+sl%d" % (self.machine.wordsize/8))
        def _flint_extra (flint_run, source):
            extras = []
            if flint_run == 1:
                extras.append("-zero")
            return extras

        self.scons_env["_FLINT_EXTRA"] = _flint_extra
        self.scons_env["_FLINT_OPTIONS"] = '${_FLINT_EXTRA(FLINT_RUN)} ${_concat("", FLINT_OPTIONS, "", __env__)}'
        self.scons_env["FLINTCOM"] = "$FLINT $_FLINT_OPTIONS $_FLINT_SUPPRESS $_CPPDEFFLAGS $_CPPINCFLAGS  $SOURCE"
        self.scons_env["FLINT_EXTS"] = self.c_ext + self.cxx_ext

        ########################################################################
        # Setup and define splint tool
        ########################################################################

        self.scons_env["SPLINT"] = "splint"
        self.scons_env["SPLINT_OPTIONS"] = ["-nolib",
                                            "-weak"]
        self.scons_env["_SPLINT_OPTIONS"] = '${_concat("", SPLINT_OPTIONS, "", __env__)}'
        self.scons_env["SPLINTCOM"] = "$SPLINT " + \
                                      "$_SPLINT_OPTIONS " + \
                                      "$_CPPDEFFLAGS " + \
                                      "$_CPPINCFLAGS $SOURCE"
        self.scons_env["SPLINT_EXTS"] = self.c_ext

        #
        # Initialise list of linters
        #
        self.linters = ["FLINT", "SPLINT"]

        for key in kargs:
            self.scons_env[key] = copy.copy(kargs[key])

    def process_global_config(self):
        default_max_threads = 1024

        max_threads = get_int_arg(self, "MAX_THREADS",
                                  default_max_threads)

        if (max_threads < 16) or (max_threads > 4092):
            raise UserError, "MAX_THREADS of %d is out of range" % max_threads

        max_thread_bits = int(ceil(log(max_threads, 2)))

        self.Append(CPPDEFINES =[("CONFIG_MAX_THREAD_BITS", max_thread_bits)])

        if not get_bool_arg(self, "ENABLE_DEBUG", True):
            self.scons_env.Append(CPPDEFINES = ["NDEBUG"])

        # Read in the scheduling behaviour
        sched_algorithm = get_arg(self, "SCHEDULING_ALGORITHM", 'inheritance').lower()
        self.args["SCHEDULING_ALGORITHM"] = sched_algorithm
        if (sched_algorithm == 'inheritance'):
            self.Append(CPPDEFINES = [("CONFIG_SCHEDULE_INHERITANCE", 1)])
        elif (sched_algorithm == 'strict'):
            self.Append(CPPDEFINES = [("CONFIG_STRICT_SCHEDULING", 1)])
        else:
            raise UserError, "'%s' is not a valid scheduling algorithm." \
                  % sched_algorithm

        feature_profiles = ["NORMAL", "EXTRA"]
        feature_profile = get_option_arg(self, "OKL4_FEATURE_PROFILE",
                                             self.machine.default_feature_profile, feature_profiles)
        self.feature_profile = feature_profile
        if feature_profile == "EXTRA":
            self.Append(CPPDEFINES=[("CONFIG_EAS", 1),
                                    ("CONFIG_ZONE", 1),
                                    ("CONFIG_MEM_PROTECTED", 1),
                                    ("CONFIG_MEMLOAD", 1),
                                    ("CONFIG_REMOTE_MEMORY_COPY", 1)])

        self.machine.simulate_cache = self.args.get("ENABLE_CACHE_EMUL", True)

    def set_scons_attr(self, target, attr, value):
        if isinstance(target, SCons.Node.NodeList):
            target = target[0]

        setattr(target.attributes, attr, value)

    def GenWeaverXML(self, dom, apps):
        """Generate a weave XML file."""

        def generate_xml(target, source, env):

            f = open(target[0].abspath, "w")
            f.write(dom.toprettyxml())
            f.close()

        weaver = os.path.join(self.builddir, "weaver.xml")
        # HACK: Should depend on machine element as well.
        apps = Flatten(apps)
        spec = Command(weaver,
                       apps,
                       Action(generate_xml, "[XML ] $TARGET"))

        return spec

    def Package(self, package, buildname=None, duplicate = 0, **kargs):
        """Called to import a package. A package contains source files and
        a SConscript file. It may create multiple Libraries and Applications."""
        # Export env and args to the app SConstruct
        if buildname is None:
            buildname = package.replace(os.sep, "_")
        builddir = os.path.join(self.builddir, "object", buildname)

        args = copy.copy(self.args)
        args.update(kargs)
        if hasattr(conf, self.name) and hasattr(getattr(conf, self.name), buildname):
            kargs.update(getattr(getattr(conf, self.name), buildname).dict)

        env = self
        args["buildname"] = buildname
        args["package"] = package
        args["system"] = self.system
        args["cust"] = self.cust
        args["platform"] = self.machine.platform

        # Call the apps SConstruct file to build it
        paths = [
            os.path.join('cust', self.cust, package),
            os.path.join(Dir("#/.").srcnode().abspath, 'cust', self.cust, package),
            package,
            os.path.join(Dir("#/.").srcnode().abspath, package),
            os.path.join(self.oklinux_dir, package),
        ]

        if self.cell is not None:
            cell = self.cell
            exports = ['env', 'args', 'cell']
        else:
            exports = ['env', 'args']

        for path in paths:
            sconscript_path = os.path.join(path, 'SConscript')
            if os.path.exists(sconscript_path):
                return SConscript(sconscript_path, build_dir=builddir,
                                  exports = exports, duplicate = duplicate)

        #raise UserError("Unable to find package: %s" % package)
        print UserError("Unable to find package: %s" % package)
        return (None, None, None)

    def LayoutVirtual(self, apps):
        """Layout a list of objects in memory.  Virtual, physical and
        rommable addresses are calculated separately"""

        def get_next_address(target, source, env):

            def check_fixed_collision(address, size_hint, fixed_apps):
                """Check to see if this address collides with a fixed memsection and
                find the next hole if it does"""

                for app in fixed_apps:
                    addressing = app.attributes.addressing
                    if address < app.attributes.weaver.get_last_virtual_addr(addressing) and\
                            address > addressing.virt_addr - size_hint:
                        #find next hole
                        address = align_up(app.attributes.weaver.get_last_virtual_addr(addressing),
                                           addressing.get_align())
                    elif address <= addressing.virt_addr - size_hint:
                        break

                return address

            weaver = source[0].attributes.weaver
            addressing = source[0].attributes.addressing
            alignment = source[1].value
            size_hint = addressing.get_size_hint()
            address = align_up(weaver.get_last_virtual_addr(addressing), alignment)
            address = check_fixed_collision(address, size_hint, fixed_apps)
            open(target[0].abspath, "w").write("0x%xL\n" % address)


        def set_next_address(target, source, env):
            open(target[0].abspath, "w").write("0x%xL\n" % source[0].value)

        apps = Flatten(apps)
        addr_apps = [app for app in apps
                     if getattr(app.attributes, 'addressing', None)]
        fixed_apps = []
        non_fixed_apps = []

        for app in addr_apps:
            if app.attributes.addressing.ignore_virt():
                continue
            elif app.attributes.addressing.virt_is_fixed():
                fixed_apps.append(app)
            else:
                non_fixed_apps.append(app)

        fixed_apps.sort(key=lambda x: x.attributes.addressing.virt_addr)

        # hack to remove pistachio after we have made the linkfile
        fixed_apps_to_remove = []

        for app in fixed_apps:
            addressing = app.attributes.addressing
            linkfile = File(app.abspath + ".linkaddress")
            app.LINKFILE = linkfile

            Depends(app, linkfile)

            # hack to still make linkfile but ignore for future apps
            if addressing.virt_addr == 0x0:
                # L4 on ia32
                fixed_apps_to_remove.append(app)
            else:
                for x in non_fixed_apps:
                    Depends(x, linkfile)

            self.Command(app.LINKFILE, Value(addressing.virt_addr),
                    Action(set_next_address, "[VIRT] $TARGET"))

        # Put the first non-fixed app at the bottom of memory.
        for app in fixed_apps_to_remove:
            fixed_apps.remove(app)

        if len(non_fixed_apps) != 0:
            app = non_fixed_apps.pop(0)
            linkfile = File(app.abspath + ".linkaddress")
            app.LINKFILE = linkfile

            Depends(app, linkfile)

            Command(app.LINKFILE, Value(self.machine.base_vaddr),
                    Action(set_next_address, "[VIRT] $TARGET"))
            last_app = app
        else:
            last_app = None

        # Put remaining apps at increasing address spaces.
        for app in non_fixed_apps:
            addressing = app.attributes.addressing
            linkfile = File(app.abspath + ".linkaddress")
            app.LINKFILE = linkfile

            Depends(app, linkfile)

            self.Command(app.LINKFILE, [last_app,
                                   Value(app.attributes.addressing.get_align())],
                    Action(get_next_address, "[VIRT] $TARGET"))
            last_app = app

        return apps

    def CreateImages(self, weaverxml, apps):
        elf_image = self.Command(os.path.join(self.builddir, "image.elf"),
                            [weaverxml, self.elfweaver],
                            Action("$ELFWEAVER --traceback merge " + \
                                   "-o$TARGET $SOURCE", "[ELF ] $TARGET"))

        if self.machine.zero_bss == True:
            elf_image = self.Command(os.path.join(self.builddir, "image.elf.nobits"),
                                [elf_image, self.elfweaver],
                                Action("$ELFWEAVER modify " + \
                                       "-o$TARGET $SOURCE --remove_nobits", "[ELF ] $TARGET"))

        if self.machine.copy_elf == True:
            elf_image = self.Command(os.path.join(self.builddir, "../image.elf"),
                    elf_image,
                    Action("cp $SOURCE $TARGET", "[ELF ] $TARGET"))

        Depends(elf_image, apps)

        boot_image = self.Command(os.path.join(self.builddir, "image.boot"),
                            [elf_image, self.elfweaver],
                            Action("$ELFWEAVER modify " + \
                                   "-o$TARGET $SOURCE --physical_entry --physical", "[ELF ] $TARGET"))

        if self.machine.arch == "mips":
            boot_image = self.Command(os.path.join(self.builddir, "image.mips"),
                                [elf_image, elfweaver],
                                Action("$ELFWEAVER modify " + \
                                       "-o$TARGET $SOURCE --physical_entry --physical --adjust segment.paddr +0x80000000",
                                       "[ELF ] $TARGET"))

        if self.machine.boot_binary == True:
            boot_image = self.Command(os.path.join(self.builddir, "image.boot.bin"),
                                [boot_image, self.elfweaver],
                                Action("$ELFWEAVER modify " + \
                                       "-o$TARGET $SOURCE --binary", "[BIN ] $TARGET"))

        Depends(boot_image, apps)

        sim_image = self.Command(os.path.join(self.builddir, "image.sim"),
                           [elf_image, self.elfweaver],
                           Action("$ELFWEAVER modify " + \
                                  "-o$TARGET $SOURCE --physical_entry", "[ELF ] $TARGET"))

        if self.machine.arch == "mips":
            sim_image = self.Command(os.path.join(self.builddir, "image.sim"),
                                [elf_image, self.elfweaver],
                                Action("$ELFWEAVER modify " + \
                                       "-o$TARGET $SOURCE --physical_entry --physical --adjust segment.paddr +0x80000000", "[ELF ] $TARGET"))

        Depends(sim_image, boot_image)

        # Configure the 'memstats' target to generate memory
        # statistics of the image.
        st = self.Command("#memstats", [elf_image, self.elfweaver],
                          Action("$ELFWEAVER memstats -x -r initial -R $REVISION -c $TAG -l 20 $SOURCE",
                                 "[STAT] $SOURCE"))
        AlwaysBuild(st)

        return [elf_image, sim_image, boot_image]

    def CreateFatImage(self, install_bootable):

        elf_image = install_bootable[0]

        # Generate an image that can be used for the simulator...
        sim_menulst = self.Command("%s/grub/menu.lst" % self.builddir, elf_image,
                              self.BuildMenuLst, ROOT="/boot/grub")

        install_bootable += sim_menulst
        sim_image = self.Command("%s/c.img" % self.builddir, install_bootable , self.GrubBootImage)

        return sim_image

    def BuildMenuLst(self, target, source, env):
        menulist_serial = """
        serial --unit=0 --stop=1 --speed=115200 --parity=no --word=8
        terminal --timeout=0 serial console
        """

        menulst_data = """
        default 0
        timeout = 0
        title = L4 NICTA::Iguana
        root (hd0,0)
        kernel=%s/image.elf
        """
        menulst = target[0]
        dir = env["ROOT"]

        menu = menulist_serial + menulst_data % dir

        open(str(menulst), "w").write(menu)

    def GrubBootImage(self, target, source, env):
        GRUB_BIN = "tools/grub/grub"
        GRUB_STAGE_1 = "tools/grub/stage1"
        GRUB_STAGE_2 = "tools/grub/stage2"
        GRUB_STAGE_FAT = "tools/grub/fat_stage1_5"

        elf_image = source[0]
        output_image = target[0]

        unstripped_image = str(elf_image) + ".unstripped"
        os.system("cp %s %s" % (elf_image, unstripped_image))
        os.system("strip %s" % elf_image)

        bmap = "%s/grub/bmap" % self.builddir[1:]
        print bmap
        mtoolsrc = "%s/grub/mtoolsrc" % self.builddir[1:]
        print mtoolsrc

        open(bmap, "w").write("(hd0) %s" % output_image)
        print "wrote bmap"
        open(mtoolsrc, "w").write('drive c: file="%s" partition=1\n' % output_image)
        print "wrote mtoolsrc"
        # This needs to by dynamically set (from disk_size in projects/iguana/SConstruct?)
        size = 48000
        sectors = size * 1024 / 512
        tracks = sectors / 64 / 16

        def mtoolcmd(str):
            #return os.system("MTOOLSRC=%s tools/%s" % (mtoolsrc, str))
            error =  os.system("MTOOLSRC=%s tools/mtools/%s" % (mtoolsrc, str))
            if error != 0:
                raise UserError, "%s returned error, perhaps you haven't got mtools-bin installed?" % str
            else:
                return 0

        def copy(file):
            # Hack! If file ends in .reloc, strip it!
            if file.abspath.endswith(".reloc"):
                filenm = file.abspath[:-6]
            elif file.abspath.endswith(".gz"):
                filenm = file.abspath[:-3]
            else:
                filenm = file.abspath


            x = mtoolcmd("mcopy -o %s c:/boot/grub/%s" % (file.abspath, os.path.basename(filenm)))
            if x != 0:
                os.system("rm %s" % output_image)
                raise SCons.Errors.UserError, "Bootimage disk full"

        os.system("dd if=/dev/zero of=%s count=%s bs=512" % (output_image, sectors) ) # Create image
        mtoolcmd("mpartition -I c:")
        mtoolcmd("mpartition -c -t %s -h 16 -s 63 c:" % tracks) #
        mtoolcmd("mformat c:") # Format c:
        mtoolcmd("mmd c:/boot")
        mtoolcmd("mmd c:/boot/grub")
        mtoolcmd("mcopy %s c:/boot/grub" % GRUB_STAGE_1)
        mtoolcmd("mcopy %s c:/boot/grub" % GRUB_STAGE_2)
        mtoolcmd("mcopy %s c:/boot/grub" % GRUB_STAGE_FAT)

        os.system("""echo "geometry (hd0) %s 16 63
                  root (hd0,0)
                  setup (hd0)" | %s --device-map=%s --batch
                  """ % (tracks, 'tools/grub/grub', bmap))

        for each in source:
            print "copying %s to bootdisk" % each.abspath
            copy(each)

    ############################################################################
    # Methods called by SConscript
    ############################################################################
    class _extra:
        def __init__(self, data):
            self.data = data

    def _mogrify(self, kargs):
        for key in kargs:
            if isinstance(kargs[key], self._extra):
                kargs[key] = self.scons_env[key] + kargs[key].data
        return kargs

    def Extra(self, arg):
        return self._extra(arg)

    def AddPostAction(self, files, action, **kargs):
        return self.scons_env.AddPostAction(files, action, **self._mogrify(kargs))

    def Append(self, *args, **kargs):
        return self.scons_env.Append(*args, **self._mogrify(kargs))

    def AsmFile(self, *args, **kargs):
        return self.scons_env.AsmFile(*args, **self._mogrify(kargs))

    def CppFile(self, *args, **kargs):
        return self.scons_env.CppFile(*args, **self._mogrify(kargs))

    def CopyFile(self, target, source):
        return self.scons_env.InstallAs(target, source)

    def Command(self, target, source, action, **kargs):
        return self.scons_env.Command(target, source, action, **self._mogrify(kargs))

    def Depends(self, target, source):
        return self.scons_env.Depends(target, source)

    def CopyTree(self, target, source, test_func=None):
        ret = []
        for root, dirs, files in os.walk(Dir(source).srcnode().abspath, topdown=True):
            if "=id" in files:
                files.remove("=id")
            if ".arch-ids" in dirs:
                dirs.remove(".arch-ids")
            stripped_root = root[len(Dir(source).srcnode().abspath):]
            dest = Dir(target).abspath + stripped_root
            if files:
                for f in files:
                    src = root + os.sep + f
                    des = dest + os.sep + f
                    if not test_func or test_func(src):
                        ret += self.CopyFile(des, src)
            elif not dirs:
                ret += self.Command(Dir(dest), [], Mkdir(dest))
        return ret

    def CopySConsTree(self, target, sources):
        """
        Copy what SCons thinks should go into the source tree, not
        its actual contents.
        """
        def glob_nodes(target_dir, source_dir):
            target_nodes = []
            source_nodes = []

            for node in Dir(source_dir).all_children():
                if isinstance(node, SCons.Node.FS.Dir):
                    (t, s) = glob_nodes(target_dir.Dir(os.path.basename(node.path)), node)
                    target_nodes.append(t)
                    source_nodes.append(s)
                else:
                    target_nodes.append(target_dir.File(os.path.basename(node.path)))
                    source_nodes.append(node)

            return (flatten(target_nodes), flatten(source_nodes))

        if (type(sources) != type([])):
            sources = [sources]
        copies = []

        # Find all the files we need to copy.
        for source in sources:
            (target_nodes, source_nodes) = glob_nodes(Dir(target), Dir(source))
            for t, s in zip(target_nodes, source_nodes):
                copies.append(self.CopyFile(t, s))

        return copies

    def Ext2FS(self, size, dev):
        genext2fs = SConscript(os.path.join(self.oklinux_dir, "tools", "genext2fs", "SConstruct"),
                               build_dir=os.path.join(self.builddir, "native", "tools", "genext2fs"),
                               duplicate=0)
        ramdisk = self.builddir + os.sep + "ext2ramdisk"
        cmd = self.Command(ramdisk, Dir(os.path.join(self.builddir,
                                                     "install")),
                           genext2fs[0].abspath +
                           " -b $EXT2FS_SIZE -d $SOURCE -f $EXT2FS_DEV $TARGET",
                           EXT2FS_SIZE=size, EXT2FS_DEV=dev)

        Depends(cmd, genext2fs)

        # always regenerate the ramdisk file
        AlwaysBuild(cmd)
        return cmd

    def _build_objects(self, build_path, args, source_list):
        obj_dir = os.path.join(*([self.builddir] + build_path))
        objects = [self.scons_env.StaticObject(os.path.join(obj_dir, self.source_to_obj(src, '.o')), src, **args) for src in source_list]
        objects = Flatten(objects)
        return objects

    def _find_source(self, buildname, test_lib=False):
        """This function returns the globs used to automagically find
        source when the user doesn't supply it explicitly."""
        source = []
        if buildname == "ig_server":
            buildname = os.path.join("iguana","server")
        dirs = ["src/",
                "src/%s/" % (self.kernel),
                "#arch/%s/%s/src/" % (self.machine.arch, buildname),
                "#arch/%s/%s/src/toolchain-%s/" % (self.machine.arch, buildname, self.toolchain.name),
                "#arch/%s/libs/%s/src/" % (self.machine.arch, buildname),
                "#cust/%s/arch/%s/%s/src/" % (self.cust, self.machine.arch, buildname),
                "#cust/%s/arch/%s/%s/src/toolchain-%s/" % (self.cust, self.machine.arch, buildname, self.toolchain.name),
                "#cust/%s/%s/cust/src/" % (self.cust, buildname),
                "#cust/%s/%s/cust/src/toolchain-%s/" % (self.cust, buildname, self.toolchain.name),
                "system/%s/src/" % self.system
                ]
        if test_lib:
            dirs.append("test/")
        for src_ext in self.src_exts:
            for dir_ in dirs:
                source.append(dir_ + "*." + src_ext)
        return source

    def _setup_linters(self, objects):
        """Setup any linters to run on the list of objects."""
        for linter in self.linters:
            if "%s_RUN" % linter in self.args:
                for obj in objects:
                    ext = str(obj.sources[0]).split(".")[-1]
                    if ext in self.scons_env["%s_EXTS" % linter]:
                        self.scons_env.AddPreAction(obj, Action("$%sCOM" % linter, "[LINT] $SOURCE"))

    def KengeIDL(self, source=None):
        """
        Build IDL files into client and server header and source
        files.  The function returns a tuple containing:

        1) A list of nodes of client headers.  This can be passed to
           KengeLibrary() via the extra_headers keyword.
        2) A list of nodes of client source files.  This can be passed to
           KengeLibrary() via the extra_source keyword.
        3) A hash of server header files.  The key is the basename of
           the IDL file.
        4) A hash of server source files.  The key is the basename of
           the IDL file.  The hash value can be passed to KengeProgram
           via the extra_source keyword.

        The use of hashes in (3) and (4) is because the source list to
        this function can contain multiple IDL files, but a server may
        only implement one of the interfaces.

        The implementation currently uses the Magpie IDL compiler.
        """
        if source is None:
            source = ["include/interfaces/*.idl4"]

        idl4_files = Flatten([src_glob(glob) for glob in source])

        server_headers = {}
        server_src     = {}
        client_headers = []
        client_src     = []

        for src in idl4_files:
            srv  = self.scons_env.MagpieServer(src)[0]
            dest = os.path.join(self.builddir, str(srv))
            server_src[os.path.basename(src)] = self.scons_env.InstallAs(dest, str(srv))

            srv_header = self.scons_env.MagpieServerHeader(src)[0]
            self.scons_env.Depends(srv_header, srv)
            dest = os.path.join(self.builddir, str(srv_header))
            client_headers += self.scons_env.InstallAs(dest, str(srv_header))

            cli  = self.scons_env.MagpieClient(src)[0]
            dest = os.path.join(self.builddir, str(cli))
            client_headers += self.scons_env.InstallAs(dest, str(cli))

        # Magpie does not generate client src files, but include the
        # possibility in the API in case the implementation changes.
        return (client_headers, client_src, server_headers, server_src)

    def add_public_headers(self, name, buildname, public_headers):
        if public_headers is None:
            public_headers = [("include/", "include/%(name)s/"),
                              ("include/%(kernel)s/", "include/%(name)s/"),
                              ("#arch/%(arch)s/%(name)s/include/", "include/%(name)s/arch/"),
                              ("#arch/%(arch)s/libs/%(name)s/include/", "include/%(name)s/arch/"),
                              ("#arch/%(arch)s/libs/%(name)s/v%(ver)s/include/", "include/%(name)s/arch/ver/"),
                              ("#cust/%(cust)s/%(name)s/include/", "include/%(name)s/cust/"),
                              ("#cust/%(cust)s/arch/%(arch)s/libs/%(name)s/include/", "include/%(name)s/arch/"),
                              ("#cust/%(cust)s/arch/%(arch)s/libs/%(name)s/v%(ver)s/include/", "include/%(name)s/arch/ver/"),
                              ("#cust/%(cust)s/arch/%(arch)s/%(name)s/include/", "include/%(name)s/arch/"),
                              ("system/%(system)s/include/", "include/%(name)s/system/"),
                              ("toolchain/%(toolchain)s/include/", "include/%(name)s/toolchain/")
                              ]
            if self.test_libs and name in self.test_libs:
                public_headers.append(("test/", "include/%(name)s/"))

        src_replacements = {"arch" : self.machine.arch,
                            "ver"  : self.machine.arch_version,
                            "cust" : self.cust,
                            "system" : self.system,
                            "toolchain" : self.toolchain.name,
                            "name" : buildname,
                            "kernel" : self.kernel,
                            }
        dst_replacements = {"name" : buildname}

        for src, dest in public_headers:
            headers = src_glob( (src + "*.h") % src_replacements)
            for header in headers:
                dir, header_file = os.path.split(header)
                destfile = os.path.join(self.builddir, dest % dst_replacements)
                destfile = os.path.join(destfile, header_file)
                self.headers += self.scons_env.InstallAs(destfile, header)

    def strip_suffix(self, filename):
        pos = filename.rfind(".")
        if pos == -1:
            return filename
        return filename[:pos]

    def source_to_obj(self, filename, suffix):
        base = self.strip_suffix(filename)
        if base.startswith('#'):
            base = base[1:]
        return base


    def KengeLibrary(self, name, buildname=None, source=None, extra_source=None,
                     public_headers=None, extra_headers=None, dx_files=None,
                     di_files=None, reg_files=None, **kargs):
        """
        source is a list of 'globs' describing which files to compile.
        """
        if buildname is None:
            buildname = name

        # First step in building a library is installing all the
        # header files.
        self.add_public_headers(name, buildname, public_headers)

        kargs = self._mogrify(kargs)
        library_args = kargs

        # We want to make sure the source directory is in the include path
        # for private headers
        if "CPPPATH" not in library_args:
            library_args["CPPPATH"] = copy.copy(self.scons_env["CPPPATH"])
        library_args["CPPPATH"].append(Dir("src").abspath)

        # Handle .di files
        if di_files is None:
            di_files = ["include/*.di"]

        di_file_list = Flatten([src_glob(glob) for glob in di_files])
        for fl in di_file_list:
            di_header = self.scons_env.Di(fl)
            self.scons_env.Install(self.builddir + "/include/driver/", di_header)

        if extra_headers is not None:
            self.headers += extra_headers

        if reg_files:
            reg_file_list = Flatten([src_glob(glob) for glob in reg_files])
            for fl in reg_file_list:
                reg_header = self.scons_env.Reg(fl)

        # Handle .dx files
        if dx_files is None:
            dx_files = ["src/*.dx"]

        dx_file_list = Flatten([src_glob(glob) for glob in dx_files])

        for fl in dx_file_list:
            dx_header = self.scons_env.Dx(fl)


        if source is None:
            test_lib = self.test_libs and name in self.test_libs
            source = self._find_source(name, test_lib=test_lib)

        if extra_source is not None:
            source += extra_source

        source_list = Flatten([src_glob(glob) for glob in source])

        objects = self._build_objects(["object", "libs_" + name], library_args, source_list)

        self._setup_linters(objects)

        lib_name = os.path.join(self.builddir, "lib", "lib%s.a" % buildname)
        lib = self.scons_env.StaticLibrary(lib_name, objects, **library_args)

        self.libs += lib

        return lib

    def KengeProgram(self, name, buildname=None, source=None, extra_source=None,
                     public_headers=None, relocatable=None, **kargs):
        """
        source is a list of 'globs' describing which files to compile.
        """
        if buildname is None:
            buildname = name

        self.add_public_headers(name, buildname, public_headers)

        kargs = self._mogrify(kargs)
        program_args = kargs
        program_args["CPPPATH"] = program_args.get("CPPPATH",
                                                   copy.copy(self.scons_env["CPPPATH"]))
        program_args["CPPPATH"].append(Dir("include"))

        depend_prefix = ""
        if "LINKSCRIPTS" not in program_args or program_args["LINKSCRIPTS"] is None:
            # Is a default linker script provided?
            if (hasattr(self.machine, "link_script")):
                depend_prefix = "#"     # file comes from source dir, not builddir
                program_args["LINKSCRIPTS"] = self.machine.link_script

        libs = program_args.get("LIBS", [])
        program_args["LIBS"] = list(sets.Set(libs))

        if source is None:
            source = self._find_source(name)

        source_list = Flatten([src_glob(glob) for glob in source])

        # EVIL...
        template_files = Flatten(src_glob("src/*.template"))

        for file_name in template_files:
            # FIXME this is hacky
            template_env = kargs.get("TEMPLATE_ENV", self.scons_env["TEMPLATE_ENV"])
            template = self.scons_env.Template(file_name, TEMPLATE_ENV=template_env)
            self.scons_env.Depends(template, Value(template_env))
            source_list.append(str(template[0]))

        objects = self._build_objects([name, "object"], program_args, [src for src in source_list if not src.endswith(".template")])


        # Support for a list of pre-processed objects
        # eg. as needed by x86_64 for init32.cc
        if extra_source is not None:
            objects += extra_source

        # CEG hack
        if name == "l4kernel" and (self.machine.arch == "x86_64" or
                                   self.machine.arch == "ia32"):
            for obj in objects:
                self.Depends(obj, "include/tcb_layout.h")
                self.Depends(obj, "include/ktcb_layout.h")

        # Generate targets for .i, pre-processed but not compiled files.
        # these aren't depended on by anything and target must be explicitly
        # defined on the command line to get them built.
        #[self.scons_env.CppI(x, **program_args)
        # for x in source_list if not x.endswith(".template")]

        self._setup_linters(objects)

        # For GNU compilers, include our libgcc if needed
        if self.toolchain.type == "gnu" and self.toolchain.dict["LIBGCC"] == "-lgcc":
            program_args['LIBS'] += ["gcc"]

        output = os.path.join(self.builddir, "bin", buildname)

        # Prelink support.  Needed for SoC SDK kernel.o files.
        if relocatable:
            reloc_output = output + ".o"

            program_args['LINKSCRIPTS'] = ''
            program_args['ELFADORN_FLAGS'] = ''
            if not program_args.has_key('LINKFLAGS'):
                program_args['LINKFLAGS'] = []
            program_args['LINKFLAGS'] += ['$RELOCATION_OPT']

            prog = self.scons_env.Program(reloc_output, objects, **program_args)
        else:
            prog = self.scons_env.Program(output, objects, **program_args)

        self.set_scons_attr(prog, 'name', name)

        # FIXME: This should be part of the program scanner, libc might
        # be built after program
        if self.scons_env.get("CRTOBJECT"):
            self.Depends(prog, self.scons_env["CRTOBJECT"])

        if program_args.get("LINKSCRIPTS", None) is not None:
            self.Depends(prog, depend_prefix + program_args["LINKSCRIPTS"])

        self.Depends(prog, self.elfadorn)

        return prog

    def KengeCRTObject(self, source, **kargs):
        # Rule to build a C runtime object.
        output = os.path.join(self.builddir, "lib", "crt0.o")
        source_list = Flatten([src_glob(glob) for glob in source])
        if source_list == []:
            raise UserError, "No valid object in " + \
                  "KengeCRTObject from glob: %s" % source
        obj = self.scons_env.StaticObject(output, source_list, **kargs)
        self.scons_env["CRTOBJECT"] = obj

    def KengeTarball(self, target, source):
        """Create a compressed tarball of the source objects."""
        def make_tarball(target, source, env):
            tar = tarfile.open(str(target[0]), "w:gz");

            for entry in source:
                tar.add(entry.path,
                        entry.path.replace(env['TAR_PREFIX'], '', 1))
            tar.close()

        # Record the prefix of the source directory tree.  This will
        # be remove from files entering the tarball so that contants
        # will be relative to the to of the directory.
        #
        # HACK:  This only works if an environment is building just
        # one tarball.
        self.scons_env['TAR_PREFIX'] = os.path.dirname(Dir(source).path) + os.sep

        return self.scons_env.Command(target, source, Action(make_tarball, "[TAR ] $TARGET"))

    def KengeLicenceTool(self, target, source, sdk):
        """Run the Licence Tool over a directory in expand mode."""
        def expand_licence(target, source, env):
            for src in source:
                sdk_dir = os.path.abspath(str(src))
                cmd_str = "licence_tool --type=%s --mode=expand %s"
                print "Executing: %s" % (cmd_str % (["complete", "sdk"][sdk], sdk_dir))
                ret = os.system(cmd_str % (["complete", "sdk"][sdk], sdk_dir))

                if ret != 0:
                    raise UserError, "licence_tool returned error, perhaps it's not installed?"

        return self.scons_env.Command(target, source,
                                      Action(expand_licence, '[LIC ] $SOURCES'))

    def KengeSed(self, target, source, subs):
        """
        Copy source to target, running through sed with the given
        substitutions.  The subs is a dictionary of (sub, value)
        strings.

        Construction vars can be used in either part.
        """
        substr = ""

        # Because we can get paths as substitutions we need to repalce /'s with
        # \/'s so that sed can cope
        escape_fix = re.compile(r"/")
        for (k, v) in subs.iteritems():
            v = self.scons_env.subst(v)
            v = escape_fix.sub(r"\\/", v)
            substr += 's/%s/%s/; ' % (k, v)

        return self.Command(target, source,
            Action("sed -e '%s' <$SOURCES >$TARGET" % substr, '[SED ] $TARGET'))

    def KengeSedDelete(self, target, source, subs):
        """
        Copy source to target, running through sed with the given
        deletions.  The subs is a dictionary of strings.

        """
        substr = ""

        # Because we can get paths as substitutions we need to repalce /'s with
        # \/'s so that sed can cope
        escape_fix = re.compile(r"/")
        for v in subs:
            v = self.scons_env.subst(v)
            v = escape_fix.sub(r"\\/", v)
            substr += '/%s/d; ' % v

        return self.Command(target, source,
            Action("sed -e '%s' <$SOURCES >$TARGET" % substr, '[SED ] $TARGET'))


class KengeCell(KengeEnvironment):
    """
    The KengeCell is a KengeEnvironment for building cells.

    It is also responsible for creating the components that will go
    into the XML file for elfweaver.

    The XML file is built from a dom kept in the KengeBuild object.
    This is constructed as the SConscript files are processed.  XML
    fragments are added to the dependency list for the XML file, so
    that it a property changes and the XML file will be rebuilt.
    """

    class _Addressing(object):
        """
        Memory allocation properties for weaver objects.

        This class describes how an object should be laid out in
        virtual and physical memory.  The properties for each can be
        specified separately.  If values for these properties are not
        supplied in the constructor then reasonable defaults are
        given.

        There are three properties that can be described:

        1) Address

        The address if the segment in virtual or physical memory.
        Allowable values are:

        None - The address should be calculated by elfweaver.

        ADDR_IGNORE - The object has no meaningful address and should
        be ignored.

        ADDR_BASE - The object should be placed at the bottom of the
        address range.

        ADDR_AS_BUILT - The address as it appears in the ELF segment
        is the address of the segment.  AKA trust the compiler

        ADDR_ALLOC - The address should be allocate by the build
        system.

        <integer> - The absolute address of the object.

        3) Alignment

        The alignment of the segment in memory.  Allowable values are:

        None - Alignment should be determined by elfweaver.  This is not
        meaningful if ADDR_ALLOC is used.

        ALIGN_ELF - Use the alignment as specified in the ELF segment
        header.  AKA trust the compiler.

        ALIGN_MIN - Use the minimum page size for the target
        architecture.

        ALIGN_PREFERRED - The best static alignment for this
        architecture.  This is a single value that is a good trade off
        between TLB usage and memory wastage.

        4) Size Hint

        If the object is expected to take more than 1MB of memory, set the
        size hint to be an overestimate of the size, so that it does not
        collide with any fixed memsections.

        5) Pools

        Specify the regions of memory from which segments are allocated.
        """
        ALIGN_MIN       = -1
        ALIGN_PREFERRED = -2
        ALIGN_ELF       = -4
        ADDR_BASE       = -5
        ADDR_ALLOC      = -6
        ADDR_AS_BUILT   = -7
        ADDR_IGNORE     = -8

        def __init__(self, machine,
                     align      = None,
                     virt_addr  = None,
                     phys_addr  = None,
                     virtpool   = None,
                     physpool   = None,
                     pager      = None,
                     direct     = None,
                     zero       = None,
                     cache_policy = None,
                     size_hint  = 0x100000):

            # Convert MIN and PREFERRED alignments to their machine
            # specific values.
            if align == self.ALIGN_MIN:
                align = machine.min_alignment

            if align == self.ALIGN_PREFERRED:
                align = machine.preferred_alignment

            if virt_addr == self.ADDR_BASE:
                virt_addr = machine.base_vaddr
                self.virt_base = True
            else:
                self.virt_base = False

            self.align      = align
            self.virt_addr  = virt_addr
            self.phys_addr  = phys_addr
            self.virtpool   = virtpool
            self.physpool   = physpool
            self.pager      = pager
            self.direct     = direct
            self.zero       = zero
            self.cache_policy = cache_policy
            self.size_hint  = size_hint
            self.decreasing_page_sizes = machine.page_sizes[:]
            self.decreasing_page_sizes.sort()
            self.decreasing_page_sizes.reverse()
            self.zone_end   = False

        def virt_is_fixed(self):
            """Is the virtual address at a fixed location?"""
            return self.virt_addr != self.ADDR_ALLOC #and not self.virt_base

        def phys_is_fixed(self):
            """Is the physical address at a fixed location?"""
            return self.phys_addr != self.ADDR_ALLOC

        def use_phys_as_built(self):
            """Use the physical address as found in the ELF file?"""
            return self.phys_addr == self.ADDR_AS_BUILT

        def ignore_virt(self):
            """Should this virtual address be ignored?"""
            return (self.virt_addr == self.ADDR_IGNORE or
                    self.virt_addr == None)

        def ignore_phys(self):
            """Should this physical address be ignored?"""
            return (self.phys_addr == self.ADDR_IGNORE or
                    self.phys_addr == None)

        def virt_is_base(self):
            """Place segment at the bottom on virtual memory?"""
            return self.virt_base

        def get_virtpool(self):
            """Return the virtual address pool."""
            return self.virtpool

        def get_physpool(self):
            """Return the physical address pool."""
            return self.physpool

        def get_pager(self):
            """Return the pager."""
            return self.pager

        def get_direct(self):
            """Return the direct flag."""
            return self.direct

        def set_zone_end(self, end):
            """
            Indicate whether or not the weaver item is at the end of a
            zone.
            """
            self.zone_end = end

        def align_zone(self, addr):
            """
            Move an address to the end of the zone it is it the last item.

            This is bit hacky.  When placing items in a zone, no other
            items be placed between the last item and end of the
            zone.  If this is the last item, then change address
            returned to the end of the zone so that LayoutVirtual will
            not put the next item in the same 1M region.
            """
            if self.zone_end:
                return align_up(addr, 1024 * 1024) - 1
            else:
                return addr

        def get_align(self):
            """Return the alignment in virtual memory"""
            return self.align

        def get_phys_addr(self):
            """Return the physical address of the segment."""
            return self.phys_addr

        def get_size_hint(self):
            """Return the size hint or 0x100000 if it is smaller."""
            return max(self.size_hint, 0x100000)

        def set_attrs(self, elem, include_virt = True, program_like=False):
            """Assign the memory attributes to an XML element."""

            def hexstr(num):
                """Return the number as a hex string."""
                return "%#x" % num

            if not program_like and include_virt and self.virt_addr is not None:
                assert self.virt_addr >= 0
                elem.setAttribute('virt_addr', hexstr(self.virt_addr))

            if not program_like and self.phys_addr is not None:
                assert self.phys_addr >= 0
                elem.setAttribute('phys_addr', hexstr(self.phys_addr))

            if not program_like and self.align is not None and \
               (self.virt_addr is None or self.phys_addr is None):
                assert self.align >= 0
                elem.setAttribute('align', hexstr(self.align))

            if include_virt and self.virtpool is not None:
                elem.setAttribute('virtpool', self.virtpool)

            if self.physpool is not None:
                elem.setAttribute('physpool', self.physpool)

            if self.direct is not None:
                elem.setAttribute('direct', xmlbool(self.direct))

            if not program_like and self.zero is not None:
                elem.setAttribute('zero', xmlbool(self.zero))

            if self.pager is not None:
                elem.setAttribute('pager', self.pager)

            if not program_like and self.cache_policy is not None:
                elem.setAttribute('cache_policy', self.cache_policy)

    def __init__(self, name, dom, SYSTEM, CUST, MACHINE, TOOLCHAIN,
                 BUILD_DIR, xml_tag = None, **kargs):
        KengeEnvironment.__init__(self, name, self, SYSTEM, CUST, MACHINE,
                                  TOOLCHAIN, BUILD_DIR, **kargs)

        # Dom for creating elements.
        self.dom   = dom
        # XML file dependencies in this file.
        self.apps  = []

        # Create the top level element for the cell.
        self.elem = self.dom.createElement(xml_tag)
        self.dom.documentElement.appendChild(self.elem)

    # Public interface to addressing constants.
    ALIGN_MIN          = _Addressing.ALIGN_MIN
    ALIGN_PREFERRED    = _Addressing.ALIGN_PREFERRED
    ALIGN_ELF          = _Addressing.ALIGN_ELF
    ADDR_BASE          = _Addressing.ADDR_BASE
    ADDR_ALLOC         = _Addressing.ADDR_ALLOC
    ADDR_AS_BUILT      = _Addressing.ADDR_AS_BUILT
    ADDR_IGNORE        = _Addressing.ADDR_IGNORE

    def Addressing(self,
                   align      = None,
                   virt_addr  = None,
                   phys_addr  = None,
                   virtpool   = None,
                   physpool   = None,
                   pager      = None,
                   direct     = None,
                   zero       = None,
                   cache_policy = None,
                   size_hint  = 0x100000):
        """Return the addressing details for a weaved object."""
        return self._Addressing(self.machine,
                                align      = align,
                                virt_addr  = virt_addr,
                                phys_addr  = phys_addr,
                                virtpool   = virtpool,
                                physpool   = physpool,
                                pager      = pager,
                                direct     = direct,
                                zero       = zero,
                                cache_policy = cache_policy,
                                size_hint  = size_hint)

    def program_like(self, node, elem, addressing, stack = None,
                     heap = None, patches = None,
                     segments = None, priority = None):
        """
        An internal method for setting up program like elements.

        This adds subelements and attributes to <elem> based on the
        the parameters passed to it.
        """

        # Add attributes to the node so that LayoutVirtual() can put
        # the calculate the link addresses of the programs.
        self.set_scons_attr(node, 'addressing', addressing)
        self.set_scons_attr(node, 'weaver', self.AllocatedElfSize(node))

        # Record the file name.
        elem.setAttribute('file', node[0].abspath)

        # Add addressing attributes.
        # Note: At this point the program isn't linked and so the
        # virtual address isn't know.  However, elfweaver gets the
        # virtual address of ELF segments from the ELF file, not from
        # XML attributes, so there is no need to write them out.
        if addressing is not None:
            addressing.set_attrs(elem, program_like = True)

        if priority is not None:
            elem.setAttribute('priority', str(priority))

        # Add segment subelements.
        if segments is not None:
            for s in segments:
                elem.appendChild(s.attributes.xml)
                self.apps.append(s)

        # Add patch subelements.
        if patches is not None:
            for p in patches:
                elem.appendChild(p.attributes.xml)
                self.apps.append(p)

        # Add the details of the stack.
        if stack is not None:
            # If the stack is a number, change it to an XML element.
            if not isinstance(stack,  SCons.Node.Node):
                stack = self.Stack(size = stack)

            elem.appendChild(stack.attributes.xml)
            self.apps.append(stack)

        # Add the details of the heap.
        if heap is not None:
            # If the heap is a number, change it to an XML element.
            if not isinstance(heap,  SCons.Node.Node):
                heap = self.Heap(size = heap)

            self.apps.append(heap)
            elem.appendChild(heap.attributes.xml)

    def Segment(self, name, protected = None,
                      addressing = None):
        """
        Create a SCons node describing an ELF segment.
        """
        elem = self.dom.createElement("segment")

        elem.setAttribute('name', name)

        if protected is not None:
            elem.setAttribute('protected', xmlbool(protected))

        if addressing is not None:
            addressing.set_attrs(elem, include_virt = False)

        # Store XML text in a Value node.  If the text changes, then
        # the XML file will be rebuilt.
        seg = self.scons_env.Value(elem.toxml())
        self.set_scons_attr(seg, 'xml', elem)

        self.apps.append(seg)

        return seg

    def Stack(self, size, attach = None, addressing = None):
        """
        Create a SCons node describing a stack
        """
        elem = self.dom.createElement("stack")

        elem.setAttribute('size', "%#x" % size)

        if attach is not None:
            elem.setAttribute('attach', "%s" % attach)

        if addressing is not None:
            addressing.set_attrs(elem)

        # Store XML text in a Value node.  If the text changes, then
        # the XML file will be rebuilt.
        stack = self.scons_env.Value(elem.toxml())
        self.set_scons_attr(stack, 'xml', elem)

        return stack

    def Heap(self, size, user_map = None, attach = None, addressing = None):
        """
        Create a SCons node describing a heap
        """
        elem = self.dom.createElement("heap")

        elem.setAttribute('size', "%#x" % size)

        if user_map is not None:
            elem.setAttribute('user_map', xmlbool(user_map))

        if attach is not None:
            elem.setAttribute('attach', "%s" % attach)

        if addressing is not None:
            addressing.set_attrs(elem)

        heap = self.scons_env.Value(elem.toxml())
        self.set_scons_attr(heap, 'xml', elem)

        return heap

    def Patch(self, address, value, bytes = None):
        """
        Create a SCons node describing a patch.
        """
        elem = self.dom.createElement("patch")

        # Print string addresses as-is, otherwise assume that they are
        # numbers that should be printed in hex.
        if type(address) is str:
            elem.setAttribute('address', address)
        else:
            elem.setAttribute('address', "%#x" % address)

        elem.setAttribute('value',   "%#x" % value)

        if bytes is not None:
            elem.setAttribute('bytes', str(bytes))

        patch = self.scons_env.Value(elem.toxml())
        self.set_scons_attr(patch, 'xml', elem)

        return patch

    class AllocatedSize(object):
        """
        Generic class to help calculate the size of an object in
        the address space.
        """

        def get_last_virtual_addr(self, addressing):
            """Return the last virtual address used by the object."""
            raise NotImplementedError

    class AllocatedElfSize(AllocatedSize):
        """
        Class to return the size address ranges taken by an ELF
        file.
        """
        def __init__(self, node):
            self.filename = node[0].abspath

        def get_last_virtual_addr(self, addressing):
            elffile = PreparedElfFile(filename=self.filename)
            last_virtual = elffile.get_last_addr("virtual")

            return addressing.align_zone(last_virtual)

    def UseDevice(self, name):
        """
        Create a description for using a physical device within a space.
        """
        elem = self.dom.createElement("use_device")

        elem.setAttribute('name', name)

        use_device = self.scons_env.Value(elem.toxml())
        self.set_scons_attr(use_device, 'xml', elem)

        return use_device

class OKL4Cell(KengeCell):
    """
    A class for a cell that contains a single okl4 program.
    """
    def __init__(self, name, dom, SYSTEM, CUST, MACHINE, TOOLCHAIN,
                 BUILD_DIR, xml_tag = "okl4", **kargs):
        KengeCell.__init__(self, name, dom, SYSTEM, CUST, MACHINE,
                           TOOLCHAIN, BUILD_DIR,
                           xml_tag = xml_tag, **kargs)

        self.elem.setAttribute('name', name)

        self.stack = None
        self.heap  = None
        self.env = None

    # Various objects that can live in a cell (or space)
    def Space(self, name, physpool = None, virtpool = None,
              pager = None, direct = False):
        """
        Create a description of a space that is assigned to a Cell.
        """
        elem = self.dom.createElement("space")

        elem.setAttribute('name', name)

        if physpool is not None:
            elem.setAttribute('physpool', physpool)

        if virtpool is not None:
            elem.setAttribute('virtpool', virtpool)

        if pager is not None:
            elem.setAttribute('pager', pager)

        if direct is not None:
            elem.setAttribute('direct', "%s" % direct)

        space = self.scons_env.Value(elem.toxml())
        self.set_scons_attr(space, 'xml', elem)

        return space

    def Thread(self, name, stack = None, entry = None, priority = None,
               physpool = None, virtpool = None):
        """
        Create a description of a thread that is assigned to an address space.
        """
        elem = self.dom.createElement("thread")

        elem.setAttribute('name', name)

        if entry is not None:
            elem.setAttribute('start', str(entry))

        if priority is not None:
            elem.setAttribute('priority', str(priority))

        if physpool is not None:
            elem.setAttribute('physpool', physpool)

        if virtpool is not None:
            elem.setAttribute('virtpool', virtpool)

        if stack is not None:
            if not isinstance(stack,  SCons.Node.Node):
                stack = self.Stack(size = stack)

            elem.appendChild(stack.attributes.xml)
            self.apps.append(stack)

        seg = self.scons_env.Value(elem.toxml())
        self.set_scons_attr(seg, 'xml', elem)

        return seg

    def Mutex(self, name):
        """
        Create a description of a mutex that is assigned to an address space.
        """
        elem = self.dom.createElement("mutex")

        elem.setAttribute('name', name)

        mutex = self.scons_env.Value(elem.toxml())
        self.set_scons_attr(mutex, 'xml', elem)

        return mutex

    def IRQ(self, value):
        """
        Create a description of an IRQ that is assigned to an address space.
        """
        elem = self.dom.createElement("irq")

        elem.setAttribute('value', "%#x" % value)

        irq = self.scons_env.Value(elem.toxml())
        self.set_scons_attr(irq, 'xml', elem)

        return irq

    def Memsection(self, name, file = None, size = None, attach = None,
                   mem_type = None, addressing = None):
        """
        Create a description of a memsection object that is assigned to an
        address space.
        """
        elem = self.dom.createElement("memsection")

        elem.setAttribute('name', name)

        if file is not None:
            elem.setAttribute('file', str(file))

        if size is not None:
            elem.setAttribute('size', "%#x" % size)

        if attach is not None:
            elem.setAttribute('attach', "%s" % attach)

        if mem_type is not None:
            elem.setAttribute('mem_type', mem_type)

        if addressing is not None:
            addressing.set_attrs(elem)

        memory = self.scons_env.Value(elem.toxml())
        self.set_scons_attr(memory, 'xml', elem)

        return memory

    def Arg(self, name):
        """
        Create a command line argument.
        """
        elem = self.dom.createElement("arg")

        elem.setAttribute('value', name)

        arg = self.scons_env.Value(elem.toxml())
        self.set_scons_attr(arg, 'xml', elem)

        return arg

    def CommandLine(self, args = None):
        """
        Create a command line that is expressed in XML.
        """
        elem = self.dom.createElement("commandline")

        if args is not None:
            for arg in args:
                arg = self.Arg(arg)
                elem.appendChild(arg.attributes.xml)

        cmdline = self.scons_env.Value(elem.toxml())
        self.set_scons_attr(cmdline, 'xml', elem)

        return cmdline

    # Functions to add various objects to a cell (or space)
    def add_spaces(self, spaces):
        """ Assign a list of spaces to a cell. """
        for space in spaces:
            if not isinstance(space, SCons.Node.Node):
                space = self.Space(space)

            self.elem.appendChild(space.attributes.xml)
            self.apps.append(space)

    def add_threads(self, node, threads):
        """
        Assign a list of threads to a space.  If node is none, then the
        threads are assigned to the cell's initial program space.
        """
        if node is None:
            xml = self.elem
        else:
            xml = node.attributes.xml

        for thread in threads:
            if not isinstance(thread, SCons.Node.Node):
                thread = self.Thread(thread)

            xml.appendChild(thread.attributes.xml)
            self.apps.append(thread)

    def assign_irqs(self, node, irqs):
        """
        Assign a list of irqs to a space.  If node is none, then the
        irqs are assigned to the cell's initial program space.
        """
        if node is None:
            xml = self.elem
        else:
            xml = node.attributes.xml

        for irq in irqs:
            if not isinstance(irq, SCons.Node.Node):
                irq = self.IRQ(irq)

            xml.appendChild(irq.attributes.xml)
            self.apps.append(irq)

    def add_use_device(self, node, devices):
        """
        Add a device for use in a space.  If node is none, then the
        device is assigned to the cell's initial program space.
        """
        if node is None:
            xml = self.elem
        else:
            xml = node.attributes.xml

        for dev in devices:
            if not isinstance(dev, SCons.Node.Node):
                dev = self.UseDevice(dev)

            xml.appendChild(dev.attributes.xml)
            self.apps.append(dev)

    def add_mutexes(self, node, mutexes):
        """
        Assign a list of mutexes to a space.  If node is none, then the
        mutexes are assigned to the cell's initial program space.
        """
        if node is None:
            xml = self.elem
        else:
            xml = node.attributes.xml

        for mutex in mutexes:
            if not isinstance(mutex, SCons.Node.Node):
                mutex = self.Mutex(mutex)

            xml.appendChild(mutex.attributes.xml)
            self.apps.append(mutex)

    def add_memsections(self, node, memsections):
        """
        Assign a list of memsection objects to a space.  If node is none, then
        the memsection objects are assigned to the cell's initial program space.
        """
        if node is None:
            xml = self.elem
        else:
            xml = node.attributes.xml

        for memsection in memsections:
            if not isinstance(memsection, SCons.Node.Node):
                memsection = self.Memsection(memsection)

            xml.appendChild(memsection.attributes.xml)
            self.apps.append(memsection)

    def add_command_line(self, node, arg_list = None):
        """
        Add an command line argument list to the initial cell program, such
        that they will appear as argc and argv.
        """
        if node is None:
            xml = self.elem
        else:
            xml = node.attributes.xml

        cmdline = self.CommandLine(arg_list)
        xml.appendChild(cmdline.attributes.xml)
        self.apps.append(cmdline)

    # By default a cell has a 4MB kernel heap.
    def set_cell_config(self, virtpool = None, physpool = None,
                        kernel_heap = 4 * 1024 * 1024,
                        pager = None, direct = None,
                        plat_control = False,
                        caps = None, clists = None, spaces = None,
                        mutexes = None, threads = None, args = None,
                        priority = None):
        """
        Set the common attributes of the cell.

        Must be called after set_program()
        """
        if virtpool is not None:
            self.elem.setAttribute('virtpool', virtpool)

        if physpool is not None:
            self.elem.setAttribute('physpool', physpool)

        if pager is not None:
            self.elem.setAttribute('pager', pager)

        if direct is not None:
            self.elem.setAttribute('direct', xmlbool(direct))

        if kernel_heap is not None:
            self.elem.setAttribute('kernel_heap', "%#x" % kernel_heap)

        if caps is not None:
            self.elem.setAttribute('caps', "%d" % caps)

        if clists is not None:
            self.elem.setAttribute('clists', "%d" % clists)

        if spaces is not None:
            self.elem.setAttribute('spaces', "%d" % spaces)

        if mutexes is not None:
            self.elem.setAttribute('mutexes', "%d" % mutexes)

        if threads is not None:
            self.elem.setAttribute('threads', "%d" % threads)

        if plat_control:
            self.elem.setAttribute('platform_control', xmlbool(plat_control))

        if priority:
            self.elem.setAttribute('priority', "%d" % priority)

        if args is not None:
            self.add_command_line(None, args)

        val = self.scons_env.Value(self.elem.toxml())
        self.set_scons_attr(val, 'xml', self.elem)
        self.apps.append(val)

    def set_program(self, node, stack = None, heap = None,
                    patches = None, segments = None,
                    irqs = None,
                    addressing = None):
        """
        Set the initial program for the cell.
        """
        if addressing is None:
            addressing = self.Addressing(virt_addr = self.ADDR_ALLOC,
                                         align = self.ALIGN_PREFERRED)

        self.apps.append(node)

        self.program_like(node, self.elem, addressing,
                                 patches =  patches,
                                 segments = segments,
                                 stack = stack,
                                 heap = heap)

        if irqs is not None:
            self.assign_irqs(node, irqs)

    def env_append(self, prog, key, attach = None,
                   cap=None, value = None):
        """
        Add an entry to the program's environment.
        """
        if isinstance(prog, SCons.Node.NodeList):
            prog = prog[0]

        #if prog.attributes.env is None:
        if self.env is None:
            env = self.dom.createElement('environment')
            self.env = env
            self.elem.appendChild(env)
        else:
            env = self.env

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
