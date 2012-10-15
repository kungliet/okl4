#! /usr/bin/python
#
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
#

import sys
import re

# We assume 4-byte words.
WORD_SIZE = 4

# Default data type
DEFAULT_TYPE = "word_t"

class SyntaxErrorException(Exception):
    pass

class Field(object):
    def __init__(self, aliases, comment = ""):
        """Create a new field. 'aliases' is a list of (ctype, name) pairs."""
        self._aliases = aliases
        self.comment = comment

    def get_asm_names(self):
        return [y for (x, y) in self._aliases]

    def get_aliases(self):
        return [(x, y) for (x, y) in self._aliases if x != "asm"]

    def get_names(self):
        return [y for (x, y) in self._aliases if x != "asm"]

    def get_types(self):
        return [x for (x, y) in self._aliases if x != "asm"]


#
# Generate code for the given structure.
#
class CodeGenerator(object):
    def __init__(self, name, fields, alignment = 0):
        self._name = name
        self._fields = fields
        self._alignment = alignment

    def _generate_field(self, field, index):
        raise NotImplementedError

    def _generate_header(self):
        raise NotImplementedError

    def _generate_footer(self):
        raise NotImplementedError

    def generate(self):
        result = ""
        result += self._generate_header()
        i = 0

        # Generate fields
        for field in self._fields:
            result += self._generate_field(field, i)
            i += 1

        # If we have an alignment constraint, generate additional fields to
        # meet it.
        if self._alignment > 0:
            while (i * WORD_SIZE) % self._alignment != 0:
                result += self._generate_field(Field((DEFAULT_TYPE, "padding%d" % i)))

        result += self._generate_footer()
        return result

#
# C-Struct generator
#
class CStructGenerator(CodeGenerator):
    def __init__(self, name, fields, alignment = 0):
        CodeGenerator.__init__(self, name, fields, alignment)

    def _generate_header(self):
        result = "/* @LICENCE(\"Open Kernel Labs, Inc.\", \"2008\")@ */\n" \
            + "\n" \
            + ("#ifndef __%s_H__\n" % self._name.upper()) \
            + ("#define __%s_H__\n" % self._name.upper()) \
            + "\n" \
            + "/*\n" \
            + " * Auto-generated Structure.\n" \
            + " *\n" \
            + " * Do not modify this file, as your changes will be lost when\n" \
            + " * you next build.\n" \
            + " */\n" \
            + "\n" \
            + ("typedef struct %s %s_t;\n" % (self._name, self._name)) \
            + "\n" \
            + "struct %s\n" \
            + "{\n"
        result = result % (self._name)
        return result

    def _generate_field(self, field, index):
        result = ""

        # Generate comment
        if len(field.comment) > 0:
            result  = "\n    /* %s */\n" % field.comment

        # Generate structure
        aliases = field.get_aliases()
        if len(aliases) == 1:
            result += "    %s %s;\n" % (aliases[0][0], aliases[0][1])
        else:
            result += "    union {\n"
            for i in range(0, len(aliases)):
                result += "        %s %s;\n" % (aliases[i][0], aliases[i][1])
            result += "    };\n"

        return result

    def _generate_footer(self):
        result = "};\n" \
                + "\n" \
                + ("#endif /* __%s_H__ */\n" % self._name.upper())
        return result

#
# CPP Offset Generator
#
class COffsetGenerator(CodeGenerator):
    def __init__(self, name, fields, alignment = 0):
        CodeGenerator.__init__(self, name, fields, alignment)

    def _generate_header(self):
        result = "/* @LICENCE(\"Open Kernel Labs, Inc.\", \"2008\")@ */\n" \
            + "\n" \
            + ("#ifndef __%s_H__\n" % self._name.upper()) \
            + ("#define __%s_H__\n" % self._name.upper()) \
            + "\n" \
            + "/*\n" \
            + " * Auto-generated Structure.\n" \
            + " *\n" \
            + " * Do not modify this file, as your changes will be lost when\n" \
            + " * you next build.\n" \
            + " */\n" \
            + "\n"
        return result

    def _generate_field(self, field, index):
        result = ""

        # Generate comment
        if len(field.comment) > 0:
            result += "\n/* %s */\n" % field.comment

        # Generate definitions
        names = field.get_asm_names()
        for name in names:
            result += "#define %s_%s %d\n" \
                    % (self._name.upper(), name.upper(), index * WORD_SIZE)

        return result

    def _generate_footer(self):
        return "\n" \
                + ("#endif /* __%s_H__ */\n" % self._name.upper()) \
                + "\n"

#
# Python Code Generator
#
class PythonCodeGenerator(CodeGenerator):
    def __init__(self, name, fields, alignment = 0):
        CodeGenerator.__init__(self, name, fields, alignment)

    def _generate_header(self):
        license = "%s\n# @LICENCE(\"Open Kernel Labs, Inc.\", \"2008\")@\n" % \
                (78 * '#')
        result = license \
                + "\n" \
                + "import struct\n" \
                + "from elf.ByteArray import ByteArray\n\n" \
                + "structure = {\n"

        return result

    def _generate_field(self, field, index):
        result = ""
        for field_name in field.get_names():
            result += '    "%s": %s,\n' % (field_name, index*WORD_SIZE)
        return result

    def _generate_footer(self):
        result = '''    }

def %(name)s_size():
    """
    Return the size of the %(name)s structure.
    """
    return %(totalsize)d

def _get_%(name)s_field(%(name)s, name):
    """
    Get the field with a given name from a %(name)s struct
    """
    base = structure[name]
    return struct.unpack('<L', %(name)s[base:base+%(wordsize)d])[0]

def _set_%(name)s_field(%(name)s, name, val):
    """
    Set the value of the field with the given name in a %(name)s struct.
    """
    base = structure[name]
    %(name)s[base:base+%(wordsize)d] = ByteArray(struct.pack('<L', val))

def make_functions(entry):
    """
    This function creates a set of functions in the modue namespace.

    Two functions, a getter and setter, are created for each entry in the
    structure dictionary. These functions allow a user to get or set any
    entry in a %(name)s struct.
    """
    globals()["set_%(name)s_" + entry] = lambda %(name)s, val: _set_%(name)s_field(%(name)s, entry, val)
    globals()["get_%(name)s_" + entry] = lambda %(name)s: _get_%(name)s_field(%(name)s, entry)

# We call make_functions() at import time to populate the module namespace
for entry in structure:
    make_functions(entry)
''' % {"name": self._name,
       "wordsize": WORD_SIZE,
       "totalsize": len(self._fields)*WORD_SIZE}
        return result


# Parse a struct definition file.
def parse_fields(input):

    pragmas = {}
    fields = []
    struct_name = ""

    line_number = 0

    for line in input:
        # Track line number
        line_number += 1

        # Strip out comments, which start with '//'
        comment_index = line.find("//")
        if comment_index >= 0:
            line = line[:comment_index]

        # Comments may also take on the form /* blah */
        line = re.sub(r'^\s*#|/\*([^*]|\*[^/])+\*/', " ", line)

        # Strip space
        line = line.strip()

        # If this line is empty, continue on.
        if len(line) == 0:
            continue

        # Does this line contain the name of the struct?
        name_match = re.match(r"^name +([a-zA-Z_0-9]+)$", line)
        if name_match:
            struct_name = name_match.group(1)
            continue

        # Does this line contain a pragma?
        pragma_match = re.match(r"^pragma +([a-zA-Z_0-9]+) *= *(.*)$", line)
        if pragma_match:
            pragmas[pragma_match.group(1)] = pragma_match.group(2)
            continue

        # Otherwise, is it a field?
        field_match = re.match(r"^([a-zA-Z_0-9|,\* ]+) *(\"[^\"]+\")?$", line)
        if not field_match:
            raise SyntaxErrorException, \
                    "Unknown syntax on line %d.\n" % line_number
        line = field_match.group(1)

        # Parse the comment
        if field_match.group(2):
            comment = field_match.group(2)[1:-1]
        else:
            comment = ""

        # Split up the individual parts
        aliases = line.split(",")
        parsed_aliases = []
        for alias in aliases:
            alias = alias.strip()
            (type, name) = alias.split(" ")
            parsed_aliases.append((type, name))

        # Generate the field
        field = Field(parsed_aliases, comment)
        fields.append(field)

    return (struct_name, fields, pragmas)

# Write text to a file
def output(filename, data):
    f = open(filename, "w")
    print >> f, data
    f.close()


def main():
    # Parse input file
    f = open(sys.argv[1])
    (name, fields, _) = parse_fields(f)

    # Write output
    output_dir = sys.argv[2]
    output("%s/%s.h" % (output_dir, name), CStructGenerator(name, fields).generate())
    output("%s/%s_offsets.h" % (output_dir, name), COffsetGenerator(name, fields).generate())
    output("%s/%s.py" % (output_dir, name), PythonCodeGenerator(name, fields).generate())

if __name__ == '__main__':
    main()
