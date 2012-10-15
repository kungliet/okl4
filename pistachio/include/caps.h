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
/*
 * Description:  Capability handling and abstraction
 */
#ifndef __CAPS_H__
#define __CAPS_H__


#include <read_write_lock.h>

#define CAP_OBJ_INDEX_MASK ((1UL << (BITS_WORD - 4)) - 1)

typedef enum
{
    cap_type_invalid,
    cap_type_thread,
    cap_type_ipc
} cap_type_e;

class cap_t;
class ref_t;
class tcb_t;

namespace cap_reference_t
{
    DECLARE_READ_WRITE_LOCK(cap_reference_lock);

    void add_reference(cap_t *master_cap, cap_t *cap);
    void remove_reference(cap_t *master_cap, cap_t *cap);
    void remove_reference(cap_t *master_cap, ref_t *ref);
    void invalidate_reference(cap_t *master_cap);
    void add_reference(tcb_t *tcb, cap_t *cap);
    void remove_reference(tcb_t *tcb, cap_t *cap);
    void invalidate_reference(tcb_t *tcb);
}

/**
 * Internal kernel revocable object
 * The next pointer must map to the cap_t structure
 */
class ref_t
{
private:
    friend void mkasmsym();
    void *object;

public:
    ref_t *next;

    /**
     * Initialize this reference
     */
    void init(void) {
        object = NULL;
        next = NULL;
    }
    /**
     * Get TCB pointed to by this reference
     */
    tcb_t* get_tcb(void) {
        return (tcb_t*)object;
    }
    /**
     * Set TCB pointed to by this reference
     */
    void set_referenced(tcb_t* obj);
    /**
     * Clear TCB reference
     */
    void remove_referenced(tcb_t* obj);
};

class cap_t
{
private:
    /* IMPORTANT: 
     * cap_t size MUST be aligned to 2 power N
     * in order to optimize access in fastpath,
     * use padding if it is not! Please also 
     * change LOG2_SIZEOF_CAP_T  when adding new members.
     */
    union {
        struct {
            BITFIELD2(word_t,
                    obj_idx : BITS_WORD - 4,
                    type    : 4
            );
        } x;
        word_t raw;
    };

public:
    cap_t * next;

    /**
     * Clear CAP entry
     */
    void clear(void) { this->raw = 0; }
    /**
     * Initialize/reset CAP entry
     */
    void init(void) { this->raw = 0; this->next = 0; }
    /**
     * Get TCB pointed to by this CAP
     */
    tcb_t* get_tcb(void) {
        return (tcb_t*)(ARCH_OBJECT_PTR(this->x.obj_idx));
    }

    /**
     * Make this a thread CAP
     */
    void set_thread_cap(const tcb_t *tcb) {
        cap_t cap;
        cap.x.type = (word_t)cap_type_thread;
        cap.x.obj_idx = ARCH_OBJECT_INDEX(tcb);
        this->raw = cap.raw;
    }
    /**
     * Make this an ipc CAP
     */
    void set_ipc_cap(const tcb_t *tcb) {
        cap_t cap;
        cap.x.type = (word_t)cap_type_ipc;
        cap.x.obj_idx = ARCH_OBJECT_INDEX(tcb);
        this->raw = cap.raw;
    }

#if !defined(CONFIG_DEBUG)
private:
#endif
    /**
     * Check if we can ipc this cap
     */
    bool can_ipc_cap(void) { return (this->x.type == (word_t)cap_type_thread) || (this->x.type == (word_t)cap_type_ipc); };
    /**
     * Check if this is a valid
     */
    bool is_valid_cap(void) { return (this->raw != (word_t)cap_type_invalid); };
    /**
     * Check if this is a thread CAP
     */
    bool is_thread_cap(void) { return (this->x.type == (word_t)cap_type_thread); };
    /**
     * Check if this is an ipc CAP
     */
    bool is_ipc_cap(void) { return (this->x.type == (word_t)cap_type_ipc); };

#if defined(CONFIG_DEBUG)
    const char* cap_type(void) {
        switch (this->x.type) {
            case 0: return "inv";
            case 1: return "tcb";
            case 2: return "ipc";
            default: return "  ?";
        }
    }
#endif


    friend class clist_t;
    friend void mkasmsym();
    friend tcb_t * acquire_read_lock_tcb(tcb_t *, tcb_t *);
};

class cap_control_t
{
public:
    enum cap_op_e {
        op_create_clist = 0,
        op_delete_clist = 1,
        op_copy_cap     = 4,
        op_delete_cap   = 5
    };

    cap_op_e get_op() { return (cap_op_e)x.op; }

    word_t get_raw() { return raw; }

    void set_raw(word_t raw) { this->raw = raw; }

private:
    union {
        struct {
            BITFIELD2(word_t,
                    op      : 3,
                    _res1   : BITS_WORD - 3
            );
        } x;
        word_t raw;
    };
};

#endif /* __CAPS_H__ */

