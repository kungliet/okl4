#
# Copyright (c) 2007 Open Kernel Labs, Inc. (Copyright Holder).
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

import weaver.modify
from elf.core import UnpreparedElfFile
import util
import os
import sys
import subprocess

modules_under_test = [weaver.modify]

# Note: We test not equal to zero, rather than explicit exit codes
# as it differs across python versions.

def program_exists(name):
    """Verify that an external program exists."""
    try:
        retcode = subprocess.call('%s >/dev/null 2>&1' % name, shell=True)
        return retcode != 127
    except OSError:
        return False

have_objcopy = program_exists("arm-linux-objcopy")
if not have_objcopy:
    print >> sys.stderr, "test_modify.py: You don't have arm-linux-objcopy " \
          "installed.  Not all tests will run."

class TestMergeCmd(util.TestCmd):
    cmd = [weaver.modify.modify_cmd]

    def tearDown(self):
        """Remove work temp files."""
        for f in ("test_output", "test_objcopy"):
            try:
                os.remove(f)
            except OSError:
                pass

    def test_help(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["-H"])
        self.assertEquals(exit_value, 0)
        self.assertEquals(ret_stdout.capitalize().startswith("Usage"), True)

    def test_bad_input(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["--physical"])
        self.assertEquals(exit_value, 2)
        self.assertEquals(ret_stderr.capitalize().startswith("Usage"), True)
        
    def test_physical(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/arm_exec", "--physical", "-o", "test_output"])
        self.assertEquals(exit_value, 0)
        
    def test_adjust(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/arm_exec", "--adjust", "segment.paddr", "+0x010000", "-o", "test_output"])
        self.assertEquals(exit_value, 0)
        
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/arm_exec", "--adjust", "segment.paddr", "0x00010000", "-o", "test_output"])
        self.assertEquals(exit_value, 0)
    
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/arm_exec", "--adjust", "foo.paddr", "+0x010000", "-o", "test_output"])
        self.assertEquals(ret_stderr, "Error: Don't know about field foo\n")
        self.assertEquals(exit_value, 1)
        
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/arm_exec", "--adjust", "segment.foo", "+0x010000", "-o", "test_output"])
        self.assertEquals(ret_stdout, "Don't know about field_designator foo.\n")
        self.assertEquals(exit_value, 0)
        
        
    def test_physical_entry(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/arm_exec", "--physical_entry", "-o", "test_output"])
        self.assertEquals(exit_value, 0)
            
    def test_change(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/arm_exec", "--change", "segment.paddr", "0xa0100000=0xb0100000", "-o", "test_output"])
        self.assertEquals(exit_value, 0)
                
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/arm_exec", "--change", "foo.paddr", "0xa0100000=0xb0100000", "-o", "test_output"])
        self.assertEquals(ret_stderr, "Error: Don't know about field foo\n")
        self.assertEquals(exit_value, 1)
                
    def test_merge_sections(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/arm_exec", "--merge_sections", ".debug", "-o", "test_output"])
        self.assertEquals(exit_value, 0)
                    
    def test_remove_nobits(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/arm_scatter_load", "--remove_nobits", "-o", "test_output"])
        self.assertEquals(exit_value, 0)

    def test_binary_must_output(self):
        """
        --binary must be accompanied with -o <foo>
        """
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/image_gumstix.elf", "--binary"])
        self.assertEquals(ret_stderr.capitalize(),
                          "Usage: run.py modify file [options]\n\nrun.py: error: Output must be specified with -o <target>\n".capitalize())
        self.assertEquals(exit_value, 2)


    def test_binary_reloc(self):
        """
        Reloc files end up zero sized.

        This is different to objcopy, which does put data in the
        output file, but it's not worth fixing this for now.
        """
        test_files = (
            "data/arm_object",
            )

        for tf in test_files:
            # Then run through elfweaver modify
            exit_value, ret_stdout, ret_stderr = self._run_command([tf, "--binary", "-o", "test_output"])
            self.assertEquals(exit_value, 0)
            self.assertEquals(os.stat("test_output").st_size, 0)

    def test_binary_scatter_load(self):
        """
        Scatter-load files should be the same as the segment size.

        This is different to objcopy, which leaves out the final BSS
        section, but binutils can't handle scatter load files anyway.
        """
        test_files = (
            "data/arm_scatter_load",
            )

        for tf in test_files:
            # Then run through elfweaver modify
            exit_value, ret_stdout, ret_stderr = self._run_command([tf, "--binary", "-o", "test_output"])
            self.assertEquals(exit_value, 0)
            # The file size should be the size as the segment size.
            elf = UnpreparedElfFile(filename = tf)
            self.assertEquals(os.stat("test_output").st_size, elf.segments[0].get_memsz())

    def test_binary_kzm_arm11(self):
        # Make sure objcopy and cmp exist before beginning
        if not have_objcopy:
            return

        # ARM executables to test with.
        test_files = (
            "data/eg_weave/image_gumstix.elf",
            "data/arm_exec",
            "data/arm_gentoo_elf",
            )

        for tf in test_files:
            # Run file through objcopy first
            self.assertEquals(os.system("arm-linux-objcopy -O binary %s test_objcopy" % tf),
                              0)

            # Then run through elfweaver modify
            exit_value, ret_stdout, ret_stderr = self._run_command([tf, "--binary", "-o", "test_output"])
            self.assertEquals(exit_value, 0)

            # Shell out to cmp and make sure they are identical
            self.assertEquals(os.system("cmp --quiet test_objcopy test_output"), 0)

        os.remove("test_objcopy")

