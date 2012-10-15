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

######################################################################
# Library Import + Version checking
######################################################################

import sys
sys.path.append("tools") # Allow us to import bootimg.py
sys.path.append("tools/pyelf")
sys.path = ["tools/pexpect"] + sys.path # Allow us to import pexpect.py

import os.path
import SCons.Defaults
import SCons.Scanner
import os, glob

import traceback
from types import TupleType
from util import contains_any, identity_mapping
from xml.dom import minidom
from optparse import OptionParser

# Make it easier to raise UserErrors
from SCons.Errors import UserError

# We want the SConsign file (where it stores
# information on hashs and times
SConsignFile(".sconsign")

def get_version():
    # Work make the scons version easy to test
    scons_version = tuple([int(x) for x in SCons.__version__.split(".")])

    # Make sure that we have a decent version of SCons
    if scons_version <= (0, 95):
        raise UserError, "Support for SCons 0.95 has been dropped. " + \
              "Please upgrade to at least 0.96"

    # We check we have at least version 2.3, If we don't we are in trouble!
    if sys.version_info < (2, 3):
        raise UserError, "To use the Kenge build system you need Python2.3 "  + \
              "including the python devel packages."
    return scons_version

def src_glob(search, replacement = {}):
    """Src glob is used to find source files easily e.g: src_glob("src/*.c"),
    the reason we can't just use glob is due to the way SCons handles out
    of directory builds."""
    if search.startswith('#'):
        # what does '#' signify?
        search = search[1:]
        dir = os.path.dirname(search)
        if dir != "":
            dir = '#' + dir + os.sep
            #is the below any different from Dir('.').srcnode.abspath ??
        src_path = Dir('#').srcnode().abspath
    elif search.startswith(os.sep):
        # we have been given an absolute path
        dir = os.path.dirname(search)
        if dir != "":
            dir += os.sep
        src_path = os.sep
    else:
        dir = os.path.dirname(search)
        if dir != "":
            dir += os.sep
        src_path = Dir('.').srcnode().abspath

    src_path = src_path % replacement
    search = search % replacement
    files = glob.glob(os.path.join(src_path, search))
    files = map(os.path.basename, files)
    ret_files = []
    for file in files:
        ret_files.append(dir + file)
    # Sort the files so the linkers get the same list of objects
    # and hence deterministic builds
    ret_files.sort()
    return ret_files

################################################################################
# Read configuration files
################################################################################

class Options:
    """Class to hold a set of configuration options. This works as a
    recursive dictionary, with access through the "." method. This allows
    settings things such as:

    conf = Options()
    conf.FOO.BAR.BAZ = 1

    And all intermediate level will be created automagically."""

    def __init__(self):
        """Create a new instance."""
        self.__dict__["dict"] = {}

    def __getattr__(self, attr):
        """Magic getattr method to implement accessing dictionary
        members through the . syntax. Will create a new Options dictionary
        when accessing new names. This method is only called in cases
        when the attribute doesn't already exist."""
        if attr.startswith("__"): # Disallow access to "__" names
            raise AttributeError
        self.__dict__[attr] = Options()
        return self.__dict__[attr]

    def __setattr__(self, attr, value):
        """Sets a new value in the dictionary"""
        self.dict[attr.upper()] = value

def get_conf(args):
    conf = Options()
    # First read options from the .conf file
    if os.path.exists(".conf"):
        execfile(".conf", locals())

    # Then take values from the commands line options
    for (key, value) in args.items():
        parts = key.split(".")
        c = conf
        while len(parts) > 1:
            c = getattr(c, parts[0])
            parts = parts[1:]
        setattr(c, parts[0], value)
    return conf

def load_kenge(conf):
    import kenge
    kenge.conf = conf
    kenge.Dir = Dir
    kenge.SConsignFile = SConsignFile
    kenge.Environment = Environment
    kenge.Scanner = Scanner
    kenge.SCons = SCons
    kenge.Builder = Builder
    kenge.Action = Action
    kenge.SConscript = SConscript
    kenge.Flatten = Flatten
    kenge.src_glob = src_glob
    kenge.Value = Value
    kenge.Command = Command
    kenge.Touch = Touch
    kenge.File = File
    kenge.Depends = Depends
    kenge.CScan = CScan
    kenge.AlwaysBuild = AlwaysBuild


############################################################################
# Cleaning stuff
############################################################################

def clean():
    # Determine if the user is trying to clean
    cleaning = contains_any(["--clean", "--remove", "-c"], sys.argv)

    # Clean out any .pyc from tool
    if cleaning:
        pyc_clean("tools")

# Option processing functions.

def get_bool_arg(build, name, default):
    """Get the boolean value of an option."""
    if type(build) == dict:
        args = build
    else:
        args = build.args
    x = args.get(name, default)
    if type(x) is type(""):
        x.lower()
        if x in [1, True, "1", "true", "yes", "True"]:
            x = True
        elif x in [0, False, "0", "false", "no", "False"]:
            x = False
        else:
            raise UserError, "%s is not a valid argument for option %s. It should be True or False" % (x, name)

    return x

def get_arg(build, name, default):
    if type(build) == dict:
        args = build
    else:
        args = build.args
    return args.get(name, default)

def get_int_arg(build, name, default):
    val = get_arg(build, name, default)

    if val is not None:
        return int(val)
    else:
        return None

def get_option_arg(build, name, default, options, as_list = False):
    """Get the value of an option, which must be one of the supplied set"""
    if type(build) == dict:
        args = build
    else:
        args = build.args
    def_val = default
    arg = args.get(name, def_val)

    if options and type(options[0]) == TupleType:
        # This means a user has specified a list like:
        # [("foo", foo_object), ("bar", bar_object)]
        mapping = dict(options)
        options = [x[0] for x in options]
    else:
        mapping = dict([(str(x), x) for x in options])

    if arg == None:
        if str(arg) not in mapping:
            raise UserError, "need argument for option %s. Valid options are: %s" % (name, options)
        return mapping[str(arg)]

    if not isinstance(arg, str): #Assuming integer
        if arg not in options:
            raise UserError, "%s is not a valid argument for option %s. Valid options are: %s" % (x, name, options)
        return arg
    arg = arg.split(",")
    for x in arg:
        if x not in mapping:
            raise UserError, "%s is not a valid argument for option %s. Valid options are: %s" % (x, name, options)

    if as_list:
        return arg
    else:
        return mapping[arg[0]]


def get_rtos_example_args(xml_file, example_cmd_line):
    """Read the given rtos tests configuration xml file and return a dictionary
       containing the values read"""
    rtos_args = {}
    for example in example_cmd_line:
        if not example == "all":
            rtos_args[example] = {}
            rtos_args[example]["nb_copies"] = 1
            rtos_args[example]["extra_arg"] = 1
    xmltree = minidom.parse("tools/unittest/" + xml_file).documentElement
    rtos_args["server"] = xmltree.getAttribute("server")
    example_nodes = xmltree.getElementsByTagName("example")
    for example_node in example_nodes:
        example_name = example_node.getAttribute("name")
        if example_name in example_cmd_line or example_cmd_line == ["all"]:
            if example_cmd_line == ["all"]:
                rtos_args[example_name] = {}
            rtos_args[example_name]["nb_copies"] = example_node.getAttribute("nb_copies")
            rtos_args[example_name]["extra_arg"] = example_node.getAttribute("extra_arg")
    return rtos_args

################################################################################
# Now find the project SConstruct file to use.
################################################################################

def get_sconstruct_path(conf):
    # Find the project from the arguments.
    project = conf.dict.get("PROJECT", None)
    if "PROJECT" in conf.dict: del conf.dict["PROJECT"]

    # Provide useful message if it doesn't exist
    if project is None:
        raise  UserError, "You must specify the PROJECT you want to build."

    cust = conf.dict.get('CUST', None)
    sconstruct_path = "cust/%s/projects/%s/SConstruct" % (cust, project)
    if not os.path.exists(sconstruct_path):
        sconstruct_path = "projects/%s/SConstruct" % project
        if not os.path.exists(sconstruct_path):
            raise UserError, "%s is not a valid project" % project
    return sconstruct_path


scons_version = get_version()
conf = get_conf(ARGUMENTS)
load_kenge(conf)
clean()
sconstruct_path = get_sconstruct_path(conf)

Export("conf")
Export("scons_version")
Export("UserError")
Export("src_glob")
Export("get_bool_arg")
Export("get_int_arg")
Export("get_arg")
# Execute the SConstruct
execfile(sconstruct_path)
