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
 * Description: Capability Identifiers
 */
#ifndef __CAPID_H__
#define __CAPID_H__

#if defined(L4_32BIT)
#define ANYTHREAD_RAW       0xffffffffUL
#define WAITNOTIFY_RAW      0xfffffffeUL
#define MYSELF_RAW          0xfffffffdUL
#define NILTHREAD_RAW       0xfffffffcUL
#define SPECIAL_RAW_LIMIT   0xfffffffbUL
#define INVALID_RAW         0x0fffffffUL

#elif defined(L4_64BIT)
#define ANYTHREAD_RAW       0xffffffffffffffffUL
#define WAITNOTIFY_RAW      0xfffffffffffffffeUL
#define MYSELF_RAW          0xfffffffffffffffdUL
#define NILTHREAD_RAW       0xfffffffffffffffcUL
#define SPECIAL_RAW_LIMIT   0xfffffffffffffffbUL
#define INVALID_RAW         0x0fffffffffffffffUL

#else
#error Unknown wordsize
#endif

#define TYPE_CAP            0x0
#define TYPE_TCB_RESOURCE   0x1
#define TYPE_MUTEX_RESOURCE 0x2
#define TYPE_SPACE_RESOURCE 0x3
#define TYPE_CLIST_RESOURCE 0x4
#define TYPE_PHYS_MEM_SEG   0x5
#define TYPE_THREAD_HANDLE  0x8
#define TYPE_SPECIAL        0xf

#define CAPID_INDEX_MASK ((1UL << (BITS_WORD - 4)) - 1)

class capid_t
{
public:
    static capid_t nilthread() {
        capid_t cid;
        cid.raw = NILTHREAD_RAW;
        return cid;
    }

    static capid_t anythread() {
        capid_t cid;
        cid.raw = ANYTHREAD_RAW;
        return cid;
    };

    static capid_t waitnotify() {
        capid_t cid;
        cid.raw = WAITNOTIFY_RAW;
        return cid;
    };

    static capid_t myselfthread() {
        capid_t cid;
        cid.raw = MYSELF_RAW;
        return cid;
    };

    static capid_t capid(word_t type, word_t index) {
        capid_t cid;
        cid.raw = 0;
        cid.id.index = index;
        cid.id.type = type;
        return cid;
    }

    /* check for specific (well known) thread ids */
    bool is_nilthread() { return this->raw == NILTHREAD_RAW; }
    bool is_anythread() { return this->raw == ANYTHREAD_RAW; }
    bool is_waitnotify(){ return this->raw == WAITNOTIFY_RAW; }
    bool is_myself()    { return this->raw == MYSELF_RAW; }

    bool is_threadhandle()
        {
            return this->id.type == TYPE_THREAD_HANDLE;
        }
    bool is_cap_type()
        {
            return id.type == TYPE_CAP;
        }

    word_t get_index() { return id.index; }
    word_t get_type() { return id.type; }

    word_t get_raw() { return this->raw; }
    void set_raw(word_t raw) { this->raw = raw; }

    /* operators */
    bool operator == (const capid_t & cid) const
        {
            return this->raw == cid.raw;
        }

    bool operator != (const capid_t & cid) const
        {
            return this->raw != cid.raw;
        }

private:
    union {
        word_t raw;

        struct {
            BITFIELD2(word_t,
                      index : BITS_WORD-4,
                      type  : 4);
        } id;
    };
};

INLINE capid_t capid(word_t rawid)
{
    capid_t t;
    t.set_raw (rawid);
    return t;
}

INLINE capid_t threadhandle(word_t handle)
{
    capid_t t;
    t = capid_t::capid(TYPE_THREAD_HANDLE, handle);
    return t;
}

/* special ids */
#define NILTHREAD       (capid_t::nilthread())
#define ANYTHREAD       (capid_t::anythread())
#define WAITNOTIFY      (capid_t::waitnotify())

#endif /* !__CAPID_H__ */

