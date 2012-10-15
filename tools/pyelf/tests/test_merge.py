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

import weaver.merge
import tests.util as util
import os
import commands
import sys
import weaver.cells.iguana.prog_pd_xml, weaver.cells.iguana_cell, \
       weaver.cells.iguana.extension_xml, weaver.cells.okl4_cell, \
       weaver.kernel_nano_elf
from StringIO import StringIO
from elf.core import PreparedElfFile
from weaver.kernel_micro_elf import find_kernel_config, find_init_script
from weaver.cells.iguana.bootinfo_elf import find_bootinfo

modules_under_test = [weaver.merge, weaver.cells.iguana.bootinfo,
                      weaver.segments_xml, weaver.image, weaver.kernel,
                      weaver.kernel_xml,
                      weaver.kernel_micro_elf,
                      weaver.kernel_xml,
                      weaver.kernel_micro_elf,
                      weaver.kernel_nano_elf,
                      weaver.machine_xml, weaver.namespace,
                      weaver.cells.iguana.bootinfo_elf, weaver.memobjs_xml,
                      weaver.cells.iguana.memobjs_xml, weaver.pools_xml,
                      weaver.cells.iguana.prog_pd_xml,
                      weaver.cells.iguana_cell,
                      weaver.cells.iguana.extension_xml,
                      weaver.cells.iguana.device,
                      weaver.cells.okl4_cell,
                      weaver.parse_spec, weaver.pools, weaver.util]

# Test to see if we have readelf installed

status, output = commands.getstatusoutput("readelf -H")
# status is 0 when readelf gave us its help some large nonzero otherwise
have_readelf = not status
if not have_readelf:
    print >> sys.stderr, "test_structures.py: You don't have readelf " \
          "installed. Not all tests will run."

# Note: We test not equal to zero, rather than explicit exit codes
# as it differs across python versions.

class TestMergeCmd(util.TestCmd):
    cmd = [weaver.merge.merge_cmd]

    def setUp(self):
        pass

    def tearDown(self):
        try:
            os.remove("test_output")
        except OSError:
            pass

    def test_help(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["-H"])
        self.assertEquals(exit_value, 0)
        self.assertEquals(ret_stdout.capitalize().startswith("Usage"), True)

    def test_no_output(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["specfoo"])
        self.assertNotEqual(exit_value, 0)
        self.assertEquals(ret_stderr.capitalize().startswith("Usage"), True)

    def test_no_spec(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["-o", "foo"])
        self.assertNotEqual(exit_value, 0)
        self.assertEquals(ret_stderr.capitalize().startswith("Usage"), True)

    def test_empty_spec(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/empty.xml", "-o", "test_output"])
        self.assertEquals(ret_stderr.startswith("Error: Failed to parse"), True)
        self.assertEquals(exit_value, 1)

    def test_simple_spec(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/simple.xml", "-o", "test_output"])
        self.assertEquals(ret_stderr, "")
        self.assertEquals(ret_stdout, "")
        self.assertEquals(exit_value, 0)

    def test_custom_spec(self):
        try:
            exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/custom.xml", "-o", "test_output", "-c", "data/eg_weave/custom"])
            self.assertEquals(exit_value, 0)
            self.assertEquals(ret_stderr, "")
            self.assertEquals(ret_stdout, "machine FOO = test_machine\nFOO = test\n")        
        finally:
            # Reset the iguana registration so that the other tests work.
            weaver.cells.iguana_cell.IguanaCell.register()

    def test_simple_extension(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/simple_extension.xml", "-o", "test_output"])
        self.assertEquals(ret_stderr, "")
        self.assertEquals(ret_stdout, "")
        self.assertEquals(exit_value, 0)

#     def test_simple_with_offset_spec(self):
#         exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/simple_with_offset.xml", "-o", "test_output"])
#         self.assertEquals(ret_stderr, "")
#         self.assertEquals(ret_stdout, "")
#         self.assertEquals(exit_value, 0)

    def test_simple_no_exec(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/simple_no_exec.xml", "-o", "test_output"])
        self.assertEquals(exit_value, 1)
        self.assertEquals(ret_stderr, "Error: All the merged ELF files must be of EXEC type.\n")
        self.assertEquals(ret_stdout, "")

    def test_simple_non_load(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/simple_non_load.xml", "-o", "test_output"])
        self.assertEquals(exit_value, 1)
        self.assertEquals(ret_stderr.startswith("Error: Unable to handle segment"), True)
        self.assertEquals(ret_stdout, "")

    def test_simple_ignore_segment(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/simple_ignore_segment.xml", "-o", "test_output"])
        self.assertEquals(ret_stderr, "")
        self.assertEquals(ret_stdout, "")

    def test_simple_raw_section(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/simple_raw_section.xml", "-o", "test_output"])
        self.assertEquals(exit_value, 0)
        self.assertEquals(ret_stderr, "")
        self.assertEquals(ret_stdout, "")

    def test_simple_raw_file(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/simple_raw_file.xml", "-o", "test_output"])
        self.assertEquals(exit_value, 0)
        self.assertEquals(ret_stderr, "")
        self.assertEquals(ret_stdout, "")

    def test_simple_noname(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/simple_noname.xml", "-o", "test_output"])
        self.assertEquals(exit_value, 0)
        self.assertEquals(ret_stderr, "")
        self.assertEquals(ret_stdout, "")

    def test_simple_patch(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/simple_with_patch.xml", "-o", "test_output"])
        self.assertEquals(ret_stderr, "")
        self.assertEquals(ret_stdout, "")
        self.assertEquals(exit_value, 0)

    def test_import_machine(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/simple_import_machine.xml", "-o", "test_output"])
        self.assertEquals(exit_value, 0)
        self.assertEquals(ret_stderr, "")
        self.assertEquals(ret_stdout, "")

    def test_multiple_pools(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/multiple_pools.xml", "-o", "test_output"])
        self.assertEquals(exit_value, 0)
        self.assertEquals(ret_stderr, "")
        self.assertEquals(ret_stdout, "")

    def test_multiple_elf_file(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/multiple_elf_file.xml", "-o", "test_output"])
        self.assertEquals(exit_value, 0)
        self.assertEquals(ret_stderr, "")
        self.assertEquals(ret_stdout, "")
# With dynamic allocation, this check needs to be re-written.
#        exit_val = self.check_segments_and_sections("data/eg_weave/multiple_elf_file.xml", "test_output")
#        self.assertEquals(exit_val, 0)

    def test_multiple_device_regions(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/multiple_device_regions.xml", "-o", "test_output"])
        self.assertEquals(exit_value, 0)
        self.assertEquals(ret_stderr, "")
        self.assertEquals(ret_stdout, "")

    def test_multiple_pager(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/multiple_pager.xml", "-o", "test_output"])
        self.assertEquals(exit_value, 0)
        self.assertEquals(ret_stderr, "")
        self.assertEquals(ret_stdout, "")

    def test_multiple_pager2(self):
        """Test various error conditions with custom pagers."""
        template = """
<image>
	<machine file="data/eg_weave/machine.xml" />
	<virtual_pool name="virtual">
		<memory base="0x1000" size="0xcffff000"/>
	</virtual_pool>

	<physical_pool name="physical">
		<memory base="0xa0000000" size="0x3800000"/>
	</physical_pool>

	<kernel file="data/eg_weave/l4kernel" virtpool="virtual" physpool="physical">
	</kernel>

        <iguana name="iguana_server" file="data/eg_weave/ig_server" kernel_heap="2M">
        <program name="ig_naming" file="data/eg_weave/ig_naming" %s>
               <memsection name="paged" size="16K" %s />
        </program>
        <pd name="a_pd" %s>
        </pd>
    </iguana>
</image>
"""
        inputs = (
            # Defaults.
            { 'values':
              ('', '', ''),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            { 'values':
              ('pager="none"', '', ''),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            { 'values':
              ('pager="default"', '', ''),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            { 'values':
              ('pager="memload"', '', ''),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            { 'values':
              ('', '', 'pager="memload"'),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            { 'values':
              ('pager="undefined"', '', ''),
              'exit_value': 1,
              'stdout': "",
              'stderr': """Error: "undefined" is not a recognised pager.  Valid values are ('none', 'default', 'memload').\n""",
              },
            { 'values':
              ('', '', 'pager="undefined"'),
              'exit_value': 1,
              'stdout': "",
              'stderr': """Error: "undefined" is not a recognised pager.  Valid values are ('none', 'default', 'memload').\n""",
              },
            { 'values':
              ('', 'pager="none"', ''),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            { 'values':
              ('', 'pager="default"', ''),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            { 'values':
              ('', 'pager="memload"', ''),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            { 'values':
              ('', 'pager="undefined"', ''),
              'exit_value': 1,
              'stdout': "",
              'stderr': """Error: "undefined" is not a recognised pager.  Valid values are ('none', 'default', 'memload').\n""",
              },
            { 'values':
              ('pager="memload"', 'pager="none"', 'pager="default"'),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            )

        for value in inputs:
            exit_value, ret_stdout, ret_stderr = \
                        self._run_command(["-S", template % value['values'], "-o", "test_output"])
            self.assertEquals(ret_stderr, value['stderr'])
            self.assertEquals(ret_stdout, value['stdout'])
            self.assertEquals(exit_value, value['exit_value'])


    def test_bad_machine_spec(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/broken_machine.xml", "-o", "test_output"])
        self.assertEquals(exit_value, 1)
        self.assertEquals(ret_stderr, "Error: Unknown memory rights broken\n")
        self.assertEquals(ret_stdout, "")

    def test_scatter_load(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/scatter_load.xml", "-o", "test_output"])
        self.assertEquals(ret_stderr, "")
        self.assertEquals(ret_stdout, "")
        self.assertEquals(exit_value, 0)
# With dynamic allocation, this check needs to be re-written.
#        self.check_segment_addr("data/eg_weave/scatter_load.xml", "test_output")

    def test_object_environment(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/environment.xml", "-o", "test_output"])
        self.assertEquals(exit_value, 0)
        self.assertEquals(ret_stderr, "")
        self.assertEquals(ret_stdout, "")

    def test_file_inclusion(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/include_common.xml", "-o", "test_output"])
        self.assertEquals(exit_value, 0)
        self.assertEquals(ret_stderr, "")
        self.assertEquals(ret_stdout, "")

    def test_basic_okl4_cell(self):
        #
        # Build line for l4kernel referenced in okl4_cell.xml was:
        # tools/build.py machine=kzm_arm11 project=ktest mutex_type=user
        # ROOT_CAP_NO=512 MAX_THREADS=512
        #
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/okl4_cell.xml", "-o", "test_output"])
        self.assertEquals(ret_stderr, "")
        self.assertEquals(ret_stdout, "")

    def test_nano_simple_cell(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/nano_simple_cell.xml", "-o", "test_output"])
        self.assertEquals(ret_stderr, "")
        self.assertEquals(ret_stdout, "")
        self.assertEquals(exit_value, 0)

    def test_nano_wrong_libokl4_api(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/nano_wrong_api.xml", "-o", "test_output"])

        self.assertEquals(exit_value, 1)
        self.assertEquals(ret_stderr, "Error: Cell \"ig_server\" is built against SDK version 0x2 but the kernel expects version 0x80000000.\n") 
        self.assertEquals(ret_stdout, "")

    def test_micro_wrong_libokl4_api(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/micro_wrong_api.xml", "-o", "test_output"])

        self.assertEquals(exit_value, 1)
        self.assertEquals(ret_stderr, "Error: Cell \"nano_hello\" is built against SDK version 0x80000000 but the kernel expects version 0x2.\n")
        self.assertEquals(ret_stdout, "")

    def test_okl4_cell_args_padding(self):
        """
        Pass one command line arg to an okl4 cell that result in
        various amounts of padding to test that structure remains word
        aligned.
        """
        template = """
<image>
	<machine file="data/eg_weave/machine.xml" />
	<virtual_pool name="virtual">
		<memory base="0x1000" size="0xcffff000"/>
	</virtual_pool>
	<physical_pool name="physical">
		<memory base="0xa0000000" size="0x3800000"/>
	</physical_pool>
	<kernel file="data/eg_weave/l4kernel" virtpool="virtual" physpool="physical">
	</kernel>
	<okl4 name="ctest" file="data/eg_weave/second_cell" kernel_heap="0x200000">
		<commandline>
                    %s
		</commandline>
	</okl4>
</image>
        """
        inputs = (
            # No arguments
            { 'values': (''),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            # No padding.
            { 'values': ('<arg value="npadftw" />'),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            # 1 byte of padding
            { 'values': ('<arg value="padone" />'),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            # 2 bytes of padding
            { 'values': ('<arg value="padw2" />'),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            # 3 bytes of padding
            { 'values': ('<arg value="pad3" />'),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            # 2 args, two bytes of padding.
            { 'values': ('<arg value="pad3" /> <arg value="pad3" />'),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            )

        for value in inputs:
            exit_value, ret_stdout, ret_stderr = \
                    self._run_command(["-S", template % value['values'], "-o", "test_output"])
            self.assertEquals(ret_stderr, value['stderr'])
            self.assertEquals(ret_stdout, value['stdout'])
            self.assertEquals(exit_value, value['exit_value'])

    def test_okl4_memsections(self):
        """
        Test various attributes of okl4 memsections.
        """
        template = """
<image>
	<machine file="data/eg_weave/machine.xml" />
	<virtual_pool name="virtual">
		<memory base="0x1000" size="0xcffff000"/>
	</virtual_pool>
	<physical_pool name="physical">
		<memory base="0xa0000000" size="0x3800000"/>
	</physical_pool>
	<kernel file="data/eg_weave/l4kernel" virtpool="virtual" physpool="physical">
	</kernel>
	<okl4 name="ctest" file="data/eg_weave/ctest" kernel_heap="0x200000">
              <memsection %s />
	</okl4>
</image>
        """
        inputs = (
            # Nothing.  Lots of errors.
            { 'values': (''),
              'exit_value': 1,
              'stdout': "",
              'stderr': 'Error: The required attribute "name" is not specified in element "memsection".\n',
              },
#             # No size
#             { 'values': ('name="test"'),
#               'exit_value': 1,
#               'stdout': "",
#               'stderr': 'Error: No size attribute given for memsection "/ctest/test".\n',
#               },
            # Bad mem_type
            { 'values': ('name="test" size="4K" mem_type="broken"'),
              'exit_value': 1,
              'stdout': "",
              'stderr': 'Error: Unknown memory type "broken".  Supported types are [\'default\', \'physical\', \'unmapped\', \'virtual\']\n',
              },
            # Exercise the supported mem types.
            { 'values': ('name="test" size="4K" mem_type="default"'),
              'exit_value': 0,
              'stdout': "",
              'stderr': '',
              },
            { 'values': ('name="test" size="4K" mem_type="virtual"'),
              'exit_value': 0,
              'stdout': "",
              'stderr': '',
              },
            { 'values': ('name="test" size="4K" mem_type="physical"'),
              'exit_value': 0,
              'stdout': "",
              'stderr': '',
              },
            { 'values': ('name="test" size="4K" mem_type="unmapped"'),
              'exit_value': 0,
              'stdout': "",
              'stderr': '',
              },
            )

        for value in inputs:
            exit_value, ret_stdout, ret_stderr = \
                    self._run_command(["-S", template % value['values'], "-o", "test_output"])
            self.assertEquals(ret_stderr, value['stderr'])
            self.assertEquals(ret_stdout, value['stdout'])
            self.assertEquals(exit_value, value['exit_value'])

    def test_memory_overlap(self):
        """Test address machine address and pool overlap checks."""
        template = """
<image>
	<machine>
		<cpu name="xscale" />
		<word_size size="0x20" />
		<virtual_memory name="virtual">
			<region base="%s" size="%s" />
		</virtual_memory>
		<virtual_memory name="virtual2">
			<region base="%s" size="%s" />
		</virtual_memory>
		<physical_memory name="physical">
			<region base="%s" size="%s" />
		</physical_memory>
		<physical_memory name="physical2">
			<region base="%s" size="%s" />
		</physical_memory>
		<page_size size="0x1000" />
	</machine>
	<virtual_pool name="virtual">
		<memory base="%s" size="%s"/>
	</virtual_pool>
	<virtual_pool name="virtual2">
		<memory base="%s" size="%s"/>
	</virtual_pool>

	<physical_pool name="physical">
		<memory base="%s" size="%s"/>
	</physical_pool>
	<physical_pool name="physical2">
		<memory base="%s" size="%s"/>
	</physical_pool>

	<kernel file="data/eg_weave/l4kernel" virtpool="virtual" physpool="physical">
	</kernel>

	<iguana name="iguana_server" file="data/eg_weave/ig_server" kernel_heap="2M">
	</iguana>
</image>
        """

        inputs = (
            # No memory pools overlap
            { 'values':
              ("0x00000000", "0xa0000000",
               "0xa0000000", "0x60000000",
               "0xa0000000", "0x20000000",
               "0x40000000", "0x20000000",
               "0x00000000", "0xa0000000",
               "0xa0000000", "0x60000000",
               "0xa0000000", "0x20000000",
               "0x40000000", "0x20000000"),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            # One machine virtual range inside another.
            { 'values':
              ("0x00000000", "0x80000000",
               "0x10000000", "0x10000000",
               "0xa0000000", "0x20000000",
               "0x40000000", "0x20000000",
               "0x00000000", "0x80000000",
               "0x10000000", "0x10000000",
               "0xa0000000", "0x20000000",
               "0x40000000", "0x20000000"),
              'exit_value': 1,
              'stdout': "",
              'stderr': 'Error: The machine memory region 0x10000000--0x1fffffff (size 0x10000000) in "virtual2" overlaps with region 0x0--0x7fffffff (size 0x80000000) in "virtual".\n',
              },
            # One machine virtual range base address is in another range.
            { 'values':
              ("0x00000000", "0x80000000",
               "0x10000000", "0x80000000",
               "0xa0000000", "0x20000000",
               "0x40000000", "0x20000000",
               "0x00000000", "0x80000000",
               "0x10000000", "0x80000000",
               "0xa0000000", "0x20000000",
               "0x40000000", "0x20000000"),
              'exit_value': 1,
              'stdout': "",
              'stderr': 'Error: The machine memory region 0x10000000--0x8fffffff (size 0x80000000) in "virtual2" overlaps with region 0x0--0x7fffffff (size 0x80000000) in "virtual".\n',
              },
            # One machine virtual range end address is in another range.
            { 'values':
              ("0x50000000", "0x80000000",
               "0x10000000", "0x80000000",
               "0xa0000000", "0x20000000",
               "0x40000000", "0x20000000",
               "0x50000000", "0x80000000",
               "0x10000000", "0x80000000",
               "0xa0000000", "0x20000000",
               "0x40000000", "0x20000000"),
              'exit_value': 1,
              'stdout': "",
              'stderr': 'Error: The machine memory region 0x50000000--0xcfffffff (size 0x80000000) in "virtual" overlaps with region 0x10000000--0x8fffffff (size 0x80000000) in "virtual2".\n',
              },
            # One machine physical range inside another.
            { 'values':
              ("0x00000000", "0x80000000",
               "0x80000000", "0x80000000",
               "0xa0000000", "0x40000000",
               "0xa1000000", "0x20000000",
               "0x00000000", "0x80000000",
               "0x80000000", "0x80000000",
               "0xa0000000", "0x40000000",
               "0xa1000000", "0x20000000"),
              'exit_value': 1,
              'stdout': "",
              'stderr': 'Error: The machine memory region 0xa1000000--0xc0ffffff (size 0x20000000) in "physical2" overlaps with region 0xa0000000--0xdfffffff (size 0x40000000) in "physical".\n',
              },
            # One machine physical range base address is in another range.
            { 'values':
              ("0x00000000", "0x80000000",
               "0x80000000", "0x80000000",
               "0xa0000000", "0x40000000",
               "0xa1000000", "0x40000000",
               "0x00000000", "0x80000000",
               "0x80000000", "0x80000000",
               "0xa0000000", "0x40000000",
               "0xa1000000", "0x40000000"),
              'exit_value': 1,
              'stdout': "",
              'stderr': 'Error: The machine memory region 0xa1000000--0xe0ffffff (size 0x40000000) in "physical2" overlaps with region 0xa0000000--0xdfffffff (size 0x40000000) in "physical".\n',
              },
            # One machine physical range end address is in another range.
            { 'values':
              ("0x00000000", "0x80000000",
               "0x80000000", "0x80000000",
               "0xa0000000", "0x40000000",
               "0x9f000000", "0x20000000",
               "0x00000000", "0x80000000",
               "0x80000000", "0x80000000",
               "0xa0000000", "0x40000000",
               "0x9f000000", "0x20000000"),
              'exit_value': 1,
              'stdout': "",
              'stderr': 'Error: The machine memory region 0xa0000000--0xdfffffff (size 0x40000000) in "physical" overlaps with region 0x9f000000--0xbeffffff (size 0x20000000) in "physical2".\n',
              },
            # One virtual pool range inside another.
            { 'values':
              ("0x00000000", "0x80000000",
               "0x80000000", "0x80000000",
               "0xa0000000", "0x20000000",
               "0x40000000", "0x20000000",
               "0x00000000", "0x80000000",
               "0x10000000", "0x10000000",
               "0xa0000000", "0x20000000",
               "0x40000000", "0x20000000"),
              'exit_value': 1,
              'stdout': "",
              'stderr': 'Error: Virtual Pools: Parts of "virtual2" (0x10000000-0x1fffffff, size 0x10000000) overlap with "virtual" (0x0-0x7fffffff, size 0x80000000).\n',
              },
            # One virtual pool range base address is in another pool.
            { 'values':
              ("0x00000000", "0x80000000",
               "0x80000000", "0x80000000",
               "0xa0000000", "0x20000000",
               "0x40000000", "0x20000000",
               "0x00000000", "0x80000000",
               "0x10000000", "0x80000000",
               "0xa0000000", "0x20000000",
               "0x40000000", "0x20000000"),
              'exit_value': 1,
              'stdout': "",
              'stderr': 'Error: Virtual Pools: Parts of "virtual2" (0x10000000-0x8fffffff, size 0x80000000) overlap with "virtual" (0x0-0x7fffffff, size 0x80000000).\n',
              },
            # One virtual pool range end address is in another pool.
            { 'values':
              ("0x00000000", "0x80000000",
               "0x80000000", "0x80000000",
               "0xa0000000", "0x20000000",
               "0x40000000", "0x20000000",
               "0x50000000", "0x80000000",
               "0x10000000", "0x80000000",
               "0xa0000000", "0x20000000",
               "0x40000000", "0x20000000"),
              'exit_value': 1,
              'stdout': "",
              'stderr': 'Error: Virtual Pools: Parts of "virtual2" (0x10000000-0x8fffffff, size 0x80000000) overlap with "virtual" (0x50000000-0xcfffffff, size 0x80000000).\n',
              },
            # One physical pool range inside another.
            { 'values':
              ("0x00000000", "0x80000000",
               "0x80000000", "0x80000000",
               "0xa0000000", "0x40000000",
               "0x40000000", "0x20000000",
               "0x00000000", "0x80000000",
               "0x80000000", "0x80000000",
               "0xa0000000", "0x40000000",
               "0xa1000000", "0x20000000"),
              'exit_value': 1,
              'stdout': "",
              'stderr': 'Error: Physical Pools: Parts of "physical2" (0xa1000000-0xc0ffffff, size 0x20000000) overlap with "physical" (0xa0000000-0xdfffffff, size 0x40000000).\n',
              },
            # One physical pool range base address inside another pool.
            { 'values':
              ("0x00000000", "0x80000000",
               "0x80000000", "0x80000000",
               "0xa0000000", "0x40000000",
               "0x40000000", "0x40000000",
               "0x00000000", "0x80000000",
               "0x80000000", "0x80000000",
               "0xa0000000", "0x40000000",
               "0xa1000000", "0x40000000"),
              'exit_value': 1,
              'stdout': "",
              'stderr': 'Error: Physical Pools: Parts of "physical2" (0xa1000000-0xe0ffffff, size 0x40000000) overlap with "physical" (0xa0000000-0xdfffffff, size 0x40000000).\n',
              },
            # One physical pool range end address inside another pool.
            { 'values':
              ("0x00000000", "0x80000000",
               "0x80000000", "0x80000000",
               "0xa0000000", "0x20000000",
               "0x40000000", "0x20000000",
               "0x00000000", "0x80000000",
               "0x80000000", "0x80000000",
               "0xa0000000", "0x40000000",
               "0x9f000000", "0x20000000"),
              'exit_value': 1,
              'stdout': "",
              'stderr': 'Error: Physical Pools: Parts of "physical2" (0x9f000000-0xbeffffff, size 0x20000000) overlap with "physical" (0xa0000000-0xdfffffff, size 0x40000000).\n',
              },
            )
        for value in inputs:
            exit_value, ret_stdout, ret_stderr = \
                        self._run_command(["-S", template % value['values'], "-o", "test_output"])
            self.assertEquals(ret_stderr, value['stderr'])
            self.assertEquals(ret_stdout, value['stdout'])
            self.assertEquals(exit_value, value['exit_value'])

    def test_segment_direct(self):
        """
        Test various combinations of directly mapping every segment
        in a program.
        """

        template = """
<image>
	<machine>
		<cpu name="xscale" />
		<word_size size="0x20" />
		<virtual_memory name="virtual">
			<region base="0x1000" size="0xcffff000" />
		</virtual_memory>
		<physical_memory name="physical">
			<region base="%s" size="%s" />
		</physical_memory>
		<page_size size="0x1000" />
	</machine>
	<virtual_pool name="virtual">
		<memory src="virtual"/>
	</virtual_pool>

	<physical_pool name="physical" direct="%s">
		<memory src="physical" />
	</physical_pool>

	<kernel file="data/eg_weave/l4kernel" virtpool="virtual" physpool="physical">
	</kernel>

	<iguana name="iguana_server" file="data/eg_weave/ig_server" kernel_heap="2M">
        <program name="ig_naming" file="data/eg_weave/ig_naming" direct="%s" >
        </program>
    </iguana>
</image>
        """

        inputs = (
            # No direct segment mapping.
            { 'values':
              ("0xa0000000", "0x3800000", "false", "false"),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            # Direct mapping.
            { 'values':
              ("0x80000000", "0x3800000", "true", "true"),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            # Ask for direct mapping but the physpool doesn't support
            # it.
            { 'values':
              ("0x80000000", "0x3800000", "false", "true"),
              'exit_value': 1,
              'stdout': "",
              'stderr': 'Error: Physical pool "physical" does not support direct memory allocation.\n',
              },
            # Ask for direct mapping, but the address range is not in
            # the pool.
            { 'values':
              ("0xa0000000", "0x3800000", "true", "true"),
              'exit_value': 1,
              'stdout': "",
              'stderr': 'Error: Segment "/iguana_server/ig_naming/rx": Cannot reserve physical addresses 0x80100000--0x80107e55.\n',
              },
            )

        for value in inputs:
            exit_value, ret_stdout, ret_stderr = \
                        self._run_command(["-S", template % value['values'], "-o", "test_output"])
            self.assertEquals(ret_stderr, value['stderr'])
            self.assertEquals(ret_stdout, value['stdout'])
            self.assertEquals(exit_value, value['exit_value'])


    def test_segment_attrs(self):
        """
        Test reported problems with segment elements referring to
        segments that are not in the ELF file, and tests for virtual
        and physical memory overlap.
        """

        template = """
<image>
	<machine>
		<cpu name="xscale" />
		<word_size size="0x20" />
		<virtual_memory name="virtual">
			<region base="0x1000" size="0xcffff000" />
		</virtual_memory>
		<physical_memory name="physical">
			<region base="0xa0000000" size="0x3800000" />
		</physical_memory>
		<page_size size="0x1000" />
	</machine>
	<virtual_pool name="virtual">
		<memory src="virtual"/>
	</virtual_pool>

	<physical_pool name="physical">
		<memory src="physical" />
	</physical_pool>

	<kernel file="data/eg_weave/l4kernel" virtpool="virtual" physpool="physical">
	</kernel>

	<iguana name="iguana_server" file="data/eg_weave/ig_server" kernel_heap="2M">
        <program name="ig_naming" file="data/eg_weave/ig_naming">
        %s
        </program>
    </iguana>
</image>
        """

        inputs = (
            # Refer to a non-existent segment name.
            { 'values':
              ('<segment name="non_existent" />'),
              'exit_value': 1,
              'stdout': "",
              'stderr': 'Error: /iguana_server/ig_naming: Cannot find segment "non_existent" in the ELF file. Valid values are [\'rx\', \'rw\']\n',
              },
            # Overlap the segments in physical memory.
            { 'values':
              ('<segment name="rx" phys_addr="0xa2000000" /> <segment name="rw" phys_addr="0xa2000000" />'),
              'exit_value': 1,
              'stdout': "",
              'stderr': 'Error: Physical Memory: Parts of "/iguana_server/ig_naming/rw" (0xa2000000-0xa2000153, size 0x154) overlap with "/iguana_server/ig_naming/rx" (0xa2000000-0xa2007fff, size 0x8000).\n',
              },
            # Overlap a segment and memsection in virtual memory.
            { 'values':
              ('<memsection name="overlap" size="1M" virt_addr="0x80100000" />'),
              'exit_value': 1,
              'stdout': "",
              'stderr': 'Error: Virtual Memory: Parts of "/iguana_server/ig_naming/overlap" (0x80100000-0x801fffff, size 0x100000) overlap with "/iguana_server/ig_naming/rx" (0x80100000-0x80107fff, size 0x8000).\n',
              },
            )

        for value in inputs:
            exit_value, ret_stdout, ret_stderr = \
                        self._run_command(["-S", template % value['values'], "-o", "test_output"])
            self.assertEquals(ret_stderr, value['stderr'])
            self.assertEquals(ret_stdout, value['stdout'])
            self.assertEquals(exit_value, value['exit_value'])

    def test_kernel_proximity(self):
        """Test the kernel heap proximity checking code."""
        template = """
<image>
	<machine>
		<cpu name="xscale" />
		<word_size size="0x20" />
		<virtual_memory name="virtual">
			<region base="0x80000000" size="0x50000000" />
		</virtual_memory>
		<physical_memory name="physical">
			<region base="0xa0000000" size="0x3800000" />
		</physical_memory>
		<page_size size="0x1000" />
		<page_size size="0x10000" />
		<page_size size="0x100000" />
                %s
	</machine>

	<virtual_pool name="virtual">
		<memory base="0x80000000" size="0x50000000"/>
	</virtual_pool>

	<physical_pool name="physical">
		<memory base="0xa0000000" size="0x3800000"/>
	</physical_pool>

	<kernel file="data/eg_weave/l4kernel" virtpool="virtual" physpool="physical">
        %s
	</kernel>

	<iguana name="iguana_server" file="data/eg_weave/ig_server" kernel_heap="2M">
	</iguana>
</image>
        """

        inputs = (
            # Defaults.  Should be within 64M of kernel.
            { 'values':
              ('', ''),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },

            # Proximity equal to the default location.
            { 'values':
              ('<kernel_heap_attrs distance="1M" />', ''),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },

            # Very close proximity.
            { 'values':
              ('<kernel_heap_attrs distance="64K" />', ''),
              'exit_value': 1,
              'stdout': "",
              'stderr': 'Error: Physical pool "physical": Could not place /kernel_heap within 0x10000 bytes of /kernel/rwx.\n',
              },

            # Heap outside the default size
            { 'values':
              ('', '<heap phys_addr="0xc0000000" />'),
              'exit_value': 1,
              'stdout': "",
              'stderr': 'Error: Physical pool "physical": Could not place /kernel_heap within 0x4000000 bytes of /kernel/rwx.\n'
              },
            )

        for value in inputs:
            exit_value, ret_stdout, ret_stderr = \
                        self._run_command(["-S", template % value['values'], "-o", "test_output"])
            self.assertEquals(ret_stderr, value['stderr'])
            self.assertEquals(ret_stdout, value['stdout'])
            self.assertEquals(exit_value, value['exit_value'])

    def test_kernel_alignment(self):
        """Test the kernel segments alignment."""
        template = """
<image>
	<machine>
		<cpu name="xscale" />
		<word_size size="0x20" />
		<virtual_memory name="virtual">
			<region base="0x80000000" size="0x50000000" />
		</virtual_memory>
		<physical_memory name="physical">
			<region base="0xa0000000" size="0x3800000" />
		</physical_memory>
		<page_size size="0x1000" />
		<page_size size="0x10000" />
		<page_size size="0x100000" />
	</machine>

	<virtual_pool name="virtual">
		<memory base="0x80000000" size="0x50000000"/>
	</virtual_pool>

	<physical_pool name="physical">
		<memory base="0xa0000000" size="0x3800000"/>
	</physical_pool>

	<kernel file="data/eg_weave/l4kernel" virtpool="virtual" physpool="physical">
        %s
	</kernel>

	<iguana name="iguana_server" file="data/eg_weave/ig_server" kernel_heap="2M">
	</iguana>
</image>
        """

        inputs = (
            { 'values':
              (''),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },

            # changing the phys_addr of the segments should work...
            { 'values':
              ('<segment phys_addr="0xa0700000" name="rwx" />\n<segment phys_addr="0xa072c000" name="rw" />'),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },

            # but it must fail if not 1MB aligned.
            { 'values':
              ('<segment phys_addr="0xa0080000" name="rwx" />\n<segment phys_addr="0xa002a000" name="rw" />'),
              'exit_value': 1,
              'stdout': "",
              'stderr': "Error: Physical address of /kernel/rwx must be 1MB aligned!\n",
              },

            )

        for value in inputs:
            exit_value, ret_stdout, ret_stderr = \
                        self._run_command(["-S", template % value['values'], "-o", "test_output"])
            self.assertEquals(ret_stderr, value['stderr'])
            self.assertEquals(ret_stdout, value['stdout'])        
            self.assertEquals(exit_value, value['exit_value'])

    def test_kernel_heap_alignment(self):
        """
        Test various combinations of directly mapping every segment
        in a program.
        """

        template = """
<image>
	<machine>
		<cpu name="xscale" />
		<word_size size="0x20" />
		<virtual_memory name="virtual">
			<region base="0x1000" size="0xcffff000" />
		</virtual_memory>
		<physical_memory name="physical">
			<region base="0xa0000000" size="0x3800000" />
		</physical_memory>
		<page_size size="0x1000" />
		<page_size size="0x100000" />

                %s
	</machine>
	<virtual_pool name="virtual">
		<memory base="0x1000" size="0xcffff000"/>
	</virtual_pool>

	<physical_pool name="physical">
		<memory base="0xa0000000" size="0x3800000"/>
	</physical_pool>

	<kernel file="data/eg_weave/l4kernel" virtpool="virtual" physpool="physical">
        <heap size="4M" />
	</kernel>

	<iguana name="iguana_server" file="data/eg_weave/ig_server" kernel_heap="2M">
        <program name="ig_naming" file="data/eg_weave/ig_naming">
        </program>
    </iguana>
</image>
        """

        inputs = (
            # Default alignment.  Heap 1M from the kernel.
            { 'values': (""),
              'exit_value': 0,
              'stdout': """\
VIRTUAL:
  <00001000:00001fff> /iguana_server/bootinfo
  <00002000:00002fff> /initscript
  <00010000:0001ffff> /iguana_server/utcb_area
  <80000000:8000ccc8> /iguana_server/rx
  <80014ccc:80014f77> /iguana_server/rw
  <80015000:80015175> /iguana_server/cell_environment
  <80016000:800161a2> /iguana_server/environment
  <80017000:80017fff> /iguana_server/main/stack
  <80020000:8002ffff> /iguana_server/heap
  <80100000:80107e55> /iguana_server/ig_naming/rx
  <80108000:801082a2> /iguana_server/ig_naming/environment
  <80109000:80109fff> /iguana_server/ig_naming/callback
  <8010fe58:8010ffab> /iguana_server/ig_naming/rw
  <80110000:8011ffff> /iguana_server/ig_naming/heap
  <80120000:80120fff> /iguana_server/ig_naming/main/stack
  <f0000000:f002810b> /kernel/rwx
  <f002c000:f0031fff> /kernel/rw
PHYSICAL:
  <a0000000:a002810b> /kernel/rwx
  <a0029ccc:a0029f77> /iguana_server/rw
  <a002ae58:a002afab> /iguana_server/ig_naming/rw
  <a002b000:a002bfff> /iguana_server/bootinfo
  <a002c000:a0031fff> /kernel/rw
  <a0032000:a0032fff> /initscript
  <a0038000:a0044cc8> /iguana_server/rx
  <a0045000:a0045175> /iguana_server/cell_environment
  <a0046000:a00461a2> /iguana_server/environment
  <a0047000:a0047fff> /iguana_server/main/stack
  <a0048000:a004fe55> /iguana_server/ig_naming/rx
  <a0050000:a00502a2> /iguana_server/ig_naming/environment
  <a0051000:a0051fff> /iguana_server/ig_naming/callback
  <a0060000:a006ffff> /iguana_server/ig_naming/heap
  <a0070000:a0070fff> /iguana_server/ig_naming/main/stack
  <a0080000:a008ffff> /iguana_server/heap
  <a0100000:a04fffff> /kernel_heap
""",
              'stderr': "",
              },
            # 4K alignment.  Heap next to the kernel.
            { 'values': ('<kernel_heap_attrs align="4K" />'),
              'exit_value': 0,
              'stdout': """\
VIRTUAL:
  <00001000:00001fff> /iguana_server/bootinfo
  <00002000:00002fff> /initscript
  <00010000:0001ffff> /iguana_server/utcb_area
  <80000000:8000ccc8> /iguana_server/rx
  <80014ccc:80014f77> /iguana_server/rw
  <80015000:80015175> /iguana_server/cell_environment
  <80016000:800161a2> /iguana_server/environment
  <80017000:80017fff> /iguana_server/main/stack
  <80020000:8002ffff> /iguana_server/heap
  <80100000:80107e55> /iguana_server/ig_naming/rx
  <80108000:801082a2> /iguana_server/ig_naming/environment
  <80109000:80109fff> /iguana_server/ig_naming/callback
  <8010fe58:8010ffab> /iguana_server/ig_naming/rw
  <80110000:8011ffff> /iguana_server/ig_naming/heap
  <80120000:80120fff> /iguana_server/ig_naming/main/stack
  <f0000000:f002810b> /kernel/rwx
  <f002c000:f0031fff> /kernel/rw
PHYSICAL:
  <a0000000:a002810b> /kernel/rwx
  <a0029ccc:a0029f77> /iguana_server/rw
  <a002ae58:a002afab> /iguana_server/ig_naming/rw
  <a002b000:a002bfff> /iguana_server/bootinfo
  <a002c000:a0031fff> /kernel/rw
  <a0032000:a0431fff> /kernel_heap
  <a0432000:a0432fff> /initscript
  <a0438000:a0444cc8> /iguana_server/rx
  <a0445000:a0445175> /iguana_server/cell_environment
  <a0446000:a04461a2> /iguana_server/environment
  <a0447000:a0447fff> /iguana_server/main/stack
  <a0448000:a044fe55> /iguana_server/ig_naming/rx
  <a0450000:a04502a2> /iguana_server/ig_naming/environment
  <a0451000:a0451fff> /iguana_server/ig_naming/callback
  <a0460000:a046ffff> /iguana_server/ig_naming/heap
  <a0470000:a0470fff> /iguana_server/ig_naming/main/stack
  <a0480000:a048ffff> /iguana_server/heap
""",
              'stderr': "",
              },
            )

        for value in inputs:
            exit_value, ret_stdout, ret_stderr = \
                        self._run_command(["-S", template % value['values'], "--map", "-o", "test_output"])
            self.assertEquals(ret_stderr, value['stderr'])
            self.assertEquals(ret_stdout, value['stdout'])
            self.assertEquals(exit_value, value['exit_value'])

    def test_ignore_prog(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/ignore_program_test.xml", "-i", "(vmlinu.)|(ig_serial)|(ig_timer)", "-o", "test_output"])
        self.assertEquals(exit_value, 0)
        self.assertEquals(ret_stderr, "")

        if not have_readelf:
            return

        # check excluded programs are not in the elf
        status, output = commands.getstatusoutput("readelf -e test_output|grep 'vmlinu'")
        self.assertNotEquals(status, 0)
        self.assertEquals(output, "")
        status, output = commands.getstatusoutput("readelf -e test_output|grep 'ig_serial'")
        self.assertNotEquals(status, 0)
        self.assertEquals(output, "")
        status, output = commands.getstatusoutput("readelf -e test_output|grep 'ig_timer'")
        self.assertNotEquals(status, 0)
        self.assertEquals(output, "")

        #check the other stuff is still included
        status, output = commands.getstatusoutput("readelf -e test_output|grep 'ig_nam'")
        self.assertEquals(status, 0)
        self.assertNotEquals(output, "")
        status, output = commands.getstatusoutput("readelf -e test_output|grep 'kernel'")
        self.assertEquals(status, 0)
        self.assertNotEquals(output, "")
        status, output = commands.getstatusoutput("readelf -e test_output|grep 'server'")
        self.assertEquals(status, 0)
        self.assertNotEquals(output, "")
        status, output = commands.getstatusoutput("readelf -e test_output|grep 'bootinfo'")
        self.assertEquals(status, 0)
        self.assertNotEquals(output, "")
#         status, output = commands.getstatusoutput("readelf -e test_output|grep 'segment_info'")
#         self.assertEquals(status, 0)
#         self.assertNotEquals(output, "")

    def test_program_header_offset(self):
        """Test the the ---program-header-offset option."""
        if not have_readelf:
            return

        # Check that a default merge puts the headers at the minimum offset,
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/simple.xml", "-o", "test_output"])
        self.assertEquals(exit_value, 0)
        self.assertEquals(ret_stderr, "")
        status, output = commands.getstatusoutput("readelf -e test_output|grep 'Start of program headers'")
        self.assertEquals(status, 0)
        self.assertEquals(output, "  Start of program headers:          52 (bytes into file)")

        exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/simple.xml", "--program-header-offset=4096", "-o", "test_output"])
        self.assertEquals(exit_value, 0)
        self.assertEquals(ret_stderr, "")
        status, output = commands.getstatusoutput("readelf -e test_output|grep 'Start of program headers'")
        self.assertEquals(status, 0)
        self.assertEquals(output, "  Start of program headers:          4096 (bytes into file)")

    def test_dump_layout(self):
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/multiple_elf_file.xml", "--map", "-o", "test_output"])
        output = """\
VIRTUAL:
  <00001000:00001fff> /iguana_server/bootinfo
  <00002000:00011fff> /iguana_server/utcb_area
  <00012000:00012fff> /initscript
  <80000000:8000ccc8> /iguana_server/rx
  <80014ccc:80014f77> /iguana_server/rw
  <80015000:80015175> /iguana_server/cell_environment
  <80016000:800161c9> /iguana_server/environment
  <80017000:80017fff> /iguana_server/main/stack
  <80018000:80027fff> /iguana_server/heap
  <80100000:80107e55> /iguana_server/ig_naming/rx
  <80108000:801082c9> /iguana_server/ig_naming/environment
  <80109000:80109fff> /iguana_server/ig_naming/callback
  <8010fe58:8010ffab> /iguana_server/ig_naming/rw
  <80110000:8011ffff> /iguana_server/ig_naming/heap
  <80120000:80120fff> /iguana_server/ig_naming/main/stack
  <f0000000:f002810b> /kernel/rwx
  <f002c000:f0031fff> /kernel/rw
PHYSICAL:
  <a0000000:a002810b> /kernel/rwx
  <a0029ccc:a0029f77> /iguana_server/rw
  <a002ae58:a002afab> /iguana_server/ig_naming/rw
  <a002b000:a002bfff> /iguana_server/bootinfo
  <a002c000:a0031fff> /kernel/rw
  <a0032000:a0431fff> /kernel_heap
  <a0432000:a043ecc8> /iguana_server/rx
  <a043f000:a0446e55> /iguana_server/ig_naming/rx
  <a0447000:a04472c9> /iguana_server/ig_naming/environment
  <a0448000:a0448fff> /iguana_server/ig_naming/callback
  <a0449000:a0458fff> /iguana_server/ig_naming/heap
  <a0459000:a0459fff> /iguana_server/ig_naming/main/stack
  <a045a000:a045a175> /iguana_server/cell_environment
  <a045b000:a045b1c9> /iguana_server/environment
  <a045c000:a045cfff> /iguana_server/main/stack
  <a045d000:a046cfff> /iguana_server/heap
  <a046d000:a046dfff> /initscript
"""
        self.assertEquals(exit_value, 0)
        self.assertEquals(ret_stderr, "")
        self.assertEquals(ret_stdout, output)

    def test_last_phys(self):
        "Test the -l option that prints the last physical address."
        exit_value, ret_stdout, ret_stderr = self._run_command(["data/eg_weave/multiple_elf_file.xml", "-l", "-o", "test_output"])
        self.assertEquals(exit_value, 0)
        self.assertEquals(ret_stderr, "")
        self.assertEquals(ret_stdout, "physical: 0xa046e000\n")

    def test_zones(self):
        """Simple tests for memory zones."""
        template = """
<image>
        <machine>
                <cpu name="xscale" />
                <word_size size="0x20" />
                <virtual_memory name="virtual">
                        <region base="0x1000" size="0xcffff000" />
                </virtual_memory>
                <physical_memory name="physical">
                        <region base="0xa0000000" size="0x3800000" />
                </physical_memory>
                <page_size size="0x1000" />
        </machine>
        <virtual_pool name="virtual">
                <memory base="0x1000" size="0xcffff000"/>
        </virtual_pool>

        <physical_pool name="physical">
                <memory base="0xa0000000" size="0x3800000"/>
        </physical_pool>

        <kernel file="data/eg_weave/l4kernel" virtpool="virtual" physpool="physical">
        </kernel>

        <iguana name="iguana_server" file="data/eg_weave/ig_server" kernel_heap="2M">
            <program name="ig_naming" file="data/eg_weave/ig_naming">
               %s
            </program>
        </iguana>
</image>
        """

        inputs = (
            # Simple zone, occupying the whole of a window.
            { 'values':
"""
                <zone name="z1" >
                        <memsection name="zoned_ms" size="0x100000" virt_addr="0x80600000" align="1048576"  />
                </zone>
""",
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            # Small memsection occupying a fixed address at the base
            # of a zone.
            { 'values':
"""
                <zone name="z1" >
                        <memsection name="zoned_ms" size="0x1000" virt_addr="0x80600000" align="1048576"  />
                </zone>
""",
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            # Small memsection to be allocated dynamically.
            { 'values':
"""
                <zone name="z1" >
                        <memsection name="zoned_ms" size="0x1000"  />
                </zone>
""",
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            # Multiple dynamically allocated memsections.
            { 'values':
"""
                <zone name="z1" >
                        <memsection name="zoned_ms" size="0x1000"  />
                        <memsection name="zoned_ms2" size="0x1000"  />
                        <memsection name="zoned_ms3" size="0x1000"  />
                </zone>
""",
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            # Multiple fixed zones in the same window.
            # of a zone.
            { 'values':
"""
                <zone name="z1" >
                        <memsection name="zoned_ms" size="0x1000" virt_addr="0x80600000"  />
                        <memsection name="zoned_ms1" size="0x1000" virt_addr="0x80604000"  />
                        <memsection name="zoned_ms2" size="0x1000" virt_addr="0x80608000"  />
                        <memsection name="zoned_ms3" size="0x1000" virt_addr="0x80680000"  />
                </zone>
""",
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            # Multiple zones in the same program
            { 'values':
"""
                <zone name="z1" >
                        <memsection name="zoned_ms" size="0x1000"  />
                </zone>
                <zone name="z2" >
                        <memsection name="zoned_ms" size="0x1000"  />
                </zone>
""",
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            )

        for value in inputs:
            exit_value, ret_stdout, ret_stderr = \
                        self._run_command(["-S", template % value['values'], "-o", "test_output"])
            self.assertEquals(ret_stderr, value['stderr'])
            self.assertEquals(ret_stdout, value['stdout'])
            self.assertEquals(exit_value, value['exit_value'])

    def test_cache_rights(self):
        """Test the cache rights code."""
        template = """
<image>
    <machine>
        <cpu name="xscale" />
        <word_size size="0x20" />
        <virtual_memory name="virtual">
            <region base="0x1000" size="0xcffff000" />
        </virtual_memory>
        <physical_memory name="physical_one">
            <region base="0xa0000000" size="0x2000000" rights="%s" />
        </physical_memory>
        <physical_memory name="physical_two">
            <region base="0xa2000000" size="0x1800000" rights="%s" />
        </physical_memory>
        <page_size size="0x1000" />
    </machine>
    <virtual_pool name="virtual">
        <memory base="0x1000" size="0xcffff000" />
    </virtual_pool>

    <physical_pool name="physical_one">
        <memory src="physical_one" />
    </physical_pool>
    <physical_pool name="physical_two">
        <memory src="physical_two" />
    </physical_pool>

    <kernel file="data/eg_weave/l4kernel" virtpool="virtual" physpool="%s">
    </kernel>

    <iguana name="iguana_server" file="data/eg_weave/ig_server" kernel_heap="2M" physpool="%s">
        <segment name="rw" cache_policy="%s" />
        <segment name="rx" cache_policy="%s" />
    </iguana>
</image>
        """

        inputs = (
            { 'values': ('all', 'all', 'physical_one',
                         'physical_two', 'writeback', 'writeback'),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },

            { 'values': ('uncached', 'all', 'physical_one',
                         'physical_two', 'writeback', 'writeback'),
              'exit_value': 1,
              'stdout': "",
              'stderr': "Error: Physical pool \"physical_one\": Trying to allocate \"/kernel/rwx\": Cache policy \"writeback\" is not allowed by the cache rights \"uncached\"\n",
              },

            { 'values': ('all', 'writethrough, cached, uncached', 'physical_one',
                         'physical_two', 'writeback', 'writeback'),
              'exit_value': 1,
              'stdout': "",
              'stderr': "Error: Physical pool \"physical_two\": Trying to allocate \"/iguana_server/rx\": Cache policy \"writeback\" is not allowed by the cache rights \"cached, uncached, writethrough\"\n",
              },

            { 'values': ('all', 'writeback, uncached', 'physical_one',
                         'physical_two', 'writeback', 'writeback'),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },

            { 'values': ('all', 'writeback, uncached', 'physical_one',
                         'physical_two', 'writeback', 'writethrough'),
              'exit_value': 1,
              'stdout': "",
              'stderr': "Error: Physical pool \"physical_two\": Trying to allocate \"/iguana_server/rx\": Cache policy \"writethrough\" is not allowed by the cache rights \"writeback, uncached\"\n",
              },

            { 'values': ('writeback, writethrough, cached, uncached, shared, strong',
                         'writeback, uncached', 'physical_one',
                         'physical_two', 'writeback', 'writeback'),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },

            )

        for value in inputs:
            exit_value, ret_stdout, ret_stderr = \
                        self._run_command(["-S", template % value['values'], "-o", "test_output"])
            self.assertEquals(ret_stderr, value['stderr'])
            self.assertEquals(ret_stdout, value['stdout'])        
            self.assertEquals(exit_value, value['exit_value'])

    def test_cache_policies(self):
        """Test the cache policy code."""
        template = """
<image>
	<machine>
		<cpu name="xscale" />
		<word_size size="0x20" />

                <cache_policy name="user1" value="2" />
                <cache_policy name="user2" value="3" />

		<virtual_memory name="virtual">
			<region base="0x1000" size="0xcffff000" />
		</virtual_memory>
		<physical_memory name="physical">
			<region base="0xa0000000" size="0x3800000" />
		</physical_memory>
		<page_size size="0x1000" />
	</machine>
	<virtual_pool name="virtual">
		<memory base="0x1000" size="0xcffff000"/>
	</virtual_pool>

	<physical_pool name="physical">
		<memory base="0xa0000000" size="0x3800000"/>
	</physical_pool>

	<kernel file="data/eg_weave/l4kernel" virtpool="virtual" physpool="physical">
	</kernel>

	<iguana name="iguana_server" file="data/eg_weave/ig_server" kernel_heap="2M">
        <program name="ig_naming" file="data/eg_weave/ig_naming">
        	<segment name="rx" cache_policy="%s" />
        	<memsection name="demo" size="4K" cache_policy="%s" />
        </program>
    </iguana>
</image>
        """

        inputs = (
            # Iterate through the standard cache policies.  All should
            # work.
            { 'values': ('default', 'default'),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },

            { 'values': ('cached', 'cached'),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },

            { 'values': ('uncached', 'uncached'),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },

            { 'values': ('writeback', 'writeback'),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },

            { 'values': ('writethrough', 'writethrough'),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },

            { 'values': ('coherent', 'coherent'),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },

            { 'values': ('device', 'device'),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },

            { 'values': ('writecombining', 'writecombining'),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            # Test the user specified cache policies.
            { 'values': ('user1', 'user1'),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },

            { 'values': ('user2', 'user2'),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            # Test a non-existent policy name.
            { 'values': ('rubbish', 'rubbish'),
              'exit_value': 1,
              'stdout': "",
              'stderr': "Error: Unknown cache policy: 'rubbish'.\n",
              },
            )

        for value in inputs:
            exit_value, ret_stdout, ret_stderr = \
                        self._run_command(["-S", template % value['values'], "-o", "test_output"])
            self.assertEquals(ret_stderr, value['stderr'])
            self.assertEquals(ret_stdout, value['stdout'])
            self.assertEquals(exit_value, value['exit_value'])

    def test_max_priority(self):
        """Test the max priority checking code."""
        template = """
<image>
	<machine>
		<cpu name="xscale" />
		<word_size size="0x20" />
		<virtual_memory name="virtual">
			<region base="0x1000" size="0xcffff000" />
		</virtual_memory>
		<physical_memory name="physical">
			<region base="0xa0000000" size="0x3800000" />
		</physical_memory>
		<page_size size="0x1000" />
	</machine>
	<virtual_pool name="virtual">
		<memory base="0x1000" size="0xcffff000"/>
	</virtual_pool>

	<physical_pool name="physical">
		<memory base="0xa0000000" size="0x3800000"/>
	</physical_pool>

	<kernel file="data/eg_weave/l4kernel" virtpool="virtual" physpool="physical">
	</kernel>

    <okl4 %s max_priority="%s" file="data/eg_weave/second_cell" kernel_heap="0x200000" name="ktest">
        <space name="thread_space" %s>
            <thread name="test_thread" start="_entry" %s/>
        </space>
    </okl4>

</image>
        """

        inputs = (
            #         prio, max_prio, string max prio, prio
            { 'values': ('priority="100"', '255', '', 'priority="255"'),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            #         prio, max_prio, string max prio, prio
            { 'values': ('priority="100"', '100', '', 'priority="120"'),
              'exit_value': 1,
              'stdout': "",
              'stderr': "Error: Tried to create thread with priority 120, but space (/ktest/thread_space) max priority is 100.\n",
              },
            #         prio, max_prio, string max prio, prio
            { 'values': ('priority="100"', '100', '', 'priority="101"'),
              'exit_value': 1,
              'stdout': "",
              'stderr': "Error: Tried to create thread with priority 101, but space (/ktest/thread_space) max priority is 100.\n",
              },
            #         prio, max_prio, string max prio, prio
            { 'values': ('priority="100"', '100', 'max_priority="150"', 'priority="101"'),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            #         prio, max_prio, string max prio, prio
            { 'values': ('priority="100"', '150', '', 'priority="50"'),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            #         prio, max_prio, string max prio, prio
            { 'values': ('priority="100"', '150', 'max_priority="40"', 'priority="50"'),
              'exit_value': 1,
              'stdout': "",
              'stderr': "Error: Tried to create thread with priority 50, but space (/ktest/thread_space) max priority is 40.\n",
              },
            #         prio, max_prio, string max prio, prio
            { 'values': ('priority="1000"', '150', 'max_priority="100"', 'priority="50"'),
              'exit_value': 1,
              'stdout': "",
              'stderr': "Error: Tried to create thread with priority 1000, but space (/ktest) max priority is 150.\n",
              },
            #         prio, max_prio, string max prio, prio
            { 'values': ('priority="1000"', '10', '', 'priority="10"'),
              'exit_value': 1,
              'stdout': "",
              'stderr': "Error: Tried to create thread with priority 1000, but space (/ktest) max priority is 10.\n",
              },
            #         prio, max_prio, string max prio, prio
            { 'values': ('priority="100"', '255', 'max_priority="300"', 'priority="150"'),
              'exit_value': 1,
              'stdout': "",
              'stderr': "Error: Maximum priority of the Space /ktest/thread_space (300) exceeds the kernel max priority (255)\n",
              },
            #         prio, max_prio, string max prio, prio
            { 'values': ('priority="100"', '255', 'max_priority="250"', 'priority="150"'),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            #         prio, max_prio, string max prio, prio
            { 'values': ('priority="255"', '1000', 'max_priority="250"', 'priority="150"'),
              'exit_value': 1,
              'stdout': "",
              'stderr': "Error: Maximum priority of the Cell ktest (1000) exceeds the kernel max priority (255)\n",
              },
            #         prio, max_prio, string max prio, prio
            { 'values': ('priority="100"', '255', 'max_priority="250"', 'priority="1500"'),
              'exit_value': 1,
              'stdout': "",
              'stderr': "Error: Tried to create thread with priority 1500, but space (/ktest/thread_space) max priority is 250.\n",
              },
            #         prio, max_prio, string max prio, prio
            { 'values': ('priority="100"', '200', 'max_priority="250"', 'priority="210"'),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            #         prio, max_prio, string max prio, prio
            { 'values': ('', '255', 'max_priority="110"', ''),
              'exit_value': 0,
              'stdout': "",
              'stderr': "",
              },
            #         prio, max_prio, string max prio, prio
            { 'values': ('', '200', 'max_priority="250"', 'priority="210"'),
              'exit_value': 1,
              'stdout': "",
              'stderr': "Error: Tried to create thread with priority 255, but space (/ktest) max priority is 200.\n",
              },
            #         prio, max_prio, string max prio, prio
            { 'values': ('priority="200"', '200', 'max_priority="10"', ''),
              'exit_value': 1,
              'stdout': "",
              'stderr': "Error: Tried to create thread with priority 100, but space (/ktest/thread_space) max priority is 10.\n",
              },

            )

        for value in inputs:
            exit_value, ret_stdout, ret_stderr = \
                        self._run_command(["-S", template % value['values'], "-o", "test_output"])
            self.assertEquals(ret_stderr, value['stderr'])
            self.assertEquals(ret_stdout, value['stdout'])        
            self.assertEquals(exit_value, value['exit_value'])

    def test_roinit_existance(self):
        """
        Test kernel before weaving
        It should contain an roinit but it will be empty
        """
        elf = PreparedElfFile(filename="data/eg_weave/l4kernel")
        self.assertNotEquals(find_kernel_config(elf), None)

    def test_initscript_location(self):
        """
        Test the kernel in a build image.
        Ensure initscript pointer is filled in
        """
        out = StringIO()
        elf = PreparedElfFile(filename="data/eg_weave/image_ia32.elf")
        kcp = find_kernel_config(elf)
        kcp.output(out)
        self.assertEquals(out.getvalue(), 'Kernel Configuration:\n   Init Script: 0x155000\n')

    def test_find_kcp_valid(self):
        """An arbitrary object file shouldn't trip up weaver"""
        elf = PreparedElfFile(filename="data/arm_object")
        self.assertEquals(find_kernel_config(elf), None)

    def test_initscript_output(self):
        """
        Test to print the initscript in human-readable format.
        """
        expected_output = """\
Initialisation Script (Little endian, 4 bit) Operations:
0 (0x0008): CREATE HEAP (base: 0xa0100000, size: 0x200000)
1 (0x0010): INIT IDS (spaces: 256, clists: 259, mutexes: 258)
2 (0x001c): CREATE THREAD HANDLE ARRAY (phys: 0xa0500000, size: 0x400)
3 (0x0024): END OF PHASE
4 (0x0024): CREATE CLIST (id: 0, max_caps: 1024)
5 (0x002c): CREATE SPACE (id: 0, space range: 0-4 (5), clist range: 0-0 (1), mutex range: 0--1 (0), max physical segments: 7, utcb area base: 0x80111000, utcb area size (log 2): 12, has kresources: 1, max priority: 255)
6 (0x0048): CREATE THREAD (cap slot: 0, priority: 255, ip: 0x80100000, sp: 0x80111000, utcb base address: 0x80111000, mr1: 0x8010f000)
7 (0x0060): CREATE SEGMENT (seg_num: 0x0, segment phys_addr: 0xa0501000, attr: 0xff, size: 0xff000, rwx: 0x7)
8 (0x0070): CREATE SEGMENT (seg_num: 0x1, segment phys_addr: 0xa0034000, attr: 0xff, size: 0x7000, rwx: 0x5)
9 (0x0080): MAP MEMORY (segment offset: 0x0, segment number: 0x1, rwx: 0x5, page size (log2): 12, num pages: 0x7, attr: 0x3, virtual base: 0x80100000)
10 (0x0090): CREATE SEGMENT (seg_num: 0x2, segment phys_addr: 0xa0029000, attr: 0xff, size: 0x1000, rwx: 0x6)
11 (0x00a0): MAP MEMORY (segment offset: 0x0, segment number: 0x2, rwx: 0x6, page size (log2): 12, num pages: 0x1, attr: 0x3, virtual base: 0x8010e000)
12 (0x00b0): CREATE SEGMENT (seg_num: 0x3, segment phys_addr: 0xa003c000, attr: 0xff, size: 0x1000, rwx: 0x6)
13 (0x00c0): MAP MEMORY (segment offset: 0x0, segment number: 0x3, rwx: 0x6, page size (log2): 12, num pages: 0x1, attr: 0x3, virtual base: 0x80110000)
14 (0x00d0): CREATE SEGMENT (seg_num: 0x4, segment phys_addr: 0xa0700000, attr: 0xff, size: 0x100000, rwx: 0x6)
15 (0x00e0): MAP MEMORY (segment offset: 0x0, segment number: 0x4, rwx: 0x6, page size (log2): 12, num pages: 0x100, attr: 0x3, virtual base: 0x80300000)
16 (0x00f0): CREATE SEGMENT (seg_num: 0x5, segment phys_addr: 0xa00bc000, attr: 0xff, size: 0x4000, rwx: 0x6)
17 (0x0100): MAP MEMORY (segment offset: 0x0, segment number: 0x5, rwx: 0x6, page size (log2): 12, num pages: 0x4, attr: 0x3, virtual base: 0x80084000)
18 (0x0110): CREATE SEGMENT (seg_num: 0x6, segment phys_addr: 0xa003b000, attr: 0xff, size: 0x1000, rwx: 0x6)
19 (0x0120): MAP MEMORY (segment offset: 0x0, segment number: 0x6, rwx: 0x6, page size (log2): 12, num pages: 0x1, attr: 0x3, virtual base: 0x8010f000)
20 (0x0130): CREATE HEAP (base: 0xa0300000, size: 0x200000)
21 (0x0138): CREATE CLIST (id: 1, max_caps: 1024)
22 (0x0140): CREATE SPACE (id: 5, space range: 5-255 (251), clist range: 1-256 (256), mutex range: 0-255 (256), max physical segments: 15, utcb area base: 0x8007a000, utcb area size (log 2): 12, has kresources: 1, max priority: 255)
23 (0x015c): ALLOW PLATFORM CONTROL
24 (0x0160): ASSIGN IRQ (irq = 825)
25 (0x0168): ASSIGN IRQ (irq = 22)
26 (0x0170): CREATE THREAD (cap slot: 0, priority: 100, ip: 0x80000000, sp: 0x80073000, utcb base address: 0x8007a000, mr1: 0x80071000)
27 (0x0188): CREATE THREAD (cap slot: 1, priority: 100, ip: 0x80000010, sp: 0x80074000, utcb base address: 0x8007a000, mr1: 0x80071000)
28 (0x01a0): CREATE MUTEX (id = 0)
29 (0x01a8): CREATE SEGMENT (seg_num: 0x0, segment phys_addr: 0xa0800000, attr: 0xff, size: 0x3000000, rwx: 0x7)
30 (0x01b8): CREATE SEGMENT (seg_num: 0x1, segment phys_addr: 0xa0040000, attr: 0xff, size: 0x46000, rwx: 0x5)
31 (0x01c8): MAP MEMORY (segment offset: 0x0, segment number: 0x1, rwx: 0x5, page size (log2): 12, num pages: 0x46, attr: 0x3, virtual base: 0x80000000)
32 (0x01d8): CREATE SEGMENT (seg_num: 0x2, segment phys_addr: 0xa008e000, attr: 0xff, size: 0x23000, rwx: 0x6)
33 (0x01e8): MAP MEMORY (segment offset: 0x0, segment number: 0x2, rwx: 0x6, page size (log2): 12, num pages: 0x23, attr: 0x3, virtual base: 0x8004e000)
34 (0x01f8): CREATE SEGMENT (seg_num: 0x3, segment phys_addr: 0xa00b2000, attr: 0xff, size: 0x1000, rwx: 0x6)
35 (0x0208): MAP MEMORY (segment offset: 0x0, segment number: 0x3, rwx: 0x6, page size (log2): 12, num pages: 0x1, attr: 0x3, virtual base: 0x80072000)
36 (0x0218): CREATE SEGMENT (seg_num: 0x4, segment phys_addr: 0xa00b3000, attr: 0xff, size: 0x1000, rwx: 0x6)
37 (0x0228): MAP MEMORY (segment offset: 0x0, segment number: 0x4, rwx: 0x6, page size (log2): 12, num pages: 0x1, attr: 0x3, virtual base: 0x80073000)
38 (0x0238): CREATE SEGMENT (seg_num: 0x5, segment phys_addr: 0x40103000, attr: 0xff, size: 0x1000, rwx: 0x6)
39 (0x0248): MAP MEMORY (segment offset: 0x0, segment number: 0x5, rwx: 0x6, page size (log2): 12, num pages: 0x1, attr: 0x3, virtual base: 0x80076000)
40 (0x0258): CREATE SEGMENT (seg_num: 0x6, segment phys_addr: 0x40100000, attr: 0xff, size: 0x1000, rwx: 0x6)
41 (0x0268): MAP MEMORY (segment offset: 0x0, segment number: 0x6, rwx: 0x6, page size (log2): 12, num pages: 0x1, attr: 0x0, virtual base: 0x80077000)
42 (0x0278): CREATE SEGMENT (seg_num: 0x7, segment phys_addr: 0x40101000, attr: 0xff, size: 0x1000, rwx: 0x6)
43 (0x0288): MAP MEMORY (segment offset: 0x0, segment number: 0x7, rwx: 0x6, page size (log2): 12, num pages: 0x1, attr: 0x3, virtual base: 0x80078000)
44 (0x0298): CREATE SEGMENT (seg_num: 0x8, segment phys_addr: 0x40102000, attr: 0xff, size: 0x1000, rwx: 0x6)
45 (0x02a8): MAP MEMORY (segment offset: 0x0, segment number: 0x8, rwx: 0x6, page size (log2): 12, num pages: 0x1, attr: 0x3, virtual base: 0x80079000)
46 (0x02b8): CREATE SEGMENT (seg_num: 0x9, segment phys_addr: 0x40100000, attr: 0xff, size: 0x1000, rwx: 0x6)
47 (0x02c8): MAP MEMORY (segment offset: 0x0, segment number: 0x9, rwx: 0x6, page size (log2): 12, num pages: 0x1, attr: 0x0, virtual base: 0x80075000)
48 (0x02d8): CREATE SEGMENT (seg_num: 0xa, segment phys_addr: 0xa00b8000, attr: 0xff, size: 0x4000, rwx: 0x6)
49 (0x02e8): CREATE SEGMENT (seg_num: 0xb, segment phys_addr: 0xa00bc000, attr: 0xff, size: 0x4000, rwx: 0x6)
50 (0x02f8): MAP MEMORY (segment offset: 0x0, segment number: 0xb, rwx: 0x6, page size (log2): 12, num pages: 0x4, attr: 0x3, virtual base: 0x80084000)
51 (0x0308): CREATE SEGMENT (seg_num: 0xc, segment phys_addr: 0xa0600000, attr: 0xff, size: 0x100000, rwx: 0x6)
52 (0x0318): MAP MEMORY (segment offset: 0x0, segment number: 0xc, rwx: 0x6, page size (log2): 12, num pages: 0x100, attr: 0x3, virtual base: 0x80200000)
53 (0x0328): CREATE SEGMENT (seg_num: 0xd, segment phys_addr: 0xa00c0000, attr: 0xff, size: 0x4000, rwx: 0x6)
54 (0x0338): MAP MEMORY (segment offset: 0x0, segment number: 0xd, rwx: 0x6, page size (log2): 12, num pages: 0x4, attr: 0x3, virtual base: 0x80088000)
55 (0x0348): CREATE SEGMENT (seg_num: 0xe, segment phys_addr: 0xa00b1000, attr: 0xff, size: 0x1000, rwx: 0x6)
56 (0x0358): MAP MEMORY (segment offset: 0x0, segment number: 0xe, rwx: 0x6, page size (log2): 12, num pages: 0x1, attr: 0x3, virtual base: 0x80071000)
57 (0x0368): CREATE SPACE (id: 256, space range: 256-256 (1), clist range: 257-257 (1), mutex range: 256-257 (2), max physical segments: 1, utcb area base: 0x8007b000, utcb area size (log 2): 12, has kresources: 0, max priority: 255)
58 (0x0384): CREATE MUTEX (id = 256)
59 (0x038c): CREATE MUTEX (id = 257)
60 (0x0394): CREATE SEGMENT (seg_num: 0x0, segment phys_addr: 0xa00c0000, attr: 0xff, size: 0x4000, rwx: 0x6)
61 (0x03a4): MAP MEMORY (segment offset: 0x0, segment number: 0x0, rwx: 0x6, page size (log2): 12, num pages: 0x4, attr: 0x3, virtual base: 0x80088000)
62 (0x03b4): CREATE SPACE (id: 257, space range: 257-257 (1), clist range: 258-258 (1), mutex range: 258-257 (0), max physical segments: 1, utcb area base: 0x8007c000, utcb area size (log 2): 12, has kresources: 0, max priority: 255)
63 (0x03d0): CREATE THREAD (cap slot: 4, priority: 100, ip: 0x80000010, sp: 0x80075000, utcb base address: 0x8007c000, mr1: 0x80071000)
64 (0x03e8): CREATE SEGMENT (seg_num: 0x0, segment phys_addr: 0xa00b4000, attr: 0xff, size: 0x1000, rwx: 0x6)
65 (0x03f8): MAP MEMORY (segment offset: 0x0, segment number: 0x0, rwx: 0x6, page size (log2): 12, num pages: 0x1, attr: 0x3, virtual base: 0x80074000)
66 (0x0408): CREATE MUTEX CAP (clist ref: 0x24, cap slot: 1, mutex reference: 0x384)
67 (0x0414): CREATE IPC CAP (clist ref: 0x138, cap slot: 2, thread reference: 0x0048)
68 (0x0420): CREATE MUTEX CAP (clist ref: 0x138, cap slot: 3, mutex reference: 0x384)
69 (0x042c): END OF PHASE
Total size: 1068 bytes
"""
        out = StringIO()
        elf = PreparedElfFile(filename="data/eg_weave/image_okl4_cell.elf")
        initscript = find_init_script(elf)
        self.assertNotEquals(initscript, None)
        initscript.output(out)
        self.assertEquals(out.getvalue(), expected_output)

    def test_max_system_threads(self):
        """
        Test setting the maximum number of threads allowed on a system.
        """

        template = """
<image>
	<machine>
		<cpu name="xscale" />
		<word_size size="0x20" />
		<virtual_memory name="virtual">
			<region base="0x1000" size="0xcffff000" />
		</virtual_memory>
		<physical_memory name="physical">
			<region base="0xa0000000" size="0x3800000" />
		</physical_memory>
		<page_size size="0x1000" />
	</machine>
	<virtual_pool name="virtual">
		<memory src="virtual"/>
	</virtual_pool>

	<physical_pool name="physical">
		<memory src="physical" />
	</physical_pool>

	<kernel file="data/eg_weave/l4kernel" virtpool="virtual" physpool="physical">
		<config>
                	%s
		</config>
	</kernel>

	<okl4 name="test" kernel_heap="4K" file="data/eg_weave/second_cell" %s >
               %s 
        </okl4>
</image>
        """

        inputs = (
            # Default thread values.
            { 'values':
              ('',
               'threads="256"',
               ''),
              'exit_value': 0,
              'stdout': "",
              'stderr': '',
              },
            # Set max threads above the kernel limit.
            { 'values':
              ('<option key="threads" value="10000" />',
               'threads="256"',
               ''),
              'exit_value': 1,
              'stdout': "",
              'stderr': 'Error: Maximum number of threads (4092) exceeded: 10000\n',
              },
            # 
            { 'values':
              ('<option key="threads" value="256" />',
               'threads="1000"',
               ''),
              'exit_value': 1,
              'stdout': "",
              'stderr': 'Error: Total number of threads (1000) exceeds kernel maximum of 256.\n',
              },
            # Default thread values.
            { 'values':
              ('',
               'threads="1"',
               '<thread name="foo" start="main" />'),
              'exit_value': 1,
              'stdout': "",
              'stderr': 'Error: Number of threads created in space "/test" (2) exceed the maximum allowed (1).\n',
              },
            # Default thread values.
            { 'values':
              ('',
               'threads="256"',
               '<thread name="foo" start="main" /> <space name="1"> <thread name="bar" start="main" /> </space>'),
              'exit_value': 0,
              'stdout': "",
              'stderr': '',
              },
            )

        for value in inputs:
            exit_value, ret_stdout, ret_stderr = \
                        self._run_command(["-S", template % value['values'], "-o", "test_output"])
            self.assertEquals(ret_stdout, value['stdout'])        
            self.assertEquals(ret_stderr, value['stderr'])
            self.assertEquals(exit_value, value['exit_value'])

    def test_bootinfo_output(self):
        """
        Test to print the bootinfo in human-readable format.

        This is a duplicate of what's in test display and it only here
        to improve the test coverage of the bootinfo code.
        """
        out = StringIO()
        elf = PreparedElfFile(filename="data/bootinfo_elf")
        bootinfo = find_bootinfo(elf)
        self.assertNotEquals(bootinfo, None)
	bootinfo.output(out)

