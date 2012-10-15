#!/bin/bash
#
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
#
# Unit test script to exercise the soc-sdk code

target=simulate_test
if [ "$1" = "--build-only" ]; then
    target=
    shift
fi

tools/build.py build_dir=build.soc_sdk project=soc-sdk "$@"
if [ $? -ne 0 ]; then
    echo "Building soc-sdk failed"
    exit 1
fi

mkdir build.soc_sdk_tar
pushd build.soc_sdk_tar
tar xvf ../build.soc_sdk/SDK/object/soc-sdk/*.tar.gz

pushd soc/kernel/*/soc/*
eval `grep '^PLATFORM=' Makefile`

KERNELS=`ls --ignore=soc ../../`
for KERNEL in $KERNELS; do
    make clean TYPE=$KERNEL
    make install TYPE=$KERNEL
    if [ $? -ne 0 ]; then
        echo "$KERNEL make failed"
        exit 1
    fi
done

popd
popd

tools/build.py build_dir=build.soc_sdk_test project=ctest "$@" XIP=True

if [ $? -ne 0 ]; then
    echo "Building test base code failed"
    exit 1
fi

for KERNEL in $KERNELS; do
    rm -f build.soc_sdk_test/images/image.*
    sed -i -e "s/<kernel.*>/<kernel sdk=\"build.soc_sdk_tar\/soc\/\" configuration=\"$KERNEL\" platform=\"$PLATFORM\" physpool=\"physical\" virtpool=\"virtual\" >/" build.soc_sdk_test/images/weaver.xml
    echo ===== $KERNEL =====
    tools/build.py build_dir=build.soc_sdk_test project=ctest "$@" XIP=True TESTING.TIMEOUT=100 $target

   if [ $? -ne 0 ]; then
       echo "$KERNEL test failed"
       exit 1
   fi
done

exit 0

