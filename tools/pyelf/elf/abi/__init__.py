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
Generic interfaces to ABI specific data.
"""

import glob
import os
import sys
import os.path

def check_dir(all_var, directory, names):
    # pylint: disable-msg=E1101
    if os.path.basename(directory) == "abi":
        __path__.append(directory)
        process_dir(all_var, names)

def process_dir(all_var, names):
    for filename in names:
        if (filename.endswith(".py")) and \
               ("__init__" not in filename):
            module = filename[:filename.index(".py")]
            all_var.append(module)

def find_all(basepath):
    all_ = []
    for directory in __path__:
        process_dir(all_, os.listdir(directory))
    for custpath in ["/../../../../cust/*/tools/pyelf/",
                     "/../../../../tools/cust/*/pyelf/"]:
        for path in glob.glob(basepath + custpath):
            if os.path.exists(path):
                os.path.walk(path, check_dir, all_)
    return all_

if type(__path__) == list:
    __all__ = find_all(__path__[0])
else:
    print >> sys.stderr, "Wrong __init__.py - only for unfrozen use"


class ElfRelocType(long):
    """
    A sub-type of integer that allows you to associate a string with a given
    integer, in addition functions can be provided to overwrite class members.
    """
    _show = {}
    _default_string = None
    verbose = False

    def __new__(cls, arg, string=None, calculate_func=None, allocate_func=None,
	*args, **kwargs):
        if string:
            cls._show[arg] = (string, calculate_func, allocate_func, args, kwargs)
        elif arg not in cls._show:
            raise Exception, "Unknown relocation type %d for class %s" % (arg, cls)
        return long.__new__(cls, arg)

    def __str__(self):
        if long(self) in self._show:
            return self._show[long(self)][0]
        elif self._default_string:
            return self._default_string % long(self)
        else:
            return long.__str__(self)

    def calculate(self, elf, sect, symtab, addend, offset, symdx):
        """
        Calculate the value to be written out by this relocation, returns the
        value to be written out or None if we aren't able to handle the
        relocation.
        """
        if long(self) in self._show:
            if self._show[long(self)][1]:
                if self.verbose:
                    print "\t\tCalling calculate function", \
                            self._show[long(self)][1]

                return self._show[long(self)][1](elf, sect, symtab, addend,
                                                 offset, symdx,
                                                 *(self._show[long(self)][3]),
                                                 **(self._show[long(self)][4]))
            else:
                print "No calculate function implemented for relocation type %s" % str(self)
                return None
        else:
            raise Exception, "It should not be possible for this object to exist!"

    def allocate(self, elf, sect, symtab, offset, symdx):
        """
        Perform any allocations that this relocation might require for e.g.
        entries in the .got table.
        """
        if long(self) in self._show:
            if self._show[long(self)][2]:
                if self.verbose:
                    print "\t\tCalling allocation function", \
                            self._show[long(self)][1]

                return self._show[long(self)][2](elf, sect, symtab, offset,
                                                 symdx,
                                                 *(self._show[long(self)][3]),
                                                 **(self._show[long(self)][4]))
            elif self.verbose:
                print "\t\tNo allocation function provided"
        else:
            raise Exception, "It should not be possible for this object to exist!"

# Relocation types are architecture specific.

ABI_REGISTRATION = {}
KERNEL_SOC_LINKER_SCRIPT = {}
