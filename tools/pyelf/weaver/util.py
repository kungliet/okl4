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

import os
import sys

from weaver import MergeError

def _0(n):
    """
    Return 0 if n is None, otherwise n.

    This function can be called before image.layout() and
    therefore addresses and counts can contain Nones.  In
    those circumstances the value doesn't matter (it won't be
    written to a file), so use 0 instead.
    """
    if n is None:
        return 0
    else:
        return n

def get_symbol(elf, symbol, may_not_exist = False):
    sym = elf.find_symbol(symbol)

    if not sym:
        if may_not_exist:
            return None
        else:
            raise MergeError, "Symbol %s not found" % symbol

    address = sym.value
    bytes   = sym.size

    if bytes == 0:
        bytes = elf.wordsize / 8

    return (address, bytes)

def get_symbol_value(elf, symbol):
    """Given a symbol name, return it's contents."""
    sym = elf.find_symbol(symbol)

    size = [elf.wordsize/8, sym.size][sym.size != 0]
    if sym:
        return elf.get_value(sym.value, size)
    else:
        return None

def next_power_of_2(n):
    """Return the next power of two greater than of equal to 'n'."""
    result = 1
    while (result <= n):
        result *= 2
    return result


def arch_import(lookup_table, name, cpu):
    """
    Given an architecture lookup table, lookup the given cpu
    and import "weaver.<name>_<arch>"
    """

    if cpu not in lookup_table:
        raise MergeError, \
              "Unable to determine architecture of cpu: %s" % cpu
    arch = lookup_table[cpu]
    try:
        return getattr(__import__("weaver.%s_%s" % (name, arch)),
                       ("%s_%s" % (name, arch)))
    except ImportError:
        return __import__("%s_%s" % (name, arch))

def import_custom(path, package, ending):
    """
    Import all files with a given ending from a package at a given path.
    """
    if not path: return
    old_path = sys.path

    sys.path = [path]
    for _, _, files in os.walk(os.path.join(path, package)):
        mods = [mod[:-3] for mod in files if mod.endswith(ending)]

        for mod in mods:
            __import__(package + "." + mod)

    sys.path = old_path
