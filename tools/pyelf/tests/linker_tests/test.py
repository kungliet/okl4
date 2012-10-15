#!/usr/bin/python
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

"""The elfweaver linker test suite."""

import os
import sys
import glob
import shutil
import subprocess

try:
    from elf.core import UnpreparedElfFile, PreparedElfFile
    from elf.constants import SHT_REL, SHT_RELA
except ImportError:
    print """
    ImportError: Cannot find pyelf library.  Did you run from tools/pyelf?
    """
    raise

base_dir = "tests/linker_tests/"

class LinkerTest:

    def parse_bin(self, tokens, file, pos = 0, line = "-"):
        self.bin = tokens[1].strip()

    def parse_cc(self, tokens, file, pos = 0, line = "-"):
        self.cc = tokens[1].strip()
        if self.cc == "None": self.cc = None

    def parse_retval(self, tokens, file, pos = 0, line = "-"):
        self.retval = int(tokens[1])

    def parse_exec(self, tokens, file, pos = 0, line = "-"):
        self.do_exec = (tokens[1].strip() == "True")

    def parse_srcs(self, tokens, file, pos = 0, line = "-"):
        while line.count(':') == 0 and len(line):
            pos = file.tell()
            line = file.readline()
            if line.count(':'):
                file.seek(pos)
                break
            self.srcs.append(line.strip())

    def parse_stdout(self, tokens, file, pos = 0, line = "-"):
        while line.count(':') == 0 and len(line):
            pos = file.tell()
            line = file.readline()
            if line.count(':'):
                file.seek(pos)
                break
            self.stdout.append(line)
        self.stdout = "".join(self.stdout)

    def parse_relocs(self, tokens, file, pos = 0, line = "-"):
        while len(line):
            line = file.readline()
            tokens = line.split(':')
            if tokens[0] == "file":
                self.relocs[tokens[1].strip()] = self.parse_file(tokens, file)
            else:
                return

    def parse_file(self, tokens, file, pos = 0, line = "-"):
        results = {}
        while line.count(':') == 0 and len(line):
            pos = file.tell()
            line = file.readline()
            if line.count('#'): continue
            tokens = line.strip().split(" ")
            if line.count(":") or len(line) == 0:
                file.seek(pos)
                break
            tokens = map(lambda x: int(x, 16), tokens)
            results[tokens[0]] = tuple(tokens)
        return results

    def __str__(self):
        results = []
        results.append("bin: %s" % self.bin.strip())
        results.append("retval: %d" % self.retval)
        results.append("srcs:")
        results += [src.strip() for src in self.srcs]
        results.append("cc: %s" % self.cc.strip())
        results.append("stdout:")
        results.append(self.stdout.strip())
        results.append("relocs:")
        for (file, relocs) in self.relocs.items():
            results.append("file: %s" % file)
            if len(relocs) == 0: continue
            elif len(relocs[0]) == 4:
                results.append("# Offset Type     Symdx    Value")
            elif len(relocs[0]) == 2:
                results.append("# Addr   Value")
            for (_, reloc) in relocs.items():
                results.append(" ".join(["%8.8x" % num for num in reloc]))
        return "\n".join(results)

    def __init__(self, filename, line = "-"):

        # Class instance attributes.
        self.bin = ''
        self.srcs = []
        self.cc = ''
        self.do_exec = False
        self.retval = 0
        self.stdout = []
        self.relocs = {}
        self.addrs = []

        # Initialise the instance.
        file = open(filename)
        parse = {'bin': self.parse_bin,
                 'srcs': self.parse_srcs,
                 'cc': self.parse_cc,
                 'exec': self.parse_exec,
                 'retval': self.parse_retval,
                 'stdout': self.parse_stdout,
                 'relocs': self.parse_relocs}
        while len(line):
            pos = file.tell()
            line = file.readline()
            tokens = line.split(':')
            if not len(line): break
            parse[tokens[0]](tokens, file)

        file.close()

    def test(self):
        self.compile()
        self.link()
        if self.do_exec: self.execute()
        self.check_relocs()
        self.check_contents()

    def compile(self):
        if self.cc is None:
            for src in self.srcs:
                shutil.copy(os.path.join(base_dir, 'src', src),
                        os.path.join(base_dir, 'build', src))
            return
        for src in self.srcs:
            cmd = [ self.cc, \
                    "-o", os.path.join(base_dir, 'build', src.replace(".s", ".o")), \
                    "-c", os.path.join(base_dir, 'src', src)]
            print "[CC  ] %s" % " ".join(cmd),
            if subprocess.call(cmd):
                print " -> FAILED!"
                raise Exception, "Compile error occured while executing %s!" % \
                    " ".join(cmd)
            print " -> OK"

    def link(self):
        cmd = ["./elfweaver", "link", \
                "-o", os.path.join(base_dir, 'build', self.bin)] + \
                [os.path.join(base_dir, 'build', src.replace(".s", ".o")) \
                for src in self.srcs]
        print "[LINK] %s" % " ".join(cmd),
        if subprocess.call(cmd):
            print " -> FAILED!"
            raise Exception, \
                    "Elfweaver link error occured while executing %s!" % \
                    " ".join(cmd)
        print " -> OK"

    def execute(self):
        cmd = [base_dir + 'build/' + self.bin]
        print "[EXEC] %s" % " ".join(cmd),
        proc = subprocess.Popen(cmd, stdout = subprocess.PIPE)
        (stdout, _) = proc.communicate()
        retval = proc.poll()
        if stdout != self.stdout:
            print " -> FAILED!"
            raise Exception, \
                    ("Stdout differs from expected!\n" + \
                    "Got: %s\nExpected: %s") % (stdout, self.stdout)
        if retval != self.retval:
            print " -> FAILED!"
            raise Exception, \
                    ("Return value differs from expected!  " + \
                    "Got: %d; Expected: %d") % (retval, self.retval)
        print " -> OK"

    def list_relocs(self, src = None):
        if src is None: srcs = self.srcs
        else: srcs = [src]
        for src in srcs:
            print "# Offset Type     Symdx    Value"
            elf = UnpreparedElfFile(os.path.join(base_dir,
                    'build', src.replace(".s", ".o")))
            for sec in filter(lambda s: s.type in (SHT_REL, SHT_RELA),
                    elf.sections):
                for reloc in sec.relocs:
                    print "%8.8x %8.8x %8.8x %8.8x" % \
                            (reloc.offset, reloc.type, reloc.symdx,
                            sec.info.get_word_at(reloc.offset))

    def list_contents(self):
        print "# Addr   Value"
        elf = UnpreparedElfFile(filename = os.path.join(base_dir, 'build', self.bin))
        text = elf.find_section_named(".text")
        for offset in self.addrs:
            print "%8.8x %8.8x" % (text.address + offset, text.get_word_at(offset))

    def check_relocs(self):
        for src in self.srcs:
            file = src.replace(".s", ".o")
            print "[REL ] %s" % file,
            elf = UnpreparedElfFile(os.path.join(base_dir, 'build', file))
            try:
                for sec in filter(lambda s: s.type in (SHT_REL, SHT_RELA),
                        elf.sections):
                    for reloc in sec.relocs:
                        self.addrs.append(reloc.offset)
                        if self.relocs[file][reloc.offset] != \
                                (reloc.offset, reloc.type, reloc.symdx,
                                sec.info.get_word_at(reloc.offset)):
                            print " -> FAILED!"
                            self.list_relocs(file)
                            raise Exception, \
                                    ("Input relocation differs from expected!  " + \
                                    "Got: %s\nExpected: %s") % \
                                    ((reloc.offset, reloc.type, reloc.symdx,
                                    sec.info.get_word_at(reloc.offset)),
                                    (reloc, self.relocs[file][reloc.offset]))
                print " -> OK"
            except KeyError:
                print " -> FAILED!"
                self.list_relocs()

    def check_contents(self):
        print "[CONT] %s" % self.bin,
        elf = UnpreparedElfFile(filename = os.path.join(base_dir, 'build', self.bin))
        text = elf.find_section_named(".text")
        try:
            # for offset in self.addrs:
            for offset, _ in self.relocs[self.bin].items():
                if text.get_word_at(offset - text.address) != self.relocs[self.bin][offset][1]:
                    print " -> FAILED!"
                    self.list_contents()
                    raise Exception, \
                            ("ELF contents differs from expected!  " + \
                             # "Got: %8.8x\nExpected: %8.8x") % (text.get_word_at(offset),
                             "Got: %s\nExpected: %s") % (text.get_word_at(offset - text.address),
                             self.relocs[self.bin][offset][1])
            print " -> OK"
        except KeyError:
            print " -> FAILED!"
            self.list_contents()


if __name__ == "__main__":

    if not os.path.isdir(base_dir + 'build/'):
        os.mkdir(base_dir + 'build/')

    specs = glob.glob(os.path.join(base_dir, '*.spec'))
    if len(specs) == 0:
        raise Exception, "No linker test specifications found!\n" + \
                "(Did you from from tools/pyelf?)"
    for spec in specs:
        spec = LinkerTest(spec)
        # print spec
        spec.test()


