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

#ifndef __OKL4__KCLIST_H__
#define __OKL4__KCLIST_H__

#include <okl4/kclistid_pool.h>
#include <okl4/kspaceid_pool.h>
#include <l4/caps.h>

#define OKL4_DEFAULT_KCLIST_SIZE  256

/**
 *  @file
 *
 *  The kclist.h Header.
 *
 *  The kclist.h header provides the functionality required for 
 *  using a capability list also known as a  @a kclist and its associated components.
 *
 *  This interface provides support for two types of allocation:
 *
 *  @li [a{)}] Allocation of kclist ids to cell managers.
 *
 *  @li [b{)}] Allocation of caps out of clists to threads.
 *
 *  Capability lists are utilized by creating kclist with a @a kclist 
 *  Allocation and deallocation operations may then be called on the kclist, 
 *  where the parameters are encoded as @a kcap item objects.
 *  The kclist uses the bitmap allocator as the underlying strategy for
 *  allocating caps.
 *
 */

/**
 *  The okl4_kclist_attr type is used to represent a kclist attribute
 *  object used for creating a kclist. As kclists are not a purely 
 *  user-level construct,  a kernel system call is always invoked to 
 *  create a clist within the kernel.  
 *
 *  Two parameters are required to create a kclist, a valid kclist id and 
 *  and the number of capability slots contained in the kclist. As each cell can only
 *  maintain a fixed quota of kclists, the kclist id
 *  is used to keep track of clist usage in the kernel.  Valid clist ids
 *  are obtained from the kclist id allocator. Once these attributes are 
 *  encoded into a okl4_kclist_attr, it may then be used to create a kclist. 
 *
 */
struct okl4_kclist_attr {
#if !defined(NDEBUG)
    okl4_word_t magic;
#endif
    okl4_kclistid_t id;
    okl4_word_t num_cap_slots;
    /* Provides the start and end of the kclist */
    okl4_allocator_attr_t bitmap_attr;
};

/**
 *  The okl4_kcap_item structure is used to represent a capability and 
 *  serves as a wrapper around a kernel capability object.
 *  The only operation permitted on this object is
 *  to translate it to a @a kernel @a capability.
 *
 */
struct okl4_kcap_item {
    okl4_kcap_t kcap;
};

/**
 *  The okl4_kclist structure is used to represent the capability
 *  list or @a kclist.  It serves the following purposes:
 *
 *  @li A wrapper around kernel clists, user-level operations
 *  on kernel clists are proxied via this libokl4 kclist interface.
 *
 *  @li Contains an allocator for the allocation of capabilities from the kclist, tracking
 *  the allocation of capslots.
 *
 *  @li Maintains a list of kernel spaces attached to a particular
 *  kernel clist.
 *
 *  The following operations can be performed on a kclist:
 *
 *  @li Static memory allocation.
 *
 *  @li Dynamic memory allocation.
 *
 *  @li Creation and deletion.
 *
 *  @li Allocation and free.
 *
 */
struct okl4_kclist {
    okl4_kclistid_t id;
    okl4_word_t num_cap_slots;
    okl4_kspace_t * kspace_list;
    okl4_bitmap_allocator_t kcap_allocator;
};


/*
 * Static Memory Allocators.
 */

/**
 *  The OKL4_KCLIST_SIZE() macro is used to determine the amount of memory
 *  required to dynamically allocate a kclist, if it is initialized
 *  with the @a attr attribute, in bytes.
 *
 */
#define OKL4_KCLIST_SIZE(size)                                                 \
    ((sizeof(struct okl4_kclist) - sizeof(struct okl4_bitmap_allocator)) +     \
    OKL4_BITMAP_ALLOCATOR_SIZE(size))

/**
 *  The OKL4_KCLIST_DECLARE_OFRANGE() macro is used to declare and
 *  statically allocate memory for a kclist containing a @a size number 
 *  of cap slots, starting from @a base. The @a name argument is used by
 *  this function to return the an @a uninitialized kclist with the appripriate
 *  amount of memory.
 *
 *  This function requires the following arguments:
 *
 *  @param name
 *    The name of the kclist.
 *
 *  @param base
 *    The value of the first cap slot in the kclist.
 *
 *  @param size
 *    The number of cap slots in the kclist.
 *
 *  This macro only handles memory allocation. The initialization function
 *  must be called separately.
 *
 */
#define OKL4_KCLIST_DECLARE_OFRANGE(name, base, size)                          \
    okl4_word_t __##name[OKL4_KCLIST_SIZE(size)];                              \
    static okl4_kclist_t *(name) = (okl4_kclist_t *)(void *)__##name

/**
 *  The OKL4_KCLIST_DECLARE_OFSIZE() macro is used to declare and statically
 *  allocate memory for a kclist containing a @a
 *  size number of cap slots. The @a name argument is used by
 *  this function to return the an @a uninitialized kclist with the appripriate
 *  amount of memory.

 *
 *  This function requires the following arguments:
 *
 *  @param name
 *    Variable name of the kclist.
 *
 *  @param size
 *    The number of cap slots in the kclist.
 *
 *  This macro only handles memory allocation. The initialization function
 *  must be called separately.
 *
 */
#define OKL4_KCLIST_DECLARE_OFSIZE(name, size) \
    OKL4_KCLIST_DECLARE_OFRANGE(name, 0, size)

/*
 * Dynamic Memory Allocators.
 */

/**
 *  The OKL4_KCLIST_SIZE_ATTR() macro is used to
 *  determine the amount of memory required to dynamically allocate
 *  a kclist, if it is initialized with the
 *  @a attr attribute. This macro returns the requisite amount 
 *  of memory in bytes. 
 *
 *  @param attr
 *      Attribute to be used to initialize the kclist.
 *
 */
#define OKL4_KCLIST_SIZE_ATTR(attr) \
    OKL4_KCLIST_SIZE((attr)->bitmap_attr.size)

/* Allocator Interface */

/**
 *  The okl4_kclist_create() function is used to create a @a kclist with
 *  the @a attr attribute. This function expects the @a attr attribute to 
 *  encode a valid kclist id and the range of cap slots to be made available 
 *  in the kclist specified by the @a clist argument.  This
 *  function invokes the L4_CreateClist() system call to create a kernel
 *  clist with the parameters encoded in the @a attr attribute.
 *
 *  This function requires the following arguments:
 *
 *  @param kclist
 *    The kclist to be created. Its contents will reference the newly created
 *    kernel clist on function return.
 *
 *  @param attr
 *    The attributes of the kclist.
 *
 *  This function returns the following error conditions:
 *
 *  @param OKL4_OK
 *    Creation was successful.
 *
 *  @param other
 *    Return value from L4_CreateClist().
 *
 *  Please refer to the @a OKL4 @a Reference @a Manual for a more detailed explanation of 
 *  the L4_CreateClist() system call.
 *
 */
int okl4_kclist_create(okl4_kclist_t *kclist, okl4_kclist_attr_t *attr);

/**
 *  The okl4_kclist_delete() function is used to delete an existing 
 *  kclist specified by the @a kclist argument.  This
 *  function invokes the L4_DeleteClist() system call to delete the corresponding kernel
 *  clist.  The specified kclist cannot have any attached kspaces.
 *
 *  This function returns the following error codes:
 *
 *  @param OKL4_OK
 *    Deletion is successful.
 *
 *  @param OKL4_IN_USE
 *    There are one or more spaces currently attached to the kclist.
 *
 *  @param other
 *    Return value from L4_DeleteClist().
 *
 *  Please refer to the @a OKL4 @a Reference @a Manual for a more detailed explanation of 
 *  the L4_DeleteClist() system call.
 *
 */
void okl4_kclist_delete(okl4_kclist_t *kclist);

/**
 *  The okl4_kclist_kcap_allocany() function is used to allocate any capability
 *  from any cap slot in the kclist specified by the @a kclist argument.  
 *  The allocated cap is returned using the @a item argument.
 *
 *  This function requires the following arguments:
 *
 *  @param kclist
 *    kclist from which to allocate the capability.
 *
 *  @param item
 *    Capability containing the allocated capability on function
 *    return.
 *
 *  This function returns the following error conditions:
 *
 *  @param OKL4_OK
 *    Allocation was successful.
 *
 *  @param other
 *    Return value from okl4_bitmap_allocator_alloc () function .
 *
 */
int okl4_kclist_kcap_allocany(okl4_kclist_t *kclist,
        okl4_kcap_item_t *item);

/**
 *  The okl4_kclist_kcap_copy() function is used to copy a capability from one
 *  kclist to another.
 *
 *  This function requires the following arguments:
 *
 *  @param src_kclist
 *    The kclist from which the capability is copied.
 *
 *  @param src_kcap
 *    The capability to be copied.
 *
 *  @param dest_kclist
 *    The kclist into which the capability will be copied.
 *
 *  @param dest_kcap
 *    The  capability to be copy to.
 *
 */
int okl4_kclist_kcap_copy(okl4_kclist_t *src_kclist, okl4_kcap_item_t *src_kcap,
        okl4_kclist_t *dest_kclist, okl4_kcap_item_t *dest_kcap);

/**
 *  The okl4_kclist_kcap_remove() function is used to remove a capability from a
 *  kclist. 
 *
 *  This function requires the following arguments:
 *
 *  @param kclist
 *    The kclist from which the capability is to be removed.
 *
 *  @param kcap
 *    The capability to be removed.
 *
 */
void okl4_kclist_kcap_remove(okl4_kclist_t *kclist, okl4_kcap_item_t *kcap);

/**
 *  The okl4_kclist_kcap_allocunit() function is used to allocate a capability
 *  from the capability slot specified by the @a unit argument in the kclist
 *  specified by the @a kclist argument. The @a item argument is used to return 
 *  allocated capability.
 *
 *  This function requires the following arguments:
 *
 *  @param kclist
 *    The kclist from which the the capability is to be allocated.
 *
 *  @param item
 *    Capability containing the allocated capability on function
 *    return.
 *
 *  @param unit
 *    The capability number requested.
 *
 *  This function returns the following error conditions:
 *
 *  @param OKL4_OK
 *    Allocation was successful.
 *
 *  @param other
 *    Return value from the okl4_bitmap_allocator_alloc () function.
 *
 */
int okl4_kclist_kcap_allocunit(okl4_kclist_t * kclist,
        okl4_kcap_item_t *item, okl4_word_t unit);

/**
 *  The okl4_kclist_kcap_free() function is used to deallocate a capability 
 *  specified by the @a item argument from the kclist specified by the
 *  @a kclist argument.
 *
 *  This function requires the following arguments:
 *
 *  @param kclist
 *    kclist from which the capability is to be deallocated.
 *
 *  @param item
 *    Capability to be deallocated.
 *
 */
void okl4_kclist_kcap_free(okl4_kclist_t * kclist,
        okl4_kcap_item_t *item);

/**
 *  The okl4_kclist_attr_setrange() function is used to encode a kclist
 *  attribute @a attr to creating a kclist with @a size cap slots, starting
 *  from @a base.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    The attribute to encode. Its contents will reflect the new encoding
 *    after the function returns.
 *
 *  @param base
 *    The value of the first cap slot in the kclist.
 *
 *  @param size
 *    The number of cap slots in the kclist.
 *
 */
INLINE void okl4_kclist_attr_setrange(okl4_kclist_attr_t * attr,
        okl4_word_t base, okl4_word_t size);

/**
 *  The okl4_kclist_attr_setsize() function is used to encode a kclist
 *  attribute @a attr to create a kclist with @a size cap slots.  The value
 *  of the first cap slot is unspecified.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    The attribute to encode. Its contents will reflect the new encoding
 *    after the function returns.
 *
 *  @param size
 *    The number of cap slots in the kclist.
 *
 */
INLINE void okl4_kclist_attr_setsize(okl4_kclist_attr_t * attr,
        okl4_word_t size);

/**
 *  The okl4_kclist_attr_setid() function is used to encode a kclist
 *  attribute specified by the @a attr argument with the kclist identifier
 *  specified by the @a kclistid argument. Creating a kclist with this
 *  attribute will consume the @a kclistid clist slot in the kernel, if it
 *  is available to the calling cell.
 *
 *  This function requires the following arguments:
 *
 *  @param attr
 *    Attribute to be encoded. 
 *
 *  @param kclistid
 *    kclist id to be encode in the attribute.
 *
 */
INLINE void okl4_kclist_attr_setid(okl4_kclist_attr_t * attr,
        okl4_kclistid_t kclistid);

/**
 *  The okl4_kcap_item_getkcap() function is used to obtain the kernel 
 *  capability contained in the kcap item structure specified by the 
 *  @a item argument.
 *
 */
INLINE okl4_kcap_t okl4_kcap_item_getkcap(okl4_kcap_item_t *item);

/**
 *  The okl4_kclist_getid() function is used to obtain the kclist id from the
 *  kclist structure specified by the @a kclist argument.
 *
 */
INLINE okl4_kclistid_t okl4_kclist_getid(okl4_kclist_t *kclist);

/**
 *  The okl4_kclist_attr_init() function is used to initialize a kclist
 *  attribute specified by the @a attr argument to default values.
 *
 */
INLINE void okl4_kclist_attr_init(okl4_kclist_attr_t * attr);

/*
 * Inlined methods
 */

INLINE void
okl4_kclist_attr_init(okl4_kclist_attr_t * attr)
{
    assert(attr != NULL);

    OKL4_SETUP_MAGIC(attr, OKL4_MAGIC_KCLIST_ATTR);
    attr->id.clist_no = 0;
    attr->num_cap_slots = OKL4_DEFAULT_KCLIST_SIZE;
    okl4_allocator_attr_init(&attr->bitmap_attr);
    okl4_allocator_attr_setsize(&attr->bitmap_attr, OKL4_DEFAULT_KCLIST_SIZE);
}

INLINE void
okl4_kclist_attr_setrange(okl4_kclist_attr_t * attr,
        okl4_word_t base, okl4_word_t size)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_KCLIST_ATTR);

    attr->num_cap_slots = size;
    okl4_allocator_attr_setrange(&attr->bitmap_attr, base, size);
}

INLINE void
okl4_kclist_attr_setsize(okl4_kclist_attr_t * attr,
        okl4_word_t size)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_KCLIST_ATTR);

    attr->num_cap_slots = size;
    okl4_allocator_attr_setsize(&attr->bitmap_attr, size);
}

INLINE void
okl4_kclist_attr_setid(okl4_kclist_attr_t * attr,
        okl4_kclistid_t kclistid)
{
    assert(attr != NULL);
    OKL4_CHECK_MAGIC(attr, OKL4_MAGIC_KCLIST_ATTR);

    attr->id = kclistid;
}

INLINE okl4_kclistid_t
okl4_kclist_getid(okl4_kclist_t *kclist)
{
    assert(kclist != NULL);

    return kclist->id;
}

INLINE okl4_kcap_t
okl4_kcap_item_getkcap(okl4_kcap_item_t *item)
{
    assert(item != NULL);

    return item->kcap;
}


#endif /* !__OKL4__KCLIST_H__ */
