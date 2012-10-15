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

# A tree of classes represent the final demangled string components and also
# implement the printing once constructed

# FIXME: These are an enum basically (actually just #defines in the original C
# code), is there a nicer way to do it?
class PrintType:
    default = 0
    bool = 1
    float = 2
    int = 3
    unsigned = 4
    long = 5
    unsigned_long = 6
    void = 7
    long_long = 8
    unsigned_long_long = 9

class PrintModifier:
    def __init__(self, next, mod, printed, templates):
        self.next = next
        self.mod = mod
        self.printed = printed
        self.templates = templates

    def output(self, string):
        if isinstance(self.mod, DemangledRestrict) or \
                isinstance(self.mod, DemangledRestrictThis):
            string.append(" restrict")
        elif isinstance(self.mod, DemangledVolatile) or \
                isinstance(self.mod, DemangledVolatileThis):
            string.append(" volatile")
        elif isinstance(self.mod, DemangledConst) or \
                isinstance(self.mod, DemangledConstThis):
            string.append(" const")
        elif isinstance(self.mod, DemangledVendorTypeQualified):
            string.append(" ")
            self.mod.right.output(string)
        elif isinstance(self.mod, DemangledPointer):
            string.append("*")
        elif isinstance(self.mod, DemangledReference):
            string.append("&")
        elif isinstance(self.mod, DemangledComplex):
            string.append("complex ")
        elif isinstance(self.mod, DemangledImaginary):
            string.append("imaginary ")
        elif isinstance(self.mod, DemangledPointerMemoryType):
            if string.last_char != "(":
                string.append(" ")

            self.mod.left.output(string)
            string.append("::")
        elif isinstance(self.mod, DemangledTypedName):
            self.mod.left.output(string)
        else:
            self.mod.output(string)

def modifier_output_list(string, mods, suffix):
    if mods == None:
        return

    if mods.printed or (not suffix and \
            (isinstance(mods.mod, DemangledRestrictThis) or \
            isinstance(mods.mod, DemangledVolatileThis) or \
            isinstance(mods.mod, DemangledConstThis))):
        modifier_output_list(string, mods.next, suffix)

        return

    mods.printed = True

    hold_dpt = DemangledComponent.templates
    DemangledComponent.templates = mods.templates

    if isinstance(mods.mod, DemangledFunctionType):
        mods.mod.output_function(string, mods.next)
        DemangledComponent.templates = hold_dpt
    elif isinstance(mods.mod, DemangledArrayType):
        mods.mod.output_array(string, mods.next)
        DemangledComponent.templates = hold_dpt
    elif isinstance(mods.mod, DemangledLocalName):
        hold_modifiers = DemangledComponent.modifiers
        DemangledComponent.modifiers = None

        mods.mod.output(string)

        DemangledComponent.modifiers = hold_modifiers

        string.append("::")

        dc = mods.mod.right

        while isinstance(dc, DemangledRestrictThis) or \
                isinstance(dc, DemangledVolatileThis) or \
                isinstance(dc, DemangledConstThis):
            dc = dc.left

        dc.output(string)
        DemangledComponent.templates = hold_dpt

        return

    mods.output(string)
    DemangledComponent.templates = hold_dpt

    modifier_output_list(string, mods.next, suffix)

class PrintTemplate:
    def __init__(self, next, template_decl):
        self.next = next
        self.template_decl = template_decl

class DemangledComponent:
    modifiers = None
    templates = None

# FIXME: Better name?
class DemangledHalfTree(DemangledComponent):
    def __init__(self, left):
        self.left = left

class DemangledTree(DemangledComponent):
    def __init__(self, left, right):
        self.left = left
        self.right = right

class DemangledModifier(DemangledHalfTree):
    def output(self, string):
        if isinstance(self, DemangledRestrict) or \
                isinstance(self, DemangledVolatile) or \
                isinstance(self, DemangledConst):
            pdqm = DemangledComponent.modifiers

            while pdqm != None:
                if not pdqm.printed:
                    if not (isinstance(pdqm.mod, DemangledRestrict) or \
                            isinstance(pdqm.mod, DemangledVolatile) or \
                            isinstance(pdqm.mod, DemangledConst)):
                        break

                    if isinstance(pdqm.mod, self):
                        self.left.output(string)

                pdqm = pdqm.next

        dpm = PrintModifier(DemangledComponent.modifiers, self, False, \
                DemangledComponent.templates)
        DemangledComponent.modifiers = dpm

        self.left.output(string)

        if not dpm.printed:
            dpm.output(string)

        DemangledComponent.modifiers = dpm.next

class DemangledName(DemangledComponent):
    def __init__(self, name):
        self.name = name

    def output(self, string):
        string.append(self.name)

class DemangledQualifiedName(DemangledTree):
    def output(self, string):
        self.left.output(string)
        string.append("::")
        self.right.output(string)

class DemangledLocalName(DemangledTree):
    def output(self, string):
        self.left.output(string)
        string.append("::")
        self.right.output(string)

class DemangledTypedName(DemangledTree):
    def output(self, string):
        hold_modifiers = DemangledComponent.modifiers
        
        typed_name = self.left
        adpm = []
        i = 0

        while typed_name != None:
            adpm.append(PrintModifier(DemangledComponent.modifiers, \
                    typed_name, False, DemangledComponent.templates))
            DemangledComponent.modifiers = adpm[i]
            i += 1

            if not (isinstance(typed_name, DemangledRestrictThis) or \
                    isinstance(typed_name, DemangledVolatileThis) or \
                    isinstance(typed_name, DemangledConstThis)):
                break

            typed_name = typed_name.left

        if typed_name == None:
            raise Exception("typed_name == None")

        if isinstance(typed_name, DemangledTemplate):
            dpt = PrintTemplate(DemangledComponent.templates, typed_name)
            DemangledComponent.templates = dpt

        if isinstance(typed_name, DemangledLocalName):
            local_name = typed_name.right

            while isinstance(local_name, DemangledRestrictThis) or \
                    isinstance(local_name, DemangledVolatileThis) or \
                    isinstance(local_name, DemangledConstThis):
    
                adpm.append(PrintModifier(adpm[i - 1].next, adpm[i - 1].mod, \
                        adpm[i - 1].printed, adpm[i - 1].templates))
                adpm[i].next = adpm[i - 1]
                DemangledComponent.modifiers = adpm[i]
                adpm[i - 1].mod = local_name
                adpm[i - 1].printed = False
                adpm[i - 1].templates = DemangledComponent.templates
                i += 1

                local_name = local_name.left

        self.right.output(string)

        if isinstance(typed_name, DemangledTemplate):
            DemangledComponent.templates = dpt.next

        while i > 0:
            i -= 1

            if not adpm[i].printed:
                string.append(" ")
                adpm[i].output(string)
        
        DemangledComponent.modifiers = hold_modifiers

class DemangledTemplate(DemangledTree):
    def output(self, string):
        hold_mods = DemangledComponent.modifiers
        DemangledComponent.modifiers = None

        self.left.output(string)

        if string.last_char == "<":
            string.append(" ")

        string.append("<")
        self.right.output(string)

        if string.last_char == ">":
            string.append(" ")

        string.append(">")

        DemangledComponent.modifiers = hold_mods

class DemangledTemplateParam(DemangledComponent):
    def __init__(self, param):
        self.param = param

    def output(self, string):
        if DemangledComponent.templates == None:
            raise Exception("no templates")

        i = self.param
        a = DemangledComponent.templates.template_decl.right
        while a != None:
            if not isinstance(a, DemangledTemplateArgumentList):
                raise Exception("not template arglist")

            if i <= 0:
                break
            i -= 1

            a = a.right

        if i != 0 or a == None:
            raise Exception("not enough template args")

        hold_dpt = DemangledComponent.templates
        DemangledComponent.templates = hold_dpt.next

        a.left.output(string)

        DemangledComponent.templates = hold_dpt

class DemangledCTOR(DemangledComponent):
    def __init__(self, name):
        self.name = name

    def output(self, string):
        string.append(self.name)

# FIXME: We don't seem to actually care about the type of CTOR or DTOR, can we
# simply not use them at all?
class DemangledCTORBaseObject(DemangledCTOR):
    pass

class DemangledCTORCompleteObject(DemangledCTOR):
    pass

class DemangledCTORCompleteObjectAllocating(DemangledCTOR):
    pass

class DemangledDTOR(DemangledComponent):
    def __init__(self, name):
        self.name = name

    def output(self, string):
        string.append("~")
        string.append(self.name)

class DemangledDTORDeleting(DemangledDTOR):
    pass

class DemangledDTORCompleteObject(DemangledDTOR):
    pass

class DemangledDTORBaseObject(DemangledDTOR):
    pass

class DemangledVTable(DemangledHalfTree):
    def output(self, string):
        string.append("vtable for ")
        self.left.output(string)

class DemangledVTT(DemangledHalfTree):
    def output(self, string):
        string.append("VTT for ")
        self.left.output(string)

class DemangledConstructionVTable(DemangledTree):
    def output(self, string):
        string.append("construction vtable for ")
        self.left.output(string)
        string.append("-in-")
        self.right.output(string)

class DemangledTypeInfo(DemangledHalfTree):
    def output(self, string):
        string.append("typeinfo for ")
        self.left.output(string)

class DemangledTypeInfoName(DemangledHalfTree):
    def output(self, string):
        string.append("typeinfo name for ")
        self.left.output(string)

class DemangledTypeInfoFunction(DemangledHalfTree):
    def output(self, string):
        string.append("typeinfo fn for ")
        self.left.output(string)

class DemangledThunk(DemangledHalfTree):
    def output(self, string):
        string.append("non-virtual thunk to ")
        self.left.output(string)

class DemangledVirtualThunk(DemangledHalfTree):
    def output(self, string):
        string.append("virtual thunk to ")
        self.left.output(string)

class DemangledCovariantThunk(DemangledHalfTree):
    def output(self, string):
        string.append("covariant thunk to ")
        self.left.output(string)

class DemangledGuard(DemangledHalfTree):
    def output(self, string):
        string.append("guard variable for ")
        self.left.output(string)

class DemangledRefTemp(DemangledHalfTree):
    def output(self, string):
        string.append("reference temporary for ")
        self.left.output(string)

class DemangledHiddenAlias(DemangledHalfTree):
    def output(self, string):
        string.append("hidden alias for ")
        self.left.output(string)

class DemangledSubstitution(DemangledComponent):
    def __init__(self, name):
        self.name = name

    def output(self, string):
        string.append(self.name)

class DemangledRestrict(DemangledModifier):
    pass

class DemangledVolatile(DemangledModifier):
    pass

class DemangledConst(DemangledModifier):
    pass

class DemangledRestrictThis(DemangledModifier):
    pass

class DemangledVolatileThis(DemangledModifier):
    pass

class DemangledConstThis(DemangledModifier):
    pass

class DemangledVendorTypeQualified(DemangledModifier):
    pass

class DemangledPointer(DemangledModifier):
    pass

class DemangledReference(DemangledModifier):
    pass

class DemangledComplex(DemangledModifier):
    pass

class DemangledImaginary(DemangledModifier):
    pass

class DemangledBuiltinType(DemangledComponent):
    def __init__(self, (name, display)):
        self.name = name
        self.display = display

    def output(self, string):
        string.append(self.name)

class DemangledVendorType(DemangledHalfTree):
    def outpu(self, string):
        self.left.output(string)

class DemangledFunctionType(DemangledTree):
    def output(self, string):
        if self.left != None:
            dpm = PrintModifier(DemangledComponent.modifiers, self, False, \
                    DemangledComponent.templates)
            DemangledComponent.modifiers = dpm

            self.left.output(string)

            DemangledComponent.modifiers = dpm.next

            if dpm.printed:
                return

            string.append(" ")

        self.output_function(string, DemangledComponent.modifiers)

    def output_function(self, string, modifiers):
        need_paren = False
        saw_mod = False
        need_space = False

        p = modifiers
        while p != None:
            if p.printed:
                break

            saw_mod = True
            
            if isinstance(p.mod, DemangledPointer) or \
                    isinstance(p.mod, DemangledReference):
                need_paren = True
            elif isinstance(p.mod, DemangledRestrict) or \
                    isinstance(p.mod, DemangledVolatile) or \
                    isinstance(p.mod, DemangledConst) or \
                    isinstance(p.mod, DemangledVendorTypeQualified) or \
                    isinstance(p.mod, DemangledComplex) or \
                    isinstance(p.mod, DemangledImaginary) or \
                    isinstance(p.mod, DemangledPointerMemoryType):
                need_space = True
                need_paren = True
            else:
                break

            if need_paren:
                break

            p = p.next

        if self.left != None and not saw_mod:
            need_paren = True

        if need_paren:
            if not need_space:
                if string.last_char != "(" and string.last_char != "*":
                    need_space = True

            if need_space and string.last_char != " ":
                string.append(" ")

            string.append("(")

        hold_modifiers = DemangledComponent.modifiers
        DemangledComponent.modifiers = None

        modifier_output_list(string, modifiers, False)

        if need_paren:
            string.append(")")

        string.append("(")

        if self.right != None:
            self.right.output(string)

        string.append(")")

        modifier_output_list(string, modifiers, True)

        DemangledComponent.modifiers = hold_modifiers

class DemangledArrayType(DemangledTree):
    def output(self, string):
        hold_modifiers = DemangledComponent.modifiers

        adpm = []

        adpm.append(PrintModifier(hold_modifiers, self, False, \
                DemangledComponent.templates))
        DemangledComponent.modifiers = adpm[0]

        i = 1
        pdpm = hold_modifiers

        while pdpm != None and (isinstance(pdpm.mod, DemangledRestrict) or \
                isinstance(pdpm.mod, DemangledVolatile) or \
                isinstance(pdpm.mod, DemangledConst)):
            if not pdpm.printed:
                adpm.append(pdpm)
                adpm[i].next = DemangledComponent.modifiers
                DemangledComponent.modifiers = adpm[i]
                pdpm.printed = True
                i += 1

            pdpm = pdpm.next

        self.right.output(self)

        DemangledComponent.modifiers = hold_modifiers

        if adpm[0].printed:
            return

        while i > 1:
            i -= 1
            adpm[i].output(string)

        self.output_array(string, DemangledComponent.modifiers)

    def output_array(self, string, modifiers):
        need_space = True

        if modifiers != None:
            need_paren = False
            p = modifiers

            while p != None:
                if not p.printed:
                    if isinstance(p.mod, DemangledArrayType):
                        need_space = False
                        break
                    else:
                        need_paren = True
                        need_space = True
                        break

                p = p.next

            if need_paren:
                string.append(" (")

            modifier_output_list(string, modifiers, False)

            if need_paren:
                string.append(")")

            if need_space:
                string.append(" ")

            string.append("[")

            if self.left != None:
                self.left.output(string)

            string.append("]")

class DemangledPointerMemoryType(DemangledTree):
    def output(self, string):
        dpm = PrintModifier(DemangledComponent.modifiers, self, False, \
                DemangledComponent.templates)
        DemangledComponent.modifiers = dpm

        self.right.output(string)

        if not dpm.printed:
            string.append(" ")
            self.left.output(string)
            string.append("::")

        DemangledComponent.modifiers = dpm.next

class DemangledArgumentList(DemangledTree):
    def output(self, string):
        self.left.output(string)
        if self.right != None:
            string.append(", ")
            self.right.output(string)

class DemangledTemplateArgumentList(DemangledTree):
    def output(self, string):
        self.left.output(string)
        if self.right != None:
            string.append(", ")
            self.right.output(string)

class DemangledOperator(DemangledComponent):
    def __init__(self, name, args):
        self.name = name
        self.args = args

    def output(self, string):
        string.append("operator")

        if self.name[0].islower():
            string.append(" ")

        string.append(self.name)

class DemangledExtendedOperator(DemangledOperator):
    def output(self, string):
        string.append("operator " + self.name)

class DemangledCast(DemangledTree):
    def output(self, string):
        string.append("operator ")
        self.output_cast(string)

    def output_cast(self, string):
        if not isinstance(self.left, DemangledTemplate):
            self.left.output(string)
        else:
            hold_dpm = DemangledComponent.modifiers
            DemangledComponent.modifiers = None

            dpt = PrintTemplate(DemangledComponent.templates, self.left)
            DemangledComponent.templates = dpt

            self.left.left.output(string)

            DemangledComponent.templates = dpt.next

            if string.last_char == "<":
                string.append(" ")
            
            string.append("<")

            self.left.right.output(string)

            if string.last_char == ">":
                string.append(" ")

            string.append(">")

            DemangledComponent.modifiers = hold_dpm

def output_expr_op(string, comp):
    if isinstance(comp, DemangledOperator):
        string.append(comp.name)
    else:
        comp.output(string)

class DemangledUnary(DemangledTree):
    def output(self, string):
        if isinstance(self.left, DemangledCast):
            self.left.output_expr_op(string)
        else:
            string.append("(")
            self.left.output_cast(string)
            string.append(")")

        string.append("(")
        self.right.output(string)
        string.append(")")

class DemangledBinary(DemangledTree):
    def output(self, string):
        if isinstance(self.right, DemangledBinaryArguments):
            raise Exception("No binary args")

        if isinstance(self.left, DemangledOperator) and self.left.name == ">":
            string.append("(")

        string.append("(")
        self.right.left.output(string)
        string.append(") ")
        output_expr_op(string, self.left)
        string.append(" (")
        self.right.right.output(string)
        string.append(")")

        if isinstance(self.left, DemangledOperator) and self.left.name == ">":
            string.append(")")

class DemangledBinaryArguments(DemangledTree):
    pass

class DemangledTrinary(DemangledTree):
    def output(self, string):
        if not (isinstance(self.right, DemangledTrinaryArguments1) or \
                isinstance(self.right.right, DemangledTrinaryArguments2)):
            raise Exception("Invalid trinary args")

        string.append("(")
        self.right.left.output(string)
        string.append(") ")
        output_expr_op(self.left)
        string.append(" (")
        self.right.right.left.output(string)
        string.append(") : (")
        self.right.right.right.output(string)
        string.append(")")

class DemangledTrinaryArguments1(DemangledTree):
    pass

class DemangledTrinaryArguments2(DemangledTree):
    pass

class DemangledLiteral(DemangledTree):
    def output(self, string):
        tp = PrintType.default

        if isinstance(self.left, DemangledBuiltinType):
            tp = self.left.display

            if tp == PrintType.int or tp == PrintType.unsigned or \
                    tp == PrintType.long or \
                    tp == PrintType.unsigned_long or \
                    tp == PrintType.long_long or \
                    tp == PrintType.unsigned_long_long:
                if isinstance(self.right, DemangledName):
                    if isinstance(self, DemangledLiteralNegative):
                        string.append("-")

                    self.right.output(string)

                    if tp == PrintType.unsigned:
                        string.append("u")
                    elif tp == PrintType.long:
                        string.append("l")
                    elif tp == PrintType.unsigned_long:
                        string.append("ul")
                    elif tp == PrintType.long_long:
                        string.append("ll")
                    elif tp == PrintType.unsigned_long_long:
                        string.append("ull")

                    return
            elif tp == PrintType.bool:
                if isinstance(self.right, DemangledName) and \
                        isinstance(self, DemangledLiteral):
                    if self.right.name[0] == "0":
                        string.append("false")
                        return
                    elif self.right.name[0] == "1":
                        string.append("true")
                        return

        string.append("(")
        self.left.output(string)
        string.append(")")

        if isinstance(self, DemangledLiteralNegative):
            string.append("-")

        if tp == PrintType.float:
            string.append("[")

        self.right.output(string)

        if tp == PrintType.float:
            string.append("]")
        
class DemangledLiteralNegative(DemangledLiteral):
    pass

