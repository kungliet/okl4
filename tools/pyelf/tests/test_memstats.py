##############################################################################
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

import weaver.memstats
import util
import os
import commands
import sys


modules_under_test = [weaver.memstats,
                      weaver.memstats_xml,
                      weaver.memstats_model,
                      weaver.memstats_reports]

status, output = commands.getstatusoutput("xmllint")
# status is 0 when xmllint gave us its help some large nonzero otherwise
if not status:
    print >> sys.stderr, "test_memstats.py: You don't have xmllint " \
          "installed.  Not all tests will run."
    xml_args = "-x"
else:
    have_xmllint = True
    xml_args = "-xy" # -y for verify

status, output = commands.getstatusoutput("hg showconfig")
if status:
    print >> sys.stderr, "test_memstats.py: You don't have hg (mercurial) " \
          "installed.  Not all tests will run."
    have_hg = False
else:
    have_hg = True

class TestMemstatsCmd(util.TestCmd):
    cmd = [weaver.memstats.memstats_cmd]
    def tearDown(self):
        try:
            # memstats dumps the output in the same dir as the image.elf
            os.remove("data/eg_weave/memstats.xml")
        except OSError:
            pass

    ############################
    # Command line arguments
    ############################

    def test_help(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["-H"])
        self.assertEquals(exit_value, 0)
        self.assertEquals(ret_stdout.capitalize().startswith("Usage"), True)

    def test_no_arguments(self):
        exit_value, ret_stdout, ret_stderr = self._run_command([""])
        self.assertEquals(exit_value, 2)
        self.assertEquals(ret_stderr.capitalize().startswith("Usage"), True)

    def test_diff_bad_args(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["-d",
            "data/eg_weave/memstats_simple.xml"])
        self.assertEquals(exit_value, 2)
        self.assertEquals(ret_stderr.capitalize().startswith("Usage"), True)

    ############################
    # Bad input files
    ############################

    def test_nonexistant_file(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["--report=initial",
            "data/eg_weave/FOOBAR"])
        self.assertEquals(exit_value, 1)
        self.assertEquals(ret_stderr.startswith('Error: File "data/eg_weave/FOOBAR" does not exist'), True)

    def test_no_segments(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["-x",
            "data/arm_object"])
        self.assertEquals(exit_value, 1)
        self.assertEquals(ret_stderr.startswith('Error: ELF file "data/arm_object" contains no segments'), True)


    ############################
    # Plain Text Reports and XML Parsing
    ############################

    def test_empty_xml(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["--report=fixed",
            "data/eg_weave/memstats_empty.xml"])
        self.assertEquals(exit_value, 1)
        self.assertEquals(ret_stderr.startswith("Failed to parse"), True)

    def test_simple_xml(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["--report=initial",
            "data/eg_weave/memstats_simple.xml"])
        self.assertEquals(exit_value, 0)
        self.assertNotEquals(ret_stdout, "")

    ############################
    # TeX Reports 
    ############################
#
# Removed as ia32 does not currently build.
#

#    def test_ia32_no_stats(self):
#        """An image that doesn't contain any Iguana stats"""
#        exit_value, ret_stdout, ret_stderr = self._run_command([xml_args,
#            "-T", "data/eg_weave/image_ia32.elf"])
#        self.assertEquals(exit_value, 0)
#        self.assertEquals(ret_stdout, "")

#    def test_gumstix_tex(self):
#        """Print the Iguana stats in TeX"""
#        exit_value, ret_stdout, ret_stderr = self._run_command([xml_args,
#            "-T", "data/eg_weave/image_gumstix.elf"])
#        f = open("data/eg_weave/memstats_iguana_tex.txt", "r")
#        expected = f.read()
#        f.close()
#        self.assertEquals(exit_value, 0)
#        self.assertEquals(ret_stdout, expected)

    ############################
    # XML Generation 
    ############################

    def test_gumstix_empty_example_image(self):
        exit_value, ret_stdout, ret_stderr = self._run_command([xml_args,
            "data/eg_weave/image_gumstix.elf"])
        self.assertEquals(exit_value, 0)
        self.assertEquals(ret_stdout, "")

    def test_gumstix_with_revision(self):
        if have_hg:
            exit_value, ret_stdout, ret_stderr = \
                        self._run_command([xml_args,
                                           '-R', 'AAAA', '-c', 'BBBB',
                                           "data/eg_weave/image_gumstix.elf"])
            self.assertEquals(ret_stderr, "")
            self.assertEquals(ret_stdout, "")
            self.assertEquals(exit_value, 0)

            exit_value, ret_stdout, ret_stderr = \
                        self._run_command([xml_args,
                                           '-R', 'AAAA',
                                           "data/eg_weave/image_gumstix.elf"])
            self.assertEquals(ret_stderr, "")
            self.assertEquals(ret_stdout, "")
            self.assertEquals(exit_value, 0)

            exit_value, ret_stdout, ret_stderr = \
                        self._run_command([xml_args,
                                           '-c', 'BBBB',
                                           "data/eg_weave/image_gumstix.elf"])
            self.assertEquals(ret_stderr, "")
            self.assertEquals(ret_stdout, "")
            self.assertEquals(exit_value, 0)

    def test_gumstix_print_top20(self):
        """Make sure we print only the specified number of symbols"""
        exit_value, ret_stdout, ret_stderr = self._run_command([xml_args,
            "--report=fixed", "-v", "-l", "20", "data/eg_weave/image_gumstix.elf"])
        self.assertEquals(exit_value, 0)
        self.assertNotEquals(ret_stdout.find("Top 20 functions"), -1)
        self.assertNotEquals(ret_stdout.find("Top 20 global data elements"), -1)

    def test_gumstix_print_all(self):
        """Print a complete report and compare it with correct/expected output"""
        exit_value, ret_stdout, ret_stderr = self._run_command([xml_args,
            "--report=initial", "-v", "data/eg_weave/image_gumstix.elf"])
        f = open("data/eg_weave/memstats_image_gumstix_initial.txt", "r")
        expected = f.read()
        f.close()
        self.assertEquals(exit_value, 0)
        self.assertEquals(ret_stdout, expected)

        exit_value, ret_stdout, ret_stderr = self._run_command([xml_args,
            "--report=fixed", "-v", "data/eg_weave/image_gumstix.elf"])
        f = open("data/eg_weave/memstats_image_gumstix_fixed.txt", "r")
        expected = f.read()
        f.close()
        self.assertEquals(exit_value, 0)
        self.assertEquals(ret_stdout, expected)

        exit_value, ret_stdout, ret_stderr = self._run_command([xml_args,
            "--report=heap", "-v", "data/eg_weave/image_gumstix.elf"])
        f = open("data/eg_weave/memstats_image_gumstix_heap.txt", "r")
        expected = f.read()
        f.close()
        self.assertEquals(exit_value, 0)
        self.assertEquals(ret_stdout, expected)

        exit_value, ret_stdout, ret_stderr = self._run_command([xml_args,
            "--report=ids", "-v", "data/eg_weave/image_gumstix.elf"])
        f = open("data/eg_weave/memstats_image_gumstix_ids.txt", "r")
        expected = f.read()
        f.close()
        self.assertEquals(exit_value, 0)
        self.assertEquals(ret_stdout, expected)

    def test_gumstix_nano(self):
        exit_value, ret_stdout, ret_stderr = self._run_command([xml_args,
            "--report=initial", "-v", "data/eg_weave/image_gumstix_nano.elf"])
        self.assertEquals(exit_value, 0)
        
        exit_value, ret_stdout, ret_stderr = self._run_command([xml_args,
            "--report=fixed", "-v", "data/eg_weave/image_gumstix_nano.elf"])
        self.assertEquals(exit_value, 0)
        
    ############################
    # Diff Reports
    ############################

    def test_memstats_diff(self):
        """Do a diff and compare it with correct/expected output"""
        exit_value, ret_stdout, ret_stderr = self._run_command(["--report=fixed",
            "--diff",
            "data/eg_weave/memstats_simple.xml",
            "data/eg_weave/memstats_simple_diff.xml"])
        f = open("data/eg_weave/memstats_diff_output.txt", "r")
        expected = f.read()
        f.close()
        self.assertEquals(exit_value, 0)
        self.assertEquals(ret_stdout, expected)

    def test_diff_hg_changeset(self):
        """Diff should reveal mercurial changesets are different"""
        exit_value, ret_stdout, ret_stderr = self._run_command(["--report=fixed",
            "--diff",
            "data/eg_weave/memstats_simple.xml",
            "data/eg_weave/memstats_diff_hg1.xml"])
        self.assertEquals(exit_value, 0)
        self.assertNotEquals(ret_stdout.find("Revision details differ"), -1)

    def test_diff_hg_tree(self):
        """Diff should reveal mercurial trees are different"""
        exit_value, ret_stdout, ret_stderr = self._run_command(["--report=fixed",
            "--diff",
            "data/eg_weave/memstats_simple.xml",
            "data/eg_weave/memstats_diff_hg2.xml"])
        self.assertEquals(exit_value, 0)
        self.assertNotEquals(ret_stdout.find("Images built from different repositories"), -1)
