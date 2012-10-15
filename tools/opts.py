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

# Option processing functions.

import os
from types import TupleType
from xml.dom.minidom import parse
import SCons

# Make it easier to raise UserErrors
import SCons.Errors
UserError = SCons.Errors.UserError

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
    return int(get_arg(build, name, default))

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
    if os.path.exists("../../../../tools/unittest"):
        xmltree = parse("../../../../tools/unittest/" + xml_file).documentElement
    else:
        xmltree = parse("../../tools/unittest/" + xml_file).documentElement

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

