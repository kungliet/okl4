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
Common relocation functions

Relocation functions should just return the calculated value (not write it
back into the data) if successful otherwise they can return None to indicate
that they were unable to process the relocation (this is the default behaviour
if no function is provided for relocation)
"""

def reloc_get_symbol_value(symtab, symdx):
    """
    Return the value of a symbol used in a relocation.
    """
    return symtab.symbols[symdx].value

def reloc_get_addressing_origin_of_symbol(symtab, symdx):
    """
    Return B(S), the addressing origin of the section pointed to by the symbol.
    """
    return symtab.symbols[symdx].section.address

def reloc_get_got_address_of_symbol(elf, symtab, symdx):
    """
    Return the address of this symbols entry in the got table (absolute).
    """
    sym = symtab.symbols[symdx]

    got_org = elf.find_section_named('.got').address

    return got_org + sym.got_offset

def reloc_get_position(sect, offset):
    """
    Return the address of the instruction being relocated.
    """
    return sect.address + offset

def reloc_info(elf, sect, symtab, addend, offset, symdx):
    """
    Debugger function that simply prints out info about the relocation.
    """
    print "Applying reloc to section", sect
    print "at offset %x" % offset
    print "using symbol", symtab.symbols[symdx]
    print "and addend %x" % addend

    return None

def reloc_alloc_got_symbol(elf, sect, symtab, offset, symdx):
    """
    If this symbol has no entry in the .got table add one
    """
    sym = symtab.symbols[symdx]

    elf.add_got_entry(sym)

def reloc_calc_none(elf, sect, symtab, addend, offset, symdx):
    """
    This lets a reloc that does not perform any actions be processed without
    raising an unimplemented error.
    """
    return True

def reloc_calc_S_add_A(elf, sect, symtab, addend, offset, symdx):
    """
    Implement reloc
        S + A
    """
    sym_val = reloc_get_symbol_value(symtab, symdx)

    return sym_val + addend

def reloc_calc_S_add_A_sub_P(elf, sect, symtab, addend, offset, symdx):
    """
    Implement reloc
        S + A - P
    """
    sym_val = reloc_get_symbol_value(symtab, symdx)

    pos = reloc_get_position(sect, offset)

    return sym_val + addend - pos

def reloc_calc_B_S_add_A_sub_P(elf, sect, symtab, addend, offset, symdx):
    """
    Immplement reloc
        B(S) + A - P
    """
    ao_sym = reloc_get_addressing_origin_of_symbol(symtab, symdx)

    pos = reloc_get_position(sect, offset)

    return ao_sym + addend - pos

def reloc_calc_GOT_S_add_A_sub_GOT_ORG(elf, sect, symtab, addend, offset, symdx):
    """
    Implement reloc
        GOT(S) + A - GOT_ORG
    """
    ga_sym = reloc_get_got_address_of_symbol(elf, symtab, symdx)

    got_org = elf.find_section_named('.got').address

    return ga_sym + addend - got_org

def reloc_calc_S_add_A_sub_GOT_ORG(elf, sect, symtab, addend, offst, symdx):
    """
    Implement reloc
        S + A - GOT_ORG
    """
    sym_val = reloc_get_symbol_value(symtab, symdx)

    got_org = elf.find_section_named('.got').address

    return sym_val + addend - got_org
