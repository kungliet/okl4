#!/usr/bin/env python
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

# Functions to demangle a C++ mangled string

from parsers.cxxfilt.demangled_types import *
from parsers.cxxfilt.demangled_exception import DemangledException

# <mangled-name> ::= _Z <encoding>
def demangle_mangled_name(string, top_level):
    if not string.get(2) == "_Z":
        raise DemangledException("Missing \"_Z\" in mangled-name")

    return demangle_encoding(string, top_level)

def has_return_type(demangled):
    if demangled == None:
        return False

    if isinstance(demangled, DemangledTemplate):
        return not is_ctor_dtor_or_conversion(demangled.left)
    elif isinstance(demangled, DemangledRestrictThis) or \
            isinstance(demangled, DemangledVolatileThis) or \
            isinstance(demangled, DemangledConstThis):
        return has_return_type(demangled.left)
    else:
        return False

def is_ctor_dtor_or_conversion(demangled):
    if demangled == None:
        return False

    if isinstance(demangled, DemangledQualifiedName) or \
            isinstance(demangled, DemangledLocalName):
        return is_ctor_dtor_or_conversion(demangled.right)
    elif isinstance(demangled, DemangledCTOR) or \
            isinstance(demangled, DemangledDTOR) or \
            isinstance(demangled, DemangledCast):
        return True
    else:
        return False

# <encoding> ::= <(function) name> <bare-function-type>
#            ::= <(data) name>
#            ::= <special-name>
def demangle_encoding(string, top_level):
    if string.peek() in ("G", "T"):
        return demangle_special_name(string)
    else:
        name = demangle_name(string)

        #if top_level:
        #    while isinstance(name, DemangledRestrictThis) or \
        #            isinstance(name, DemangledVolatileThis) or \
        #            isinstance(name, DemangledConstThis):
        #        name = name.left

        #    if isinstance(name, DemangledLocalName):
        #        dcr = name.right

        #        while isinstance(dcr, DemangledRestrictThis) or \
        #                isinstance(dcr, DemangledVolatileThis) or \
        #                isinstance(dcr, DemangledConstThis):
        #            dcr = dcr.left
        #            name.right = dcr

        #    return name

    if string.peek() in ("", "E"):
        return name

    return DemangledTypedName(name, \
            demangle_bare_function_type(string, has_return_type(name)))

# <name> ::= <nested-name>
#        ::= <unscoped-name>
#        ::= <unscoped-template-name> <template-args>
#        ::= <local-name>
#
# <unscoped-name> ::= <unqualified-name>
#                 ::= St <unqualified-name>
#
# <unscoped-template-name> ::= <unscoped-name>
#                          ::= <substitution>
def demangle_name(string):
    if string.peek() == "N":
        return demangle_nested_name(string)
    elif string.peek() == "Z":
        return demangle_local_name(string)
    elif string.peek() == "L":
        return demangle_unqualified_name(string)
    elif string.peek() == "S":
        if string.peek(1) != "t":
            mangled = demangle_substitution(string, False)
            substitution = True
        else:
            string.advance(2)
            mangled = DemangledQualifiedName(DemangledName("std"), \
                    demangle_unqualified_name(string))
            substitution = False

        if string.peek() == "I":
            # Template candidate
            if not substitution:
                add_substitution(string, mangled)

            mangled = DemangledTemplate(mangled, demangle_template_args(string))

        return mangled
    else:
        mangled = demangle_unqualified_name(string)

        if string.peek() == "I":
            add_substitution(string, mangled)

            mangled = DemangledTemplate(mangled, demangle_template_args(string))

        return mangled

# <nested-name> ::= N [<CV-qualifiers>] <prefix> <unqualified-name> E
#               ::= N [<CV-qualifiers>] <template-prefix> <template-args> E
def demangle_nested_name(string):
    if not string.check("N"):
        raise DemangledException("Missing \"N\" in nested-name")

    (start, end) = demangle_CV_qualifiers(string, True)

    tmp = demangle_prefix(string)

    # Two cases: we can have no CV-qualifiers in which case start and end are
    # None and we just want to return tmp, otherwise tmp needs to be put into
    # the left part of end.  This is so much simpler with real pointers...
    if start == None:
        start = tmp
    else:
        end.left = tmp

    if not string.check("E"):
        raise DemangledException("Missing \"E\" in nested-name")

    return start

# <prefix> ::= <prefix> <unqualified-name>
#          ::= <template-prefix> <template-args>
#          ::=
#          ::= <substitution>
#
# <template-prefix> ::= <prefix> <(template) unqualified-name>
#                   ::= <template-param>
#                   ::= <substitution>
def demangle_prefix(string):
    ret = None

    while True:
        peek = string.peek()

        if string.peek() == "":
            raise DemangledException("End of string in prefix")

        obj = DemangledQualifiedName

        if string.peek().isdigit() or string.peek().islower() or \
                string.peek() in ("C", "D", "L"):
            demangled = demangle_unqualified_name(string)
        elif string.peek() == "S":
            demangled = demangle_substitution(string, True)
        elif string.peek() == "I":
            if ret == None:
                raise DemangledException("No return type for template in prefix")

            obj = DemangledTemplate
            demangled = demangle_template_args(string)
        elif string.peek() == "T":
            demangled = demangle_template_param(string)
        elif string.peek() == "E":
            return ret
        else:
            raise DemangledException("Unkown char in prefix")

        if ret == None:
            ret = demangled
        else:
            ret = obj(ret, demangled)

        if peek != "S" and string.peek() != "E":
            add_substitution(string, ret)

# <unqualified-name> ::= <operator-name>
#                    ::= <ctor-dtor-name>
#                    ::= <source-name>
#                    ::= <local-source-name>
#
# <local-source-name> ::= L <source-name> <discrimination>
def demangle_unqualified_name(string):
    peek = string.peek()

    if string.peek().isdigit():
        return demangle_source_name(string)
    elif string.peek().islower():
        return demangle_operator_name(string)
    elif string.peek() in ("C", "D"):
        return demangle_ctor_dtor_name(string)
    elif string.peek() == "L":
        string.advance()
        ret = demangle_source_name(string)

        demangle_discriminator(string)

        return ret
    else:
        raise DemangledException("Invalid char in unqualified-name")

# <source-name> ::= <(positive length) number> <identifier>
def demangle_source_name(string):
    length = demangle_number(string)

    if length <= 0:
        raise DemangledException("0 length number in source-name")

    ret = demangle_identifier(string, length)
    string.last_name = ret.name

    return ret
    
# <number> ::= [n] <(non-negative decimal integer)>
def demangle_number(string):
    negative = False

    if string.peek() == "n":
        negative = True
        string.advance()

    count = 0
    while string.peek(count).isdigit():
        count += 1

    ret = int(string.get(count))

    if negative:
        return -ret

    return ret

# <identifier> ::= <(unqualified source code identifier)>
def demangle_identifier(string, length):
    ret = string.get(length)

    if len(ret) != length:
        raise DemangledException("Invalid string length in identifier")

    # FIXME: Do stuff with anonymous namespaces here?

    return DemangledName(ret)

# <operator-name> ::= (see op_dict)
#                 ...
#                 ::= cv <type>
#                 ::= v <digit <source-name>
def demangle_operator_name(string):
    # <operator short name>:(<operator representation>, <argument count>)
    op_dict = {
        "aN":("&=", 2),
        "aS":("=", 2),
        "aa":("&&", 2),
        "ad":("&", 1),
        "an":("&", 2),
        "cl":("()", 0),
        "cm":(",", 2),
        "co":("~", 1),
        "dV":("/=", 2),
        "da":("delete[]", 1),
        "de":("*", 1),
        "dl":("delete", 1),
        "dv":("/", 2),
        "eO":("^=", 2),
        "eo":("^", 2),
        "eq":("==", 2),
        "ge":(">=", 2),
        "gt":(">", 2), 
        "ix":("[]", 2),
        "lS":("<<=", 2),
        "le":("<=", 2),
        "ls":("<<", 2),
        "lt":("<", 2),
        "mI":("-=", 2),
        "mL":("*=", 2),
        "mi":("-", 2),
        "ml":("*", 2),
        "mm":("--", 1),
        "na":("new[]", 1),
        "ne":("!=", 2),
        "ng":("-", 1),
        "nt":("!", 1),
        "nw":("new", 1),
        "oR":("|=", 2),
        "oo":("||", 2),
        "or":("|", 2),
        "pL":("+=", 2),
        "pl":("+", 2),
        "pm":("->*", 2),
        "pp":("++", 1),
        "ps":("+", 1),
        "pt":("->", 2),
        "qu":("?", 3),
        "rM":("%=", 2),
        "rS":(">>=", 2),
        "rm":("%", 2),
        "rs":(">>", 2),
        "st":("sizeof ", 1),
        "sz":("sizeof ", 1)
    }

    if string.peek() == "v" and string.peek(1).isdigit():
        string.advance()
        vendor_id = int(string.get())

        return DemangledExtendedOperator(vendor_id, \
                demangle_source_name(string))
    elif string.peek(length = 2) == "cv":
        string.advance(2)

        return DemangledCast(demangle_type(string), None)
    else:
        op_id = string.peek(length = 2)

        if op_dict.has_key(op_id):
            (name, args) = op_dict[op_id]
            string.advance(2)

            return DemangledOperator(name, args)
        else:
            raise DemangledException("Invalid operator-name")

# <special-name> ::= TV <type>
#                ::= TT <type>
#                ::= TI <type>
#                ::= TS <type>
#                ::= GV <(object) name>
#                ::= T <call-offset> <(base) encoding>
#                ::= Tc <call-offset> <call-offset> <(base) encoding>
# G++ Extensions:
#                ::= TC <type> <(offset) number> _ <(base) type>
#                ::= TF <type>
#                ::= TJ <type>
#                ::= GR <name>
#                ::= GA <encoding>
def demangle_special_name(string):
    if string.check("T"):
        char = string.get()
        if char == "V":
            return DemangledVTable(demangle_type(string))
        elif char == "T":
            return DemangledVTT(demangle_type(string))
        elif char == "I":
            return DemangledTypeInfo(demangle_type(string))
        elif char == "S":
            return DemangledTypeInfoName(demangle_type(string))
        elif char == "h":
            demangle_call_offset(string, "h")

            return DemangledThunk(demangle_encoding(string, False))
        elif char == "v":
            demangle_call_offset(string, "v")

            return DemangledVirtualThunk(demangle_encoding(string, False))
        elif char == "c":
            demangle_call_offset(string, "")

            demangle_call_offset(string, "")

            return DemangledCovariantThunk(demangle_encoding(string, False))
        elif char == "C":
            derived = demangle_type(string)
            offset = demangle_number(string)

            if offset < 0:
                raise DemangledException("Negative offset number in special-name")

            if not string.check() == "_":
                raise DemangledException("Missing \"_\" in special-name")

            base = demangle_type(string)

            return DemangledConstructionVTable(base, derived)
        elif char == "F":
            return DemangledTypeInfoFunction(demangle_type(string))
        else:
            raise DemangledException("Unkown char in special-name")
    elif string.check("G"):
        char = string.get()

        if char == "V":
            return DemangledGuard(demangle_name(string))
        elif char == "R":
            return DemangledRefTemp(demangle_name(string))
        elif char == "A":
            return DemangledHiddenAlias(demangle_encoding(string, False))
        else:
            raise DemangledException("Unkown char in special-name")
    else:
        raise DemangledException("Unkown char in special-name")

# <call-offset> ::= h <nv-offset> _
#               ::= v <v-offset> _
#
# <nv-offset> ::= <(offset) number>
#
# <v-offset> ::= <(offset) number> _ <(virtual offset) number>
def demangle_call_offset(string, c):
    if c == "":
        c = string.get()

    if c == "h":
        demangle_number(string)
    elif c == "v":
        demangle_number(string)

        if not string.check("_"):
            raise DemangledException("Missing \"_\" in call-offset")

        demangle_number(string)
    else:
        raise DemangledException("Not a call-offset")

    if not string.check("_"):
        raise DemangledException("Missing \"_\" in call-offset")

# <ctor-dtor-name> ::= C1
#                  ::= C2
#                  ::= C3
#                  ::= D0
#                  ::= D1
#                  ::= D2
def demangle_ctor_dtor_name(string):
    if string.peek() == "C":
        if string.peek(1) == "1":
            obj = DemangledCTORCompleteObject
        elif string.peek(1) == "2":
            obj = DemangledCTORBaseObject
        elif string.peek(1) == "3":
            obj = DemangledCTORCompleteObjectAllocating
        else:
            raise DemangledException("Unkown char in ctor-dtor-name")

        string.advance(2)

        return obj(string.last_name)
    elif string.peek() == "D":
        if string.peek(1) == "0":
            obj = DemangledDTORDeleting
        elif string.peek(1) == "1":
            obj = DemangledDTORCompleteObject
        elif string.peek(1) == "2":
            obj = DemangledDTORBaseObject
        else:
            raise DemangledException("Unkown char in ctor-dtor-name")

        string.advance(2)

        return obj(string.last_name)
    else:
        raise DemangledException("Unkown char in ctor-dtor-name")

# <type> ::= <builtin-type>
#        ::= <function-type>
#        ::= <class-enum-type>
#        ::= <array-type>
#        ::= <pointer-to-member-type>
#        ::= <template-param>
#        ::= <template-template-param> <template-args>
#        ::= <substitution>
#        ::= <CV-qualifiers> <type>
#        ::= P <type>
#        ::= R <type>
#        ::= C <type>
#        ::= G <type>
#        ::= U <source-name> <type>
#
# <builtin-type> ::= (see builtin_dict)
#                ::= u <source-name>
def demangle_type(string):
    # <builin type short name>:(<type representation>, <type printing>)
    builtin_dict = {
        "a":("signed char", PrintType.default),
        "b":("bool", PrintType.bool),
        "c":("char", PrintType.default),
        "d":("double", PrintType.float),
        "e":("long double", PrintType.float),
        "f":("float", PrintType.float),
        "g":("__float128", PrintType.float),
        "h":("unsigned char", PrintType.default),
        "i":("int", PrintType.int),
        "j":("unsigned int", PrintType.unsigned),
        "k":("", PrintType.default),
        "l":("long", PrintType.long),
        "m":("unsigned long", PrintType.unsigned_long),
        "n":("__int128", PrintType.default),
        "o":("unsigned __int128", PrintType.default),
        "p":("", PrintType.default),
        "q":("", PrintType.default),
        "r":("", PrintType.default),
        "s":("short", PrintType.default),
        "t":("unsigned short", PrintType.default),
        "u":("", PrintType.default),
        "v":("void", PrintType.void),
        "w":("wchar_t", PrintType.default),
        "x":("long long", PrintType.long_long),
        "y":("unsigned long long", PrintType.unsigned_long_long),
        "z":("...", PrintType.default)
    }

    peek = string.peek()

    if peek in ("r", "V", "K"):
        (start, end) = demangle_CV_qualifiers(string, False)

        end.left = demangle_type(string)

        add_substitution(string, start)

        return start

    can_subst = True

    if builtin_dict.has_key(peek):
        ret = DemangledBuiltinType(builtin_dict[peek])

        string.advance()
        can_subst = False
    elif peek == "u":
        string.advance()
        ret = DemangledVendorType(demangle_source_name(string))
    elif peek == "F":
        ret = demangle_function_type(string)
    elif peek in ("0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "N", "Z"):
        ret = demangle_class_enum_type(string)
    elif peek == "A":
        ret = demangle_array_type(string)
    elif peek == "M":
        ret = demangle_pointer_to_member_type(string)
    elif peek == "T":
        ret = demangle_template_param(string)

        if string.peek() == "I":
            add_substitution(ret)

            ret = DemangledTemplate(ret, demangle_template_args(string))
    elif peek == "S":
        peek_next = string.peek(1)
        if peek_next.isdigit() or peek_next == "_" or peek_next.isupper():
            ret = demangle_substitution(string, False)

            if string.peek() == "I":
                ret = DemangledTemplate(ret, demangle_template_args(string))
            else:
                can_subst = False
        else:
            ret = demangle_class_enum_type(string)

            if isinstance(ret, DemangledSubstitution):
                can_subst = False
    elif peek == "P":
        string.advance()

        ret = DemangledPointer(demangle_type(string))
    elif peek == "R":
        string.advance()

        ret = DemangledReference(demangle_type(string))
    elif peek == "C":
        string.advance()

        ret = DemangledComplex(demangle_type(string))
    elif peek == "G":
        string.advance()

        ret = DemangledImaginary(demangle_type(string))
    elif peek == "U":
        string.advance()

        ret = demangle_source_name(string)
        ret = DemangledVendorTypeQualified(demangle_type(string), ret)
    else:
        raise DemangledException("Unkown char in type")

    if can_subst:
        add_substitution(string, ret)

    return ret

# <CV-qualifiers> ::= [r] [V] [K]
def demangle_CV_qualifiers(string, member_func):
    start = None
    end = None

    while string.peek() in ("r", "V", "K"):
        if string.peek() == "r":
            if member_func:
                obj = DemangledRestrictThis
            else:
                obj = DemangledRestrict
        elif string.peek() == "V":
            if member_func:
                obj = DemangledVolatileThis
            else:
                obj = DemangledVolatile
        else:
            if member_func:
                obj = DemangledConstThis
            else:
                obj = DemangledConst

        string.advance()
        tmp = obj(None)

        if start == None:
            start = tmp
            end = tmp
        else:
            end.left = tmp
            end = end.left

    return (start, end)

# <function-type> ::= F [Y] <bare-function-type> E
def demangle_function_type(string):
    if not string.check("F"):
        raise DemangledException("Missing \"F\" in function-type")

    if string.peek() == "Y":
        string.advance()

    ret = demangle_bare_function_type(string, True)

    if not string.check("E"):
        raise DemangledException("Missing \"E\" in function-type")

    return ret

# <bare-function-type> ::= [J] <type>+
def demangle_bare_function_type(string, has_return_type):
    if string.peek() == "J":
        string.advance()
        has_return_type = True

    return_type = None
    tl = None
    ptl = None

    while True:
        if string.peek() in ("", "E"):
            break

        type = demangle_type(string)

        if has_return_type:
            return_type = type
            has_return_type = False
        else:
            tmp = DemangledArgumentList(type, None)

            if tl == None:
                tl = tmp
                ptl = tmp
            else:
                ptl.right = tmp
                ptl = ptl.right

    if tl == None:
        raise DemangledException("Invalid bare-function-type")

    if tl.right == None and isinstance(tl.left, DemangledBuiltinType) and \
            tl.left.display == PrintType.void:
        tl = None

    return DemangledFunctionType(return_type, tl)

# <class-enum-type> ::= <name>
def demangle_class_enum_type(string):
    return demangle_name(string)

# <array-type> ::= A <(positive dimension) number> _ <(element) type>
#              ::= A [<(dimension) expression] _ <(element) type>
def demangle_array_type(string):
    if not string.check("A"):
        raise DemangledException("Missing \"A\" in array-type")

    if string.peek() == "_":
        dim = None
    elif string.peek().isdigit():
        count = 0
        while string.peek(count) != None and string.peek(count).isdigit():
            count += 1

        dim = DemangledName(string.get(count))
    else:
        dim = demangle_expression(string)

    if not string.check("_"):
        raise DemangledException("Missing \"_\" in array-type")

    return DemangledArrayType(dim, demangle_type(string))

# <pointer-to-member-type> ::= M <(class) type> <(member) type>
def demangle_pointer_to_member_type(string):
    if not string.check("M"):
        raise DemangledException("Missing \"M\" in pointer-to-member-type")

    cl = demangle_type(string)

    (start, end) = demangle_CV_qualifiers(string, True)

    end.left = demangle_type(string)

    if start != end and not isinstance(end.left, DemangledFunctionType):
        add_substitution(string, start)

    return DemangledPointerMemoryType(cl, start)

# <template-param> ::= T_
#                  ::= T <(parameter-2 non negative) number> _
def demangle_template_param(string):
    if not string.check("T"):
        raise DemangledException("Missing \"T\" in template-param")
        return None

    if string.peek() == "_":
        param = 0
    else:
        param = demangle_number(string)

        if param < 0:
            raise DemangledException("Negative param in template-param")

        param += 1

    if not string.check("_"):
        raise DemangledException("Missing \"_\" in template-param")
    
    return DemangledTemplateParam(param)

# <template-args> ::= I <template-args>+ E
def demangle_template_args(string):
    hold_last_name = string.last_name

    if not string.check("I"):
        raise DemangledException("Missing \"I\" in template-args")

    al = None
    pal = None
    while True:
        a = demangle_template_arg(string)

        tmp = DemangledTemplateArgumentList(a, None)

        if al == None:
            al = tmp
            pal = tmp
        else:
            pal.right = tmp
            pal = tmp

        if string.peek() == "E":
            string.advance()
            break

    string.last_name = hold_last_name

    return al

# <template-arg> ::= <type>
#                ::= X <expression> E
#                ::= <expr-primary>
def demangle_template_arg(string):
    if string.peek() == "X":
        string.advance()
        ret = demangle_expression(string)

        if not string.check("E"):
            raise DemangledException("Missing \"E\" in template-arg")

        return ret
    elif string.peek() == "L":
        return demangle_expr_primary(string)
    else:
        return demangle_type(string)

# <expression> ::= <(unary) operator-name> <expression>
#              ::= <(binary) operator-name> <expression> <expression>
#              ::= <(trinary) operator-name> <expression> <expression> <expression>
#              ::= st <type>
#              ::= <template-param>
#              ::= sr <type> <unqualified-name>
#              ::= sr <type> <unqualified-name> <template-args>
#              ::= <expr-primary>
def demangle_expression(string):
    if string.peek() == "L":
        return demangle_expr_primary(string)
    elif string.peek() == "T":
        return demangle_template_param(string)
    elif string.peek(length = 2) == "sr":
        string.advance(1)
        type = demangle_type(string)
        name = demangle_unqualified_name(string)
        if string.peek() != "I":
            return DemangledQualifiedName(type, name)
        else:
            return DemangledQualifiedName(type, \
                    DemangledTemplate(name, demangle_template_args(string)))
    else:
        op_d = string.peek(length = 2)
        op = demangle_operator_name(string)
        
        if op == None:
            return None

        if op_d == "st":
            return DemangledUnary(op, demangle_type(string))

        if op.args == 1:
            return DemangledUnary(op, demangle_expression(string))
        elif op.args == 2:
            left = demangle_expression(string)

            return DemangledBinary(op, \
                    DemangledBinaryArguments(left, demangle_expression(string)))
        elif op.args == 3:
            first = demangle_expression(string)
            second = demangle_expression(string)

            return DemangledTrinary(op, \
                    DemangledTrinaryArguments1(first, \
                    DemangledTrinaryArguments2(second, \
                    demangle_expression(string))))
        else:
            return None

# <expr-primary> ::= L <type> <(value) number> E
#                ::= L <type> <(value) float> E
#                ::= L <mangled-name> E
def demangle_expr_primary(string):
    if not string.check("L"):
        raise DemangledException("Missing \"L\" in expr-primary")

    if string.peek() == "_":
        ret = demangle_mangled_name(string)
    else:
        type = demangle_type(string)

        obj = DemangledLiteral

        if string.peek() == "n":
            obj = DemangledLiteralNegative
            string.advance()

        count = 0
        while string.peek(count) != "E":
            if string.peek(count) == "":
                raise DemangledException("End of string in expr-primary")

            count += 1

        ret = obj(type, DemangledName(string.get(count)))

    if not string.check("E"):
        raise DemangledException("Missing \"E\" in expr-primary")

    return ret

# <local-name> ::= Z <(function) encoding> E <(entity) name> [<discriminator>]
#              ::= Z <(function) encoding> E s [<discriminator>]
def demangle_local_name(string):
    if not string.check("Z"):
        raise DemangledException("Missing \"Z\" in local-name")

    function = demangle_encoding(string, False)

    if not string.check("E"):
        raise DemangledException("Missing \"E\" in local-name")

    if string.peek() == "s":
        string.advance()

        demangle_discriminator(string)

        return DemangledLocalName(function, DemangledName("string literal"))
    else:
        name = demangle_name(string)

        demangle_discriminator(string)

        return DemangledLocalName(function, name)

# <discriminator> ::= _ <(non-negative) number>
def demangle_discriminator(string):
    if string.peek() != "_":
        return

    string.advance()
    discrim = demangle_number(string)

    if discrim < 0:
        raise DemangledException("Negative number in discriminator")

def add_substitution(string, demangled):
    string.subs.append(demangled)

# <substitution> ::= S <seq-id> _
#                ::= S_
#                ::= St
#                ::= Sa
#                ::= Sb
#                ::= Ss
#                ::= Si
#                ::= So
#                ::= Sd
def demangle_substitution(string, prefix):
    subs_dict = {
        "t":("std", "std", None),
        "a":("std::allocator", "std::allocator", "allocator"),
        "b":("std::basic_string", "std::basic_string", "basic_string"),
        "s":("std::string", "std::basic_string<char, std::char_traits<char>, std::allocator<char> >", "basic_string"),
        "i":("std::istream", "std::basic_istream<char, std::char_traits<char> >", "basic_istream"),
        "o":("std::ostream", "std::basic_ostream<char, std::char_traits<char> >", "basic_ostream"),
        "d":("std::iostream", "std::basic_iostream<char, std::char_traits<char> >", "basic_iostream")
    }

    if not string.check("S"):
        raise DemangledException("Missing \"S\" in substitution")

    c = string.get()

    if c == "_" or c.isdigit() or c.isupper():
        id = 0

        if c != "_":
            while True:
                if c != "":
                    id = id * 36 + int(c, 36)
                else:
                    raise DemangledException("Out of digits in substitution")

                c = string.get()

                if c == "_":
                    break

            id += 1

        if id >= len(string.subs):
            raise DemangledException("Id out of range of subs in substitution")

        return string.subs[id]
    else:
        # FIXME: by default c++filt has verbose on
        # This probably should be a config variable?
        verbose = True

        if prefix:
            peek = string.peek()

            if peek in ("C", "D"):
                verbose = True

        if subs_dict.has_key(c):
            std_sub = subs_dict[c]

            if std_sub[2] != None:
                string.last_name = std_sub[2]

            if verbose:
                s = std_sub[1]
            else:
                s = std_sub[0]

            return DemangledSubstitution(s)
        else:
            raise DemangledException("Unkown char in substitution")

