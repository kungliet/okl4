/*
 * Copyright (c) 2008 Open Kernel Labs, Inc. (Copyright Holder).
 * All rights reserved.
 *
 * 1. Redistribution and use of OKL4 (Software) in source and binary
 * forms, with or without modification, are permitted provided that the
 * following conditions are met:
 *
 *     (a) Redistributions of source code must retain this clause 1
 *         (including paragraphs (a), (b) and (c)), clause 2 and clause 3
 *         (Licence Terms) and the above copyright notice.
 *
 *     (b) Redistributions in binary form must reproduce the above
 *         copyright notice and the Licence Terms in the documentation and/or
 *         other materials provided with the distribution.
 *
 *     (c) Redistributions in any form must be accompanied by information on
 *         how to obtain complete source code for:
 *        (i) the Software; and
 *        (ii) all accompanying software that uses (or is intended to
 *        use) the Software whether directly or indirectly.  Such source
 *        code must:
 *        (iii) either be included in the distribution or be available
 *        for no more than the cost of distribution plus a nominal fee;
 *        and
 *        (iv) be licensed by each relevant holder of copyright under
 *        either the Licence Terms (with an appropriate copyright notice)
 *        or the terms of a licence which is approved by the Open Source
 *        Initative.  For an executable file, "complete source code"
 *        means the source code for all modules it contains and includes
 *        associated build and other files reasonably required to produce
 *        the executable.
 *
 * 2. THIS SOFTWARE IS PROVIDED ``AS IS'' AND, TO THE EXTENT PERMITTED BY
 * LAW, ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, OR NON-INFRINGEMENT, ARE DISCLAIMED.  WHERE ANY WARRANTY IS
 * IMPLIED AND IS PREVENTED BY LAW FROM BEING DISCLAIMED THEN TO THE
 * EXTENT PERMISSIBLE BY LAW: (A) THE WARRANTY IS READ DOWN IN FAVOUR OF
 * THE COPYRIGHT HOLDER (AND, IN THE CASE OF A PARTICIPANT, THAT
 * PARTICIPANT) AND (B) ANY LIMITATIONS PERMITTED BY LAW (INCLUDING AS TO
 * THE EXTENT OF THE WARRANTY AND THE REMEDIES AVAILABLE IN THE EVENT OF
 * BREACH) ARE DEEMED PART OF THIS LICENCE IN A FORM MOST FAVOURABLE TO
 * THE COPYRIGHT HOLDER (AND, IN THE CASE OF A PARTICIPANT, THAT
 * PARTICIPANT). IN THE LICENCE TERMS, "PARTICIPANT" INCLUDES EVERY
 * PERSON WHO HAS CONTRIBUTED TO THE SOFTWARE OR WHO HAS BEEN INVOLVED IN
 * THE DISTRIBUTION OR DISSEMINATION OF THE SOFTWARE.
 *
 * 3. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR ANY OTHER PARTICIPANT BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __OKL4__PD_DICT_H__
#define __OKL4__PD_DICT_H__

#include <okl4/arch/config.h>
#include <okl4/types.h>
#include <okl4/pd.h>

/**
 *  @file
 *
 *  The pd_dict.h Header.
 *
 *  The pd_dict.h header provides the functionality required for using
 *  the protection domain dictionary interface.
 *
 *  The PD dictionary is used for mapping kernel space identifiers to their
 *  corresponding protection domains.  The PD dictionary object is used
 *  to store such translations.  
 *
 *  The protection domain map object maps OKL4 SpaceIds into protection
 *  domain object pointers. This is required when a pager thread receives 
 *  a pagefault IPC. As OKL4 only provides the SpaceId of the address space
 *  in which the thread faulted, the PD dictionary is used to translate 
 *  this space identifier to a particular protection domain.  
 *
 */

/**
 *  The okl4_pd_dict_attr structure is used to represent the PD
 *  dictionary attribute.  This attribute is used to initialize PD
 *  dictionaries.
 *
 *  There are currently no parameters that need to be encoded into this
 *  attribute.  However, the attribute itself must be initialized before it
 *  is used for initializing PD dictionaries.
 *
 */
struct okl4_pd_dict_attr
{
#if !defined(NDEBUG)
    okl4_word_t magic;
#endif

    /*
     * For future use.
     *
     * Strict C does not allow 0-sized structs, so we add padding.
     */
    char _padding;
};

/**
 *  The okl4_pd_dict structure is used to represent a  PD dictionary
 *  object.  This object is used for translating a kernel address space identifier,
 *  into the corresponding protection domain.
 *
 *  The following operations may be performed on a PD dictionary:
 *
 *  @li Static memory allocation.
 *
 *  @li Dynamic memory allocation.
 *
 *  @li Initialization.
 *
 *  @li Addtion.
 *
 *  @li Removal.
 *
 *  @li Lookup.
 *
 */
struct okl4_pd_dict
{
    /* First protection domain in the list of protection domains
     * in this dictionary. */
    okl4_pd_t *head;
};

/**
 *  The okl4_pd_map_attr_init() function is used to initialize a PD
 *  dictionary attribute object specified by the @a attr argument.  
 *  This function must be invoked on the attribute
 *  before it is used for initializing PD dictionaries.
 *
 */
INLINE void okl4_pd_dict_attr_init(okl4_pd_dict_attr_t *attr);

/**
 *  The okl4_pd_dict_init() function is used to initialize a PD dictionary,
 *  @a pd_dict, with the attribute specified by the  @a attr argument.
 *
 *  This function requires the following arguments:
 *
 *  @param pd_dict
 *    The PD dictionary to be initialized.
 *
 *  @param attr
 *    The attribute to be used in initialization.
 *
 */
void okl4_pd_dict_init(okl4_pd_dict_t *, okl4_pd_dict_attr_t *);

/**
 *  The okl4_pd_dict_add() function is used to add an entry specified 
 *  by the @a pd  argument to the PD dictionary specified by the @a pd_dict
 *  argument.  
 *
 *  This function requires the following arguments:
 *
 *  @param pd_dict
 *    The PD dictionary to add to.
 *
 *  @param pd
 *    The PD entry to add.
 *
 *  This function returns the following error codes:
 *
 *  @param OKL4_OK
 *    The addition was successful.
 *
 *  @param OKL4_IN_USE
 *    The PD entry already exists in the PD dictionary.
 *
 */
int okl4_pd_dict_add(okl4_pd_dict_t *, okl4_pd_t *);

/**
 *  The okl4_pd_dict_remove() function is used to remove an entry specified by the  
 *  @a pd argument from the PD dictionary specified by the @a pd_dict argument.
 *
 *  This function requires the following arguments:
 *
 *  @param pd_dict
 *    The PD dictionary to remove from.
 *
 *  @param pd
 *    The PD entry to remove.
 *
 *  This function returns the following error codes:
 *
 *  @param OKL4_OK
 *    The removal was successful.
 *
 *  @param OKL4_OUT_OF_RANGE
 *    The PD entry does not exist in the PD dictionary.
 *
 */
int okl4_pd_dict_remove(okl4_pd_dict_t *, okl4_pd_t *);

/**
 *  The okl4_pd_dict_lookup() function is used to translate an L4_SpaceId_t
 *  specified by the @a space argument to its corresonding protection 
 *  domain returned using the @a pd argument.
 *
 *  This function requires the following arguments:
 *
 *  @param pd_dict
 *    The PD dictionary to lookup from.
 *
 *  @param space
 *    The OKL4 space identifier to translate.
 *
 *  @param pd
 *    This is an output argument, its contents will
 *    contain the corresponding PD on function return.
 *
 *  This function returns the following error codes:
 *
 *  @param OKL4_OK
 *    The lookup was successful.
 *
 *  @param OKL4_OUT_OF_RANGE
 *    The corresponding PD does not exist in the PD dictionary.
 *
 */
int okl4_pd_dict_lookup(okl4_pd_dict_t *, okl4_kspaceid_t, okl4_pd_t **);

/**
 * 
 */
#define OKL4_PD_DICT_SIZE_ATTR(attr)                                         \
        sizeof(okl4_pd_dict_t)

/**
 *  The OKL4_PD_DICT_SIZE() macro is used to determine the amount of memory
 *  required to dynamically allocatea PD dictionary, if the dictionary is
 *  initialized with the attribute specified by the @a attr argument, in bytes.
 *
 */
#define OKL4_PD_DICT_SIZE sizeof(okl4_pd_dict_t)

/*
 * Inline functions.
 */

INLINE void
okl4_pd_dict_attr_init(okl4_pd_dict_attr_t *attr)
{
    assert(attr != NULL);

    OKL4_SETUP_MAGIC(attr, OKL4_MAGIC_PD_DICT_ATTR);
}


#endif /* __OKL4__PD_MAP_H__ */
