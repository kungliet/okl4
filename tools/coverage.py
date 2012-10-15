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
# Author: Alex Webster and Guillaume Mulko

#!/usr/bin/env python
"""
The coverage script reads addresses from a trace file and use 
them to calculate the symbol coverage and file coverage of binaries
found in the directories specified to the script.

coverage --help to see usage

"""
import os
import sys
import time
from optparse import OptionParser
import re
import commands

output_filename = "_test_coverage.txt"
lookup_addr = {}
lookup_file = {}
lookup_pkg = {}

class Address:
    def __init__(self, address, symbol, filename, package):
        self.address = address
        self.symbol = None
        if filename in lookup_file:
            if lookup_file[filename].get_package().get_name() != package:
                raise "File %s already exist in a different package: %s <--> %s (symbol %s)" % (filename, lookup_file[filename].get_package().get_name(), package, symbol)
            for sym in lookup_file[filename].get_symbol_list():
                if sym.get_name() == symbol:
                    self.symbol = sym
                    self.symbol.add_address(self)
                    break
        if self.symbol == None:
            self.symbol = Symbol(self, symbol, filename, package)
        self.hit = False
        lookup_addr[address] = self

    def get_symbol(self):
        return self.symbol

    def set_hit(self):
        self.hit = True


class Symbol:
    def __init__(self, address, symbol, filename, package):
        self.name = symbol
        self.addr_list = [address]
        if filename in lookup_file:
            if lookup_file[filename].get_package().get_name() != package:
                raise "File %s already exist in a different package: %s <--> %s" % (filename, lookup_file[filename].get_package().get_name(), package)
            self.file = lookup_file[filename]
            self.file.add_symbol(self)
        else:
            self.file = File(self, filename, package)
        self.coverage = float(-1)

    def get_name(self):
        return self.name

    def get_file(self):
        return self.file

    def get_coverage(self):
        if self.coverage == -1:
            self.calculate_coverage()
        return self.coverage

    def calculate_coverage(self):
        hit_addr = [addr for addr in self.addr_list if addr.hit]
        self.coverage = len(hit_addr)
        if len(self.addr_list) != 0:
            self.coverage = (self.coverage * 100.0) / len(self.addr_list)
        return self.coverage

    def add_address(self, address):
        if not address in self.addr_list:
            self.addr_list.append(address)


class File:
    def __init__(self, symbol, filename, package):
        self.name = filename
        self.sym_list = [symbol]
        if package in lookup_pkg:
            self.package = lookup_pkg[package]
            self.package.add_file(self)
        else:
            self.package = Package(self, package)
        self.coverage = float(-1)
        lookup_file[filename] = self

    def get_name(self):
        return self.name

    def get_package(self):
        return self.package

    def get_symbol_list(self):
        return self.sym_list

    def get_coverage(self):
        if self.coverage == -1:
            self.calculate_coverage()
        return self.coverage

    def calculate_coverage(self):
        self.coverage = 0
        for sym in self.sym_list:
            self.coverage += sym.get_coverage()
        if len(self.sym_list) != 0:
            self.coverage /= len(self.sym_list)
        return self.coverage

    def add_symbol(self, symbol):
        if not symbol in self.sym_list:
            self.sym_list.append(symbol)


class Package:
    def __init__(self, file, package):
        self.name = package
        self.file_list = [file]
        self.coverage = float(-1)
        lookup_pkg[package] = self

    def get_name(self):
        return self.name

    def get_file_list(self):
        return self.file_list

    def get_coverage(self):
        if self.coverage == -1:
            self.calculate_coverage()
        return self.coverage

    def calculate_coverage(self):
        self.coverage = 0
        for file in self.file_list:
            self.coverage += file.get_coverage()
        if len(self.file_list) != 0:
            self.coverage /= len(self.file_list)
        return self.coverage

    def add_file(self, file):
        if not file in self.file_list:
            self.file_list.append(file)


class Coverage:
    def __init__(self, build_dir, packages, output, verbose):
        self.packages = packages
        self.output = output
        self.verbose = verbose
        self.build_dir = build_dir
        self.ctest = 0
        self.ntest = 0
        self.ktest = 0
        self.testlib = 0
        if "ktest" in packages or "pistachio" in packages:
            self.ktest = 1
        elif "ctest" in packages:
            self.ctest = 1
        elif "nanokernel" in packages:
            self.ntest = 1
        else:
            self.testlib = 1

        self.binaries = []
        self.search_binaries(build_dir)
        self.scan_symbols()

    def search_binaries(self, build_dir):
        for d in os.walk(build_dir).next()[1]:
            if self.ktest and d.find("pistachio") == -1 and d.find("ktest") == -1:
                continue
            if self.testlib and d.find("iguana") == -1:
                continue
            if self.ctest and d.find("ctest") == -1:
                continue
            if self.ntest and d.find("nanokernel") == -1:
                continue
            dirname = os.path.join(build_dir, d, "bin")
            for f in os.listdir(dirname):
                pathname = os.path.join(dirname, f)
                if "executable" in os.popen2("file %s" % pathname)[1].read():
                    if self.testlib:
                        if f == "test" or f == "ig_server":
                            self.binaries.append(pathname)
                    else:
                        self.binaries.append(pathname)
        print self.binaries

    def lookup_package_name(self, symbol, filename):
        if filename == "":
            if self.testlib:
                grep_fd = os.popen("grep -r \"<%s>\" %s/iguana/lib" % (symbol, self.build_dir))
            elif self.ctest:
                grep_fd = os.popen("grep -r \"<%s>\" %s/ctest/lib" % (symbol, self.build_dir))
            else:
                raise "Should not get here"
            grep_res = grep_fd.readlines()
            grep_fd.close()
            if len(grep_res) > 1:
                raise "Cannot find package for symbol %s: too many matches" % symbol
            if len(grep_res) == 0:
                return "unknown"
            fields = grep_res[0].split()
            (dummy, sep, libname) = fields[2].rpartition("/")
            libname = libname.split(".")[0]
            pkg = libname.replace("lib", "libs/")
            if pkg in self.packages:
                return pkg
            else:
                return "unknown"
        else:
            for pkg in self.packages:
                if filename.find(pkg + '/') != -1:
                    return pkg
            return "unknown"

    def add_address_range(self, start_addr, end_addr, range2skip, sym_name, filename, package):
        addr2skip = []
        if len(range2skip) > 1:
            if len(range2skip)%2 != 0:
                if range2skip[len(range2skip)-1] == end_addr:
                    range2skip.pop(len(range2skip)-1)
                else:
                    raise "Literal pool ranges are not complete for symbol %s" % sym_name
            for i in range(0, len(range2skip), 2):
                addr2skip += range(range2skip[i], range2skip[i+1], 4)
        if start_addr in lookup_addr:
            raise "Symbol scan Error: Address %x found twice in %s (%s) and %s (%s)" % (start_addr, package, sym_name, lookup_addr[start_addr].get_symbol().get_file().get_package().get_name(), lookup_addr[start_addr].get_symbol().get_name())
        for addr in range(start_addr, end_addr, 4):
            if not addr in addr2skip:
                lookup_addr[addr] = Address(addr, sym_name, filename, package)


    def scan_symbols(self):
        if self.verbose: print "Scanning symbols..."
        sys.stdout.flush()
        count = 0
        last = 0
        # List of particular symbols we should skip because the assembly
        # function is defined in a C function
        sym2skip = ["async_fp_helper_asm", "illegal1"]
        # List of keywords that match the files to skip
        file2skip = ["kdb", "platform", "platsupport", "debug.c", "assert.c", "wbtest", "resources.c", "lib.c", "trace.c", "panic.c"]

        for bin in self.binaries:
            package = "unknown"
            extra_file2skip = []
            if self.ktest:
                if bin.find("pistachio") != -1:
                    package = "pistachio"
                    extra_file2skip = ["init.c", "string.c"]
                else:
                    package = "ktest"
            elif self.ntest:
                package="nanokernel"
            elif self.testlib and bin.find("iguana_server") != -1:
                package = "iguana/server"
            objdump = commands.getoutput("arm-linux-objdump -DCl --section=.text %s" % bin)
            if self.ntest:
                objdump += commands.getoutput("arm-linux-objdump -DCl --section=.asmtext %s" % bin)
            if self.verbose: print "%s..." % bin,
            previous_addr = 0
            literal_pool = False
            # For each symbol we put in that array the ranges of addresses
            # corresponding to arm literal pools so that we can count them out
            # of the symbol coverage
            range2skip = []
            cnt = 0
            for line in os.popen2("nm -nC %s" % bin)[1]:
                fields = line.strip().split(" ", 2)
                if len(fields) < 3:
                    continue
                value = int(fields[0], 16)
                if previous_addr != 0:
                    if fields[len(fields)-1] == "$d":
                        range2skip.append(value)
                        literal_pool = True
                        continue
                    elif fields[len(fields)-1] == "$a" and literal_pool:
                        range2skip.append(value)
                        literal_pool = False
                        continue
                    else:
                        # If we have reached the next valid symbol then
                        # calculate the size of the previous symbol and add the
                        # addresses to lookup array skipping the literal pools
                        self.add_address_range(previous_addr, value, range2skip, name, filename, package)
                        previous_addr = 0
                        literal_pool = False
                        range2skip = []
                # Skip useless symbols
                if fields[len(fields)-1].startswith("$") or (fields[len(fields)-1] in sym2skip) or \
                  fields[len(fields)-1].lower().find("kdb") != -1 or \
                  re.search('<%s>:\n' % re.escape(fields[len(fields)-1]), objdump, re.M) == None:
                    continue
                if self.verbose:
                    count += 1
                    if time.time() - last > 1:
                        print "%d..." % count,
                        sys.stdout.flush()
                        last = time.time()
                if self.ctest or (self.testlib and package != "iguana/server"):
                    package = "unknown"
                previous_addr = value
                name = fields[2]

                # Search for filename in objdump, either after "<symbol_name>:"
                # or after "symbol_name():"
                m = re.search('<%s(\(\))?>:\n(.+\n)*(?P<filename>.*\.c{1,2}):[0-9]+\n' % re.escape(name), objdump, re.M)
                if m == None:
                    if package == "unknown":
                        package = self.lookup_package_name(name, "")
                        # If we still can't find the package we skip the symbol
                        if package == "unknown":
                            previous_addr = 0
                    else:
                        filename = "unknown_%s" % package
                else:
                    filename = m.group('filename')
                    if package == "unknown":
                        package = self.lookup_package_name(name, filename)
                        # If we still can't find the package we skip the symbol
                        if package == "unknown":
                            previous_addr = 0
                    else:
                        # If the symbol does not seem to belong to this package
                        # or it is a symbol we are not interested in, we also skip the symbol
                        if filename.find(package.replace("ktest", "ktest") + '/') == -1:
                            previous_addr = 0
                        else:
                            for str in file2skip + extra_file2skip:
                                if filename.find(str) != -1:
                                    previous_addr = 0
                                    break
            if previous_addr != 0:
                m = re.search('^(?P<last_addr>[a-f0-9]+):.*\Z' % re.escape(name), objdump, re.M)
                if m == None:
                    raise "Could not find the last address in objdump"
                value = m.group('last_addr')
                self.add_address_range(previous_addr, value, range2skip, name, filename, package)
            if self.verbose: print "\n",
        if self.verbose: print "Finished scanning symbols..."

    def read_tracelog(self, tracelog_file):
        if self.verbose: print "Reading tracelog file..."
        sys.stdout.flush()
        try:
            n = 0
            last = 0
            for line in file(tracelog_file):
                addr = long(line, 16)
                if addr in lookup_addr:
                    # TODO: Need to handle condition flag to make sure the instruction
                    # is actually executed
                    lookup_addr[addr].set_hit()
                if self.verbose:
                    n += 1
                    if time.time() - last > 1:
                        print "%d..." % n,
                        sys.stdout.flush()
                        last = time.time()
        except:
            print "Error while reading tracelog file %s!" % tracelog_file
            raise
        if self.verbose: print "\nFinished reading tracelog file..."
        sys.stdout.flush()

    def print_result(self, output_file, summary):
        if self.verbose: print "Writing results..."
        if not output_file: print "\n<results>",
        for pkg_name, pkg_obj in sorted(lookup_pkg.items()):
            tab1 = ""
            tab2 = "  "
            if not output_file:
                print "\nPackage %s:" % pkg_obj.get_name(),
                tab1 = "  "
                tab2 = "    "
            try:
                print >> self.output[pkg_name], "\n%sAverage coverage = %.1f%%" % (tab1, pkg_obj.calculate_coverage())
            except:
                print self.output.keys()
                raise
            print >> self.output[pkg_name], "\n%sFile coverage:" % tab1
            sym_cov = ""
            for file in pkg_obj.get_file_list():
                if file.get_name().find("unknown") != -1:
                    filename = "unknown"
                else:
                    filename = file.get_name()
                print >> self.output[pkg_name], "%s%s: %.1f%%" % (tab2, filename, file.calculate_coverage())
                for sym in file.get_symbol_list():
                    sym_cov += "\n%s%s: %.1f%%" % (tab2, sym.get_name(), sym.calculate_coverage())
            print >> self.output[pkg_name], "\n%sSymbol coverage:%s" % (tab1, sym_cov)

        if output_file:
            for file in self.output.values():
                file.close()
        if not output_file: print "</results>"
        if summary: 
            print "<summary>"
            for package, pkg_obj in sorted(lookup_pkg.items()):
                print "Package %s: average coverage = %.1f%%" % (package, pkg_obj.calculate_coverage())
            print "</summary>"
        if self.verbose: print "Finished writing results..."



def parse_command_line():
    """Parse the command line arguments and return the options and args."""

    usage = "usage: %prog -p packages_list [options]"
    parser = OptionParser(usage)
    parser.add_option("-p", "--packages", dest="packages_list", default='',
                    help="Packages to assess (separated by comma)")

    parser.add_option("-b", "--build_dir", dest="build_dir", default="build",
                    help="Directory where are located the binaries (default %default)")

    parser.add_option("-t", "--tracelog_file", dest="tracelog_file", default="/tmp/sk1.log",
                    help="Absolute path to the skyeye tracelog file (default %default)")

    parser.add_option("-f", "--file", action="store_true", dest="output_file", default=False,
                    help="Write output to file \"<package_name>%s\" for each package specified with -p option (default False)" % output_filename)

    parser.add_option("-v", "--verbose", action="store_true", dest="verbose", default=False,
                    help="Print progress information (default %default)")

    parser.add_option("-s", "--summary", action="store_true", dest="summary", default=False,
                    help="Print a summary of the results (default %default)")

    (options, args) = parser.parse_args()
    if not options.packages_list:
        parser.error("No package specified")
    packages = options.packages_list.split(",")
    output = {}
    for package in packages:
        pkg_name = package.replace("/", "")
        if options.output_file:
            output[package] = open(os.path.join(options.build_dir, pkg_name + output_filename), "w")
        else:
            output[package] = sys.stdout
    return (options.build_dir, packages, options.tracelog_file, output, options.output_file, options.summary, options.verbose)

def main():
    """main()
    
    """
    (build_dir, packages, tracelog_file, output, output_file, summary, verbose) = parse_command_line()
    coverage = Coverage(build_dir, packages, output, verbose)
    coverage.read_tracelog(tracelog_file)
    coverage.print_result(output_file, summary)

if __name__ == '__main__':
    main()
