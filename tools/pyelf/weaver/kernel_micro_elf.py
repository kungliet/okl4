###############################################################################
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
This module provides the interface to the root server descriptor in
the .roinit section.
"""

from sys import stdout
import struct
from math import log
from elf.ByteArray import ByteArray
from elf.util import IntString, align_up
from elf.constants import ElfMachine, EF_MIPS_ABI_O64
from weaver.memstats_model import InitScript
from weaver.util import _0

# ELF class  (Ref: 1-7)
class MemDescType(IntString):
    """IntString for ELF class"""
    _show = {}

MEMDESC_UNDEFINED    = MemDescType(0, "undefined")
MEMDESC_CONVENTIONAL = MemDescType(1, "conventional")
MEMDESC_RESERVED     = MemDescType(2, "reserved")
MEMDESC_DEDICATED    = MemDescType(3, "dedicated")
MEMDESC_NUMA         = MemDescType(4, "numa")
MEMDESC_GLOBAL       = MemDescType(5, "global")
MEMDESC_TRACE        = MemDescType(0xb, "trace")
MEMDESC_INITIAL      = MemDescType(0xc, "initial mapping")
MEMDESC_BOOT         = MemDescType(0xe, "bootloader")
MEMDESC_ARCH         = MemDescType(0xf, "arch specific")

# Table to map memory descriptor strings to memory descriptor
# objects.
MEMDESC_DICT = {
    str(MEMDESC_UNDEFINED)    : MEMDESC_UNDEFINED,
    str(MEMDESC_CONVENTIONAL) : MEMDESC_CONVENTIONAL,
    str(MEMDESC_RESERVED)     : MEMDESC_RESERVED,
    str(MEMDESC_DEDICATED)    : MEMDESC_DEDICATED,
    str(MEMDESC_NUMA)         : MEMDESC_NUMA,
    str(MEMDESC_GLOBAL)       : MEMDESC_GLOBAL,
    str(MEMDESC_TRACE)        : MEMDESC_TRACE,
    str(MEMDESC_INITIAL)      : MEMDESC_INITIAL,
    str(MEMDESC_BOOT)         : MEMDESC_BOOT,
    str(MEMDESC_ARCH)         : MEMDESC_ARCH,
    }

MEMRIGHTS_ALL         = MemDescType(0xff, "all")

MEMRIGHTS_DICT = {
    str(MEMRIGHTS_ALL)    : MEMRIGHTS_ALL,
    }

########## INIT SCRIPT DEFINES #############

# Magic Number
INITSCRIPT_MAGIC_NUMBER = 0x06150128

# Version of the structure format.
INITSCRIPT_STRUCTURE_VERSION = 1

# Bitfield related defines
SHIFT_4K   = 12
SHIFT_1K   = 10

# Operation Types
KI_OP_CREATE_HEAP            = 1

KI_OP_INIT_IDS               = 2
KI_OP_CREATE_THREAD_HANDLES  = 3

KI_OP_CREATE_CLIST           = 4
KI_OP_CREATE_SPACE           = 5
KI_OP_CREATE_THREAD          = 6
KI_OP_CREATE_MUTEX           = 7
KI_OP_CREATE_SEGMENT         = 8
KI_OP_MAP_MEMORY             = 9

KI_OP_CREATE_IPC_CAP         = 10
KI_OP_CREATE_MUTEX_CAP       = 11

KI_OP_ASSIGN_IRQ             = 12
KI_OP_ALLOW_PLATFORM_CONTROL = 13

class InitScriptEntry:
    """
    This is the superclass for all kernel init script entries.
    It performs both encoding and decoding of the kernel information via
    checking of the arguments passed to __init__.  Individual subclasses
    should overwrite the appropiate functions to implement themselves.
    """

    def __init__(self, script, magic, version, args, image, machine):
        """
        Setup a new instance of an init script entry, determine from the
        arguments passed if we are going to be encoding or decoding.  As
        these are done from completely different areas there shouldn't
        be any overlap in usage.
        """
        if script == None and magic == None and version == None and \
                args != None and image != None and machine != None:
            # We are doing a decode, so setup required variables and
            # store the arguments for later usage
            self.wordsize = machine.word_size
            self.halfword = self.wordsize / 2
            self._endianess = image.endianess
            self.ms = None
            self.page_shift = int(log(machine.min_page_size(), 2))

            if self.wordsize == 32:
                self._wordchar = "I"
            else:
                self._wordchar = "Q"

            self.format_str = self._endianess + self._wordchar

            self.eop = False

            self.store_args(args)
        elif script != None and magic != None and version != None and \
                args == None and image == None and machine == None:
            # Decoding is done straight away
            self.decode(script, magic, version)

    def decode(self, script, magic, version):
        """Decode a kernel init script entry.  Abstract function"""
        pass

    def encode(self):
        """Encode a kernel init script entry.  Abstract function"""
        raise NotImplementedError

    def sizeof(self):
        """Return the size in bytes of the operation."""
        return len(self.encode())

    def encode_word(self, word):
        """Encode a word into the image specific format"""
        return struct.pack(self.format_str, word)

    def encode_bitfield2_halfwords(self, a, b):
        """Helper function, encode halfwords"""
        if a > (1 << self.halfword):
            raise Exception, \
                    "Failed to encode bitfield. Parameter too large: %#x" % a
        if b > (1 << self.halfword):
            raise Exception, \
                    "Failed to encode bitfield. Parameter too large: %#x" % b

        return self.encode_word((b << self.halfword) | a)

    def encode_base_and_size(self, phys, size, shift):
        """
        Encode base and size by given shift.
        Shift also denotes phys alignment.
        Size is not modified.
        """
        if size > (1 << shift):
            raise Exception, \
                    "Failed to encode base and size. Size too large: %#x" % size
        mask = ((1 << shift) - 1)
        masked_phys = phys & (~mask)
        masked_size = size & mask
        word = masked_phys | masked_size

        return self.encode_word(word)

    def encode_hdr(self, op, size, end_of_phase = False):
        """Enoce a header item"""
        real_size = (size + 1)
        hdr = end_of_phase << (self.wordsize - 1) | \
              op << self.halfword | \
              real_size

        return self.encode_word(hdr)

    def set_eop(self):
        """Set this entry to mark the end of phase"""
        self.eop = True

    def store_args(self, args):
        """Store arguments for later use in encoding.  Abstract function"""
        pass

class InitScriptHeader(InitScriptEntry):
    """Kernel init script header"""
    def __init__(self, script, magic, version, args, image, machine):
        InitScriptEntry.__init__(self, script, magic, version, args,
                                 image, machine)

        self.magic   = None
        self.version = None

    def decode(self, _, magic, version):
        """Decode kernel init script header"""
        self.magic = magic
        self.version = version

    def encode(self):
        """Encode kernel init script header"""
        return ''.join((
            self.encode_word(INITSCRIPT_MAGIC_NUMBER),
            self.encode_word(INITSCRIPT_STRUCTURE_VERSION)
            ))

    def __repr__(self):
        """Return string representation"""
        return "HEADER (magic: %#x, version: %#x)" % \
                (self.magic, self.version)

class InitScriptCreateHeap(InitScriptEntry):
    """Kernel init script create heap"""
    def __init__(self, script, magic, version, args, image, machine):
        self.base = None
        self.size = None

        InitScriptEntry.__init__(self, script, magic, version, args,
                                 image, machine)

    def decode(self, script, section, words):
        """Decode kernel init script create heap"""
        self.base, self.size = script.decode_base_and_size(words[0], SHIFT_4K)
        self.size = self.size * 0x1000 # convert to bytes

        if script._report is not None:
            script._report.add_heap(self.base,
                                    self.size,
                                    script._sizes.kmem_chunksize,
                                    script._sizes.small_object_blocksize)

    def encode(self):
        """Encode kernel init script create heap"""
        return ''.join((
            self.encode_hdr(KI_OP_CREATE_HEAP, 1, self.eop),
            self.encode_base_and_size(_0(self.base),
                                      (align_up(_0(self.size), 0x1000)) >> \
                                      SHIFT_4K,
                                      SHIFT_4K)
            ))

    def store_args(self, args):
        """Store arguments for kernel init script create heap"""
        (self.base, self.size) = args

    def __repr__(self):
        """Return string representation"""
        return "CREATE HEAP (base: %#x, size: %#x)" % \
                (self.base, self.size)

class InitScriptInitIds(InitScriptEntry):
    """Kernel init script initial id's"""
    def __init__(self, script, magic, version, args, image, machine):
        self.spaces  = None
        self.clists    = None
        self.mutexes = None

        InitScriptEntry.__init__(self, script, magic, version, args,
                                 image, machine)

    def decode(self, script, section, words):
        """Decode kernel init script initial id's"""
        self.spaces, self.clists = script.decode_bitfield2_halfwords(words[0])
        self.mutexes = words[1]

        if script._report is not None:
            script._report.add_spaceid_list(self.spaces)
            script._report.add_clistid_list(self.clists)
            script._report.add_mutexid_list(self.mutexes)

        self.spaces_used, self.caps_used, self.mutexes_used = (0, 0, 0)
        script._global_ids = self

    def encode(self):
        """Encode kernel init script initial id's"""
        return ''.join((
            self.encode_hdr(KI_OP_INIT_IDS, 2, self.eop),
            self.encode_bitfield2_halfwords(_0(self.spaces),
                                            _0(self.clists)),
            self.encode_word(_0(self.mutexes))
            ))

    def store_args(self, args):
        """Store arguments for kernel init script initial id's"""
        (self.spaces, self.clists, self.mutexes) = args


    def __repr__(self):
        """Return string representation"""
        return "INIT IDS (spaces: %d, clists: %d, mutexes: %d)" % \
                (self.spaces, self.clists, self.mutexes)


class InitScriptCreateThreadHandles(InitScriptEntry):
    """Kernel init script create thread handles"""
    def __init__(self, script, magic, version, args, image, machine):
        self.phys = None
        self.size = None

        InitScriptEntry.__init__(self, script, magic, version, args,
                                 image, machine)

    def decode(self, script, section, words):
        """Decode kernel init script create thread handles"""
        self.phys, self.size = script.decode_base_and_size(words[0], SHIFT_1K)
        self.size = self.size * 4 # size is 4-word multiple
        if script._report is not None:
            script._report.add_global_thread_handles(self.phys, self.size)
        script._tha = self
        self.threads = 0

    def encode(self):
        """
        Encode kernel init script create thread handles,
        Size is 4-word multiple. Phys is 1K aligned.
        """
        return ''.join((
            self.encode_hdr(KI_OP_CREATE_THREAD_HANDLES, 1, self.eop),
            self.encode_base_and_size(_0(self.phys),
                                      align_up(_0(self.size), 4) / 4,
                                      SHIFT_1K)
            ))

    def store_args(self, args):
        """Store arguments for kernel init script create thread handles"""
        (self.phys, self.size) = args

    # Used to track allocations during decoding
    def add_thread(self):
        self.threads += 1

    def __repr__(self):
        """Return string representation"""
        return "CREATE THREAD HANDLE ARRAY (phys: %#x, size: %#x)" % \
                (self.phys, self.size)

    def format(self, formats):
        data = {"TOTAL" : self.size / 4,
                "USED"  : self.threads}
        return formats["create_tha"] % data

class InitScriptCreateClist(InitScriptEntry):
    """Kernel init script create clist"""
    def __init__(self, script, magic, version, args, image, machine):
        self.id       = None
        self.max_caps = None

        InitScriptEntry.__init__(self, script, magic, version, args,
                                 image, machine)

    def decode(self, script, section, words):
        """Decode kernel init script create clist"""
        self.id, self.max_caps = script.decode_bitfield2_halfwords(words[0])

        if script._report is not None:
            script._report.create_clist(self.id, self.max_caps)

    def encode(self):
        """Encode kernel init script create clist"""
        return ''.join((
            self.encode_hdr(KI_OP_CREATE_CLIST, 1, self.eop),
            self.encode_bitfield2_halfwords(_0(self.id),
                                            _0(self.max_caps))
            ))

    def store_args(self, args):
        """Store arguments kernel init script create clist"""
        (self.id, self.max_caps) = args

    def __repr__(self):
        """Return string representation"""
        return "CREATE CLIST (id: %d, max_caps: %d)" % \
                (self.id, self.max_caps)

class InitScriptCreateSpace(InitScriptEntry):
    """Kernel init script create space"""
    def __init__(self, script, magic, version, args, image, machine):
        self.id = None
        self.space_base     = None
        self.space_num      = None
        self.clist_base     = None
        self.clist_num      = None
        self.mutex_base     = None
        self.mutex_num      = None
        self.max_phys_seg   = None
        self.utcb_base      = None
        self.utcb_log2_size = None
        self.has_kresources = None
        self.max_priority   = None

        InitScriptEntry.__init__(self, script, magic, version, args,
                                 image, machine)

    def decode(self, script, section, words):
        """Decode kernel init script create space"""
        phys_seg_mask   = (1 << script._halfword_bits) - 1
        kresources_mask = 1 << script._halfword_bits
        max_prio_mask   = ~((1 << (script._halfword_bits+1)) - 1)

        self.id = words[0]
        self.space_base, self.space_num = \
                script.decode_bitfield2_halfwords(words[1])
        self.clist_base, self.clist_num = \
                script.decode_bitfield2_halfwords(words[2])
        self.mutex_base, self.mutex_num = \
                script.decode_bitfield2_halfwords(words[3])
        self.utcb_base, self.utcb_log2_size = \
                script.decode_base_and_size(words[4], SHIFT_1K)
        self.max_phys_seg   = words[5] & phys_seg_mask
        self.has_kresources = (words[5] & kresources_mask) >> script._halfword_bits
        self.max_priority   = (words[5] & max_prio_mask) >> (script._halfword_bits+1)

        if script._report is not None:
            script._report.create_space((self.space_base, self.space_num),
                                        (self.clist_base, self.clist_num),
                                        (self.mutex_base, self.mutex_num),
                                        self.utcb_base,
                                        self.has_kresources,
                                        self.max_phys_seg)


    def encode(self):
        """Encode kernel init script create space"""
        return ''.join((
            self.encode_hdr(KI_OP_CREATE_SPACE, 6, self.eop),
            self.encode_word(_0(self.id)),
            self.encode_bitfield2_halfwords(_0(self.space_base),
                                            _0(self.space_num)),
            self.encode_bitfield2_halfwords(_0(self.clist_base),
                                            _0(self.clist_num)),
            self.encode_bitfield2_halfwords(_0(self.mutex_base),
                                            _0(self.mutex_num)),
            self.encode_base_and_size(_0(self.utcb_base),
                                      _0(self.utcb_log2_size),
                                      SHIFT_1K),
            self.encode_word((_0(self.max_priority) << (_0(self.halfword) + 1)) | \
                                 (_0(self.has_kresources) << (_0(self.halfword)))   | \
                                 (_0(self.max_phys_seg)))
            ))

    def store_args(self, args):
        """Store arguments for kernel init script create space"""
        (self.id, self.space_base, self.space_num, self.clist_base,
                self.clist_num, self.mutex_base, self.mutex_num,
                self.max_phys_seg, self.utcb_base,
                self.utcb_log2_size, self.has_kresources,
                self.max_priority) = args

    def __repr__(self):
        """Return string representation"""
        return "CREATE SPACE (id: %d, space range: %d-%d (%d), " \
                "clist range: %d-%d (%d), mutex range: %d-%d (%d), " \
                "max physical segments: %d, " \
                "utcb area base: %#x, utcb area size (log 2): %d, " \
                "has kresources: %s, max priority: %d)" % \
                (self.id,
                 self.space_base, self.space_base + self.space_num - 1,
                 self.space_num,
                 self.clist_base, self.clist_base + self.clist_num - 1,
                 self.clist_num,
                 self.mutex_base, self.mutex_base + self.mutex_num - 1,
                 self.mutex_num,
                 self.max_phys_seg, self.utcb_base, self.utcb_log2_size,
                 self.has_kresources, self.max_priority)

    def format(self, formats):
        data = {"ID"          : self.id,
                # "HEAP_ID"     : self.heap_id,
                "CLIST"       : self.space_num,
                # "THREADS"     : self.threads,
                # "MUTEXES"     : self.mutex_num,
                # "MUTEX_ALLOC" : self.mutexes,
                "SEGS"        : self.max_phys_seg,
                # "SEGS_ALLOC"  : self.segments
                }
        return formats["create_space"] % data

class InitScriptCreateThread(InitScriptEntry):
    """Kernel init script create thread"""
    def __init__(self, script, magic, version, args, image, machine):
        self.cap_slot  = None
        self.prio      = None
        self.ip        = None
        self.sp        = None
        self.utcb_addr = None
        self.mr1       = None

        InitScriptEntry.__init__(self, script, magic, version, args,
                                 image, machine)

    def decode(self, script, section, words):
        """Decode kernel init script create thread"""
        self.cap_slot, self.prio = script.decode_bitfield2_halfwords(words[0])
        self.ip = words[1]
        self.sp = words[2]
        self.utcb_addr = words[3]
        self.mr1 = words[4]

        if script._report is not None:
            script._report.create_thread(self.utcb_addr)

    def encode(self):
        """Encode kernel init script create thread"""
        return ''.join((
            self.encode_hdr(KI_OP_CREATE_THREAD, 5, self.eop),
            self.encode_bitfield2_halfwords(_0(self.cap_slot),
                                            _0(self.prio)),
            self.encode_word(_0(self.ip)),
            self.encode_word(_0(self.sp)),
            self.encode_word(_0(self.utcb_addr)),
            self.encode_word(_0(self.mr1))
            ))

    def store_args(self, args):
        """Store arguments for kernel init script create thread"""
        (self.cap_slot, self.prio, self.ip, self.sp, self.utcb_addr,
                self.mr1) = args

    def __repr__(self):
        """Return string representation"""
        return "CREATE THREAD (cap slot: %d, priority: %d, " \
                "ip: %#x, sp: %#x, utcb base address: %#x, mr1: %#x)" % \
                (self.cap_slot, self.prio, self.ip, self.sp,
                        self.utcb_addr, self.mr1)

    def format(self, formats):
        return ""

class InitScriptCreateMutex(InitScriptEntry):
    """Kernel init script create mutex"""
    def __init__(self, script, magic, version, args, image, machine):
        self.id = None

        InitScriptEntry.__init__(self, script, magic, version, args,
                                 image, machine)

    def decode(self, script, section, words):
        """Decode kernel init script create mutex"""
        self.id = words[0]

        if script._report is not None:
            script._report.create_mutex()

    def encode(self):
        """Encode kernel init script create mutex"""
        return ''.join((
            self.encode_hdr(KI_OP_CREATE_MUTEX, 1, self.eop),
            self.encode_word(_0(self.id))
            ))

    def store_args(self, args):
        """Store arguments for kernel init script create mutex"""
        (self.id, ) = args

    def __repr__(self):
        """Return string representation"""
        return "CREATE MUTEX (id = %d)" % (self.id)

    def format(self, formats):
        return ""

class InitScriptCreateSegment(InitScriptEntry):
    """Kernel init script create segment"""

    def decode(self, script, section, words):
        """Decode kernel init script create segment"""
        self.seg_num = words[0]
        self.phys_addr = words[1] >> 8
        self.attr = words[1] - (self.phys_addr << 8)
        self.size = words[2] >> 10
        self.rwx = words[2] - (self.size << 10)
        # We don't have a valid page_shift value when running elfweaver itself
        # (only whatever is in the elf file is available) so we hardcode 12
        # here.  The display will be incorrect on different systems
        self.phys_addr <<= 12
        self.size <<= 12

        if script._report is not None:
            script._report.create_segment(self.seg_num,
                                          self.phys_addr,
                                          self.size)

    def encode(self):
        """Encode kernel init script create segment"""
        return ''.join((
            self.encode_hdr(KI_OP_CREATE_SEGMENT, 3, self.eop),
            self.encode_word(self.seg_num),
            self.encode_word(self.phys_addr >> self.page_shift << 8 |
                             self.attr),
            self.encode_word(self.size >> self.page_shift << 10 | self.rwx)
            ))

    def store_args(self, args):
        """Store arguments for kernel init script create segment"""
        (self.seg_num, self.phys_addr, self.attr, self.size, self.rwx) = args


    def __repr__(self):
        """Return string representation"""
        return "CREATE SEGMENT (seg_num: %#x, segment phys_addr: %#x, attr: %#x, size: %#x, rwx: %#x)" % \
                (self.seg_num, self.phys_addr, self.attr, self.size, self.rwx)


class InitScriptMapMemory(InitScriptEntry):
    """Kernel init script map memory"""

    def decode(self, script, section, words):
        """Decode kernel init script map memory"""
        rwx_mask = (1 << 3) - 1
        pg_sz_log2_mask = 0x3f << 4
        num_pages_mask = ~((1 << 10) - 1)
        attr_mask = (1 << 8) - 1
        virt_mask = num_pages_mask

        self.seg_offset, self.seg_num = script.decode_base_and_size(words[0],
                                                                    SHIFT_1K)
        self.rwx = words[1] & rwx_mask
        self.page_size_log2 = (words[1] & pg_sz_log2_mask) >> 4
        self.num_pages = (words[1] & num_pages_mask) >> 10
        self.attr = words[2] & attr_mask
        self.virt = words[2] & virt_mask

        if script._report is not None:
            pgsz = pow(2, self.page_size_log2)
            self.full_size = pgsz * self.num_pages
            script._report.map_memory(self.seg_num, self.virt, self.full_size)

    def encode(self):
        """Encode kernel init script map memory"""
        __res1 = 0
        __res2 = 0
        return ''.join((
            self.encode_hdr(KI_OP_MAP_MEMORY, 3, self.eop),
            self.encode_base_and_size(self.seg_offset,
                                      self.seg_num,
                                      SHIFT_1K),
            # Do these verbosely, saves us re-writing them once all fields are used
            self.encode_word(self.num_pages << 10 | \
                             self.page_size_log2 << 4 | \
                             __res1 << 3 | self.rwx),
            self.encode_word((self.virt >> SHIFT_1K) << 10 | \
                             __res2 << 8 | self.attr)
            ))

    def store_args(self, args):
        """Store arguments for kernel init script map memory"""
        (self.seg_num, self.seg_offset, self.rwx,
                self.page_size_log2, self.num_pages, self.attr,
                self.virt) = args

    def __repr__(self):
        """Return string representation"""
        return "MAP MEMORY (segment offset: %#x, segment number: 0x%x, " \
                "rwx: 0x%x, page size (log2): %d, num pages: %#x, " \
                "attr: 0x%x, virtual base: %#x)" % \
                (self.seg_offset, self.seg_num, self.rwx, self.page_size_log2,
                        self.num_pages, self.attr, self.virt)


class InitScriptCreateIpcCap(InitScriptEntry):
    """Kernel init script create ipc cap"""

    def decode(self, script, section, words):
        """Decode kernel init script create ipc cap"""
        self.clist_ref, self.cap_slot = \
                script.decode_bitfield2_halfwords(words[0])
        self.thread_ref = words[1]

    def encode(self):
        """Encode kernel init script create ipc cap"""
        return ''.join((
            self.encode_hdr(KI_OP_CREATE_IPC_CAP, 2, self.eop),
            self.encode_bitfield2_halfwords(self.clist_ref,
                                            self.cap_slot),
            self.encode_word(self.thread_ref)
            ))

    def store_args(self, args):
        """Store arguments for kernel init script create ipc cap"""
        (self.clist_ref, self.cap_slot, self.thread_ref) = args

    def __repr__(self):
        """Return string representation"""
        return "CREATE IPC CAP (clist ref: %#x, cap slot: %d, " \
                "thread reference: %#.4x)" % \
                (self.clist_ref, self.cap_slot, self.thread_ref)


class InitScriptCreateMutexCap(InitScriptEntry):
    """Kernel init script create mutex cap"""

    def decode(self, script, section, words):
        """Decode kernel init script create mutex cap"""
        self.clist_ref, self.cap_slot = \
                script.decode_bitfield2_halfwords(words[0])
        self.mutex_ref = words[1]

    def encode(self):
        """Encode kernel init script create mutex cap"""
        return ''.join((
            self.encode_hdr(KI_OP_CREATE_MUTEX_CAP, 2, self.eop),
            self.encode_bitfield2_halfwords(self.clist_ref,
                                            self.cap_slot),
            self.encode_word(self.mutex_ref)
            ))

    def store_args(self, args):
        """Store arguments for kernel init script create mutex cap"""
        (self.clist_ref, self.cap_slot, self.mutex_ref) = args

    def __repr__(self):
        """Return string representation"""
        return "CREATE MUTEX CAP (clist ref: %#x, cap slot: %d, " \
                "mutex reference: %#x)" % \
                (self.clist_ref, self.cap_slot, self.mutex_ref)


class InitScriptAssignIrq(InitScriptEntry):
    """Kernel init script assign irq"""

    def decode(self, script, section, words):
        """Decode kernel init script assign irq"""
        self.irq = words[0]

    def encode(self):
        """Encode kernel init script assign irq"""
        return ''.join((
            self.encode_hdr(KI_OP_ASSIGN_IRQ, 1, self.eop),
            self.encode_word(self.irq)
            ))

    def store_args(self, args):
        """Store arguments for kernel init script assign irq"""
        (self.irq, ) = args

    def __repr__(self):
        """Return string representation"""
        return "ASSIGN IRQ (irq = %d)" % (self.irq)


class InitScriptAllowPlatformControl(InitScriptEntry):
    """Kernel init script allow plat control"""
    def decode(self, script, section, words):
        """Decode kernel init script allow plat control"""
        pass

    def encode(self):
        """Encode kernel init script allow plat control"""
        return self.encode_hdr(KI_OP_ALLOW_PLATFORM_CONTROL, 0, self.eop)

    def __repr__(self):
        """Return string representation"""
        return "ALLOW PLATFORM CONTROL"


class KernelConfigurationSection(object):
    """The kernel configuration section."""

    def __init__(self, elf, section):
        """
        Create a new root server section. This is not normally called
        directly, as rootserver instances usually are created first as
        ELfSection objects and then transformed.
        """
        self._section    = section
        self._cellinit   = None
        self._format_str = None
        if section.wordsize == 32 and not \
               (elf.machine == ElfMachine(8) and
                elf.flags & EF_MIPS_ABI_O64):
            self._wordsize = 32
            self._format_str = "I"
        else:
            self._wordsize = 64
            self._format_str = "Q"

        self._format_str = section.endianess + self._format_str

    def set_cellinit_addr(self, addr):
        self._cellinit = addr

    def get_cellinit_addr(self):
        self._cellinit = struct.unpack(self._format_str, self._section._data)
        return self._cellinit

    def update_data(self):
        """Update the section data based on our changes."""

        self._section._data = ByteArray(struct.pack(self._format_str,
                                                    self._cellinit))


    def output(self, f=stdout):
        """
        Print an ASCII representation of the root server section to
        the file f.
        """

        # This function is a little useless now...
        print >> f, "Kernel Configuration:"
        print >> f, "   Init Script: 0x%x" % self.get_cellinit_addr()


class InitScriptSection(object):
    class InitScriptPhaseMarker:
        def __init__(self):
            pass

        """A pseudo op that marks the end of a phase."""
        def __repr__(self):
            return "END OF PHASE"

        def format(self, formats):
            return ""

        def sizeof(self):
            """
            Return the size in bytes of the operation.

            Always returns 0 because this is a pseudo op.
            """
            return 0

    op_classes = {
        KI_OP_CREATE_HEAP                : InitScriptCreateHeap,
        KI_OP_INIT_IDS                   : InitScriptInitIds,
        KI_OP_CREATE_THREAD_HANDLES      : InitScriptCreateThreadHandles,
        KI_OP_CREATE_CLIST               : InitScriptCreateClist,
        KI_OP_CREATE_SPACE               : InitScriptCreateSpace,
        KI_OP_CREATE_THREAD              : InitScriptCreateThread,
        KI_OP_CREATE_MUTEX               : InitScriptCreateMutex,
        KI_OP_CREATE_SEGMENT             : InitScriptCreateSegment,
        KI_OP_MAP_MEMORY                 : InitScriptMapMemory,
        KI_OP_CREATE_IPC_CAP             : InitScriptCreateIpcCap,
        KI_OP_CREATE_MUTEX_CAP           : InitScriptCreateMutexCap,
        KI_OP_ASSIGN_IRQ                 : InitScriptAssignIrq,
        KI_OP_ALLOW_PLATFORM_CONTROL     : InitScriptAllowPlatformControl,
        }

    def decode_base_and_size(self, word, shift):
        mask = ((1 << shift) - 1)
        base = word & (~mask)
        size = word & mask

        return base, size

    def decode_bitfield2_halfwords(self, word):
        mask = ((1 << self._halfword_bits)-1)

        word1 = word & mask
        word2 = (word & ~mask) >> self._halfword_bits

        return word1, word2

    def __init__(self, section, elf, memstats):
        """Create a new bootinfosection. This is not normally called
        directly, as bootinfo instances usually are created first as
        ELfSection objects and then transformed."""
        self.section = section
        self._bytes_per_word = 0
        self.obj_count = 1
        data = self.section._data
        self._items = []
        self._segment = {}
        self.heap_id = 0

        # The first thing we need to do is determine the word size and
        # the endianness.  This is quite a dodgy way of working this
        # out but unfortunately the elf flags are unavailable.

        # Work out bootinfo type / endianness + wordsize.
        magic = struct.unpack("<I", data[0:4])[0]
        if magic == INITSCRIPT_MAGIC_NUMBER:
            self._wordsize = 4
            self._word_char = 'I'
            self._endianess = '<'

        magic = struct.unpack(">I", data[0:4])[0]
        if magic == INITSCRIPT_MAGIC_NUMBER:
            self._wordsize = 4
            self._word_char = 'I'
            self._endianess = '>'

        magic = struct.unpack("<Q", data[0:8])[0]
        if magic == INITSCRIPT_MAGIC_NUMBER:
            self._wordsize = 8
            self._word_char = 'Q'
            self._endianess = '<'

        magic = struct.unpack(">Q", data[0:8])[0]
        if magic == INITSCRIPT_MAGIC_NUMBER:
            self._wordsize = 8
            self._word_char = 'Q'
            self._endianess = '>'

        if not self._wordsize:
            print "\nERROR: Init Script has BAD MAGIC NUMBER\n"

        if self._wordsize == 4:
            self._size_mask = 0x0000FFFF
            self._op_mask   = 0x7FFF0000
            self._eop_mask  = 0x80000000
        else:
            self._size_mask = 0x00000000FFFFFFFF
            self._op_mask   = 0x7FFFFFFF00000000
            self._eop_mask  = 0x8000000000000000

        self._bytes_per_word = self._wordsize
        self._word_bits      = self._bytes_per_word * 8
        self._halfword_bits  = self._word_bits / 2
        self.format_word = self._endianess + self._word_char

        # Check version number
        version = struct.unpack(self.format_word,
                                data[self._wordsize:self._wordsize*2])[0]
        if version != INITSCRIPT_STRUCTURE_VERSION:
            print "\nERROR: Init Script has BAD STRUCTURE VERSION NUMBER\n"

        idx = 2*self._bytes_per_word
        phase_count = 0

        # Sizes

        self._sizes = find_elfweaver_info(elf)

        if memstats is not None:
            self._report = InitScript(memstats, self._sizes, self._wordsize)
        else:
            self._report = None


        # Transform the operations into classes.
        while idx < len(data):
            # Read the header.
            hdr = struct.unpack(self.format_word, data[idx:idx +
                                                       self._bytes_per_word])[0]
            # Calculate the size of the instruction in bytes and
            # words, and the op.
            halfword_bits = self._bytes_per_word * 4
            size = ((hdr & self._size_mask)) * self._bytes_per_word
            op = (hdr & self._op_mask) >> halfword_bits
            eop = bool(hdr & self._eop_mask)
            words = size / self._bytes_per_word

            # Turn into a class.  Pass the constructor the list of
            # words in the op for further processing.
            format = self._endianess + self._word_char * (words - 1)
            start  = idx + self._bytes_per_word
            end    = idx + size
            if not InitScriptSection.op_classes.has_key(op):
                raise Exception, "Unknown opcode: %d" % op

            args = struct.unpack(format, data[start:end])
            inst = InitScriptSection.op_classes[op](self, self, args, None,
                    None, None)

            # Initialise fields that are needed by encode() (called by
            # sizeof()) that are not set when decoding the operation.
            inst.eop        = eop
            inst.wordsize   = self._word_bits
            inst.halfword   = self._halfword_bits
            inst.format_str = self._endianess + self._word_char
            inst.page_shift = 12

            self._items.append(inst)
            if eop:
                phase_count += 1
                self._items.append(InitScriptSection.InitScriptPhaseMarker())

            # Move to the next op.
            idx += size

            if phase_count == 2:
                self.total_size = idx
                break

    def output(self, f=stdout):
        """Print an ASCII representation of the init script to the file f."""
        print >> f, "Initialisation Script (%s, %s) Operations:" % \
                (self._endianess == "<" and "Little endian" or "Big endian", \
                 str(self._wordsize) + " bit")
        rec_no = 0
        offset = 2 * self._bytes_per_word

        for op in self._items:
            print >> f, "%d (%#.4x): %s" % (rec_no, offset, str(op))
            rec_no += 1
            offset += op.sizeof()

        print >> f, "Total size: %d bytes" % self.total_size


class ElfweaverInfoSection(object):
    """The elfweaver info section."""

    def __init__(self, elf, section):
        """ Section containing elfweaver attributes """
        self._section    = section
        self._format_str = None
        self.attributes  = None
        self.utcb_size   = 0
        self.arch_max_spaces = 0
        self.valid_page_perms = 0
        self.arch_cache_perms = None

        if elf.wordsize == 32 and not \
               (elf.machine == ElfMachine(8) and
                elf.flags & EF_MIPS_ABI_O64):
            self._wordsize = 32
            self._format_str = "12B12I22H"
        else:
            self._wordsize = 64
            self._format_str = "12B12Q22H"

        self._format_str = elf.endianess + self._format_str


       # print "%s %s" % (len(self._format_str), len(self._section._data))

        default, cached, uncached, writeback, writethrough, coherent, device, \
                writecombining, strong, _, _, _, self.utcb_size, self.arch_max_spaces, \
                self.valid_page_perms, \
                self.clist_size, self.cap_size, self.space_size, \
                self.tcb_size, self.mutex_size, self.segment_list_size, \
                self.small_object_blocksize, self.kmem_chunksize, \
                self.pgtable_top_size, \
                uncached_mask, uncached_comp, \
                cached_mask, cached_comp, \
                iomemory_mask, iomemory_comp, \
                iomemoryshared_mask, iomemoryshared_comp, \
                writethrough_mask, writethrough_comp, \
                writeback_mask, writeback_comp, \
                shared_mask, shared_comp, \
                nonshared_mask, nonshared_comp, \
                custom_mask, custom_comp, \
                strong_mask, strong_comp, \
                buffered_mask, buffered_comp \
                = struct.unpack(self._format_str, self._section._data)

        self.arch_cache_perms = {
            'uncached'      : (uncached_mask, uncached_comp),
            'cached'        : (cached_mask, cached_comp),
            'iomemory'      : (iomemory_mask, iomemory_comp),
            'iomemoryshared': (iomemoryshared_mask, iomemoryshared_comp),
            'writethrough'  : (writethrough_mask, writethrough_comp),
            'writeback'     : (writeback_mask, writeback_comp),
            'shared'        : (shared_mask, shared_comp),
            'nonshared'     : (nonshared_mask, nonshared_comp),
            'custom'        : (custom_mask, custom_comp),
            'strong'        : (strong_mask, strong_comp),
            'buffered'      : (buffered_mask, buffered_comp)
            }

        self.attributes = [
                ('default'       , default),
                ('cached'        , cached),
                ('uncached'      , uncached),
                ('writeback'     , writeback),
                ('writethrough'  , writethrough),
                ('coherent'      , coherent),
                ('device'        , device),
                ('writecombining', writecombining),
                ('strong'        , strong)
                ]

    def get_attributes(self):
        """
        Return the machine specific values for the standard cache
        attributes.
        """
        return self.attributes

    def get_arch_max_spaces(self):
        """Return the maximum number of spaces supported by the architecture."""
        return self.arch_max_spaces

    def get_arch_cache_perms(self):
        return self.arch_cache_perms

    def output(self, f=stdout):
        """ Print out the elfweaver info section contents """

        print >> f, "Elfweaver Info Section:"
        print >> f, "   Attributes:"
        for i in self.attributes:
            pad = ' ' * (20 - len(i[0]))
            print >> f, "       %s%s0x%x" % (i[0], pad, i[1])
        print >> f, "   Cache Permissions:"
        for k, v in self.arch_cache_perms.iteritems():
            pad = ' ' * (20 - len(k))
            print >> f, "       %s%s0x%x, 0x%x" % (k, pad, v[0], v[1])
        print >> f, "   Max Spaces for Architecture: %s" % \
                ["unlimited", self.arch_max_spaces][self.arch_max_spaces != 0]
        print >> f, "   Page Rights Mask: 0x%x" % self.valid_page_perms
        print >> f, "   UTCB Size: %d Bytes" % self.utcb_size
        print >> f, "   Clist Size: %d Bytes" % self.clist_size
        print >> f, "   Cap Size: %d Bytes" % self.cap_size
        print >> f, "   Space Size: %d Bytes" % self.space_size
        print >> f, "   TCB Size: %d Bytes" % self.tcb_size
        print >> f, "   Mutex Size: %d Bytes" % self.mutex_size
        print >> f, "   Segment List Size: %d Bytes" % self.segment_list_size
        print >> f, "   Small Object Blocksize: %d Bytes" % \
                                        self.small_object_blocksize
        print >> f, "   Kmem Chunksize: %d Bytes" % self.kmem_chunksize
        print >> f, "   Pagetable Top Size: %d Bytes" % self.pgtable_top_size

def find_kernel_config(elf):
    """
    Find the kernel configuration section in an ELF. Return None if it
    can't be found.
    """
    roinit = elf.find_section_named(".roinit")

    if roinit is None:
        roinit = elf.find_section_named("kernel.roinit")

    if roinit:
        roinit = KernelConfigurationSection(elf, roinit)

    return roinit

def find_init_script(elf, memstats = None):
    """
    Find the initialisation script in an ELF.
    Return None if it cannot be found.
    """
    initscript = elf.find_section_named("initscript")

    if initscript:
        initscript = InitScriptSection(initscript, elf, memstats)

    return initscript

def find_elfweaver_info(elf):
    """
    Find the elfweaver_info section in an ELF.
    Return None if it cannot be found.
    """
    elfweaverinfo = elf.find_section_named(".elfweaver_info")

    if elfweaverinfo is None:
        elfweaverinfo = elf.find_section_named("kernel.elfweaver_info")

    if elfweaverinfo:
        elfweaverinfo = ElfweaverInfoSection(elf, elfweaverinfo)

    return elfweaverinfo
