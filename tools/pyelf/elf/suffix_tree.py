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
#pylint: disable-msg=W0212
# we have a recursive, private method so disable warning about accessing a
# _method

"""
This module implements a SuffixTree class. This data structure is an efficient
way of keeping track of a set of strings where some are the suffix of others.

In particular, this could be useful for representing an ELF string table, in
which the efficient packing of data can be achieved by taking advantage of
suffix relations.
"""

class SuffixTree(object):
    """
    Suffix trees efficiently store sets of words. Futhermore, they can
    efficiently produce a list of strings such that every word in the set
    if the suffix of one of the words in the list (along with index information
    (index, offset) to identify the location of each word in the list.
    """

    def __init__(self):
        """Create an empty SuffixTree node."""
        self.children = {}
        self.start = False

    def add(self, string):
        """ Add a string to the suffix tree."""
        last_char = string[-1]
        if last_char not in self.children:
            self.children[last_char] = SuffixTree()
        if len(string) == 1:
            self.children[last_char].start = True
        else:
            self.children[last_char].add(string[:-1])

    def remove(self, string):
        """ Remove a string from the suffix tree."""
        if string:
            char = string[-1]
            child = self.children[char]
            child.remove(string[:-1])
            if not child.start and not child.children:
                del self.children[char]
        else:
            self.start = False

    def __contains__(self, string):
        """ Test whether a given string is in the suffix tree."""
        if string:
            char = string[-1]
            if char in self.children:
                return string[:-1] in self.children[char]
            else:
                return False
        else:
            return self.start

    def to_strings(self):
        """
        Return a list of strings as well as a dictionary
        of string -> (index, index)
        """
        strings, indices, suffix = [], {}, ""
        return self._to_strings(strings, indices, suffix)

    def _to_strings(self, strings, indices, suffix):
        """
        Recursively find the values required for to_strings().
        """
        if self.children:
            for child, subtree in self.children.items():
                strings, indices = subtree._to_strings(strings,
                                                       indices,
                                                       child + suffix)
        else:
            assert self.start
            strings.append(suffix)
        if self.start:
            indices[suffix] = len(strings) - 1, len(strings[-1]) - len(suffix)
        return strings, indices

    def to_nulled_strings(self):
        """
        Return a null separated string and the associated "symbol table"
        representation (string -> offset) for this tree.
        """
        strings, indices = self.to_strings()
        cum_indices = []
        cum_index = 0
        for string in strings:
            cum_indices.append(cum_index)
            cum_index += len(string) + 1

        pos_dict = dict(enumerate(cum_indices))
        for k in indices:
            index, offset = indices[k]
            indices[k] = pos_dict[index] + offset
        return "\0".join(strings), indices


def _main():
    """
    Rudimentary test code.
    """
    tree = SuffixTree()
    tree.add("foo")
    tree.add("poo")
    tree.add("bar")
    tree.add("asjskfhlksdjfghsldkfjgsldkfjg")
    assert "foo" in tree
    assert "poo" in tree
    assert "bar" in tree
    assert "food" not in tree
    assert "oo" not in tree
    tree.add("oo")
    tree.add("asd")
    tree.add("asdasd")
    assert "oo" in tree
    assert "asjskfhlksdjfghsldkfjgsldkfjg" in tree
    print tree.to_strings()

    tree.remove("oo")
    assert "oo" not in tree
    assert "foo" in tree
    tree.remove("asdasd")
    assert "asdasd" not in tree
    assert "asd" in tree

    print tree.to_strings()

    nulled_string, symbol_table = tree.to_nulled_strings()
    for symbol, offset in symbol_table.items():
        assert symbol == nulled_string[offset:].split("\0")[0]

if __name__ == '__main__':
    _main()
