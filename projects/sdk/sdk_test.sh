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
# Unit test script to exercise the sdk code

target=simulate_test
if [ "$1" = "--build-only" ]; then
    target=
    shift
fi

# Build the SDK.
tools/build.py build_dir=build.sdk project=sdk "$@"

# Test that build was successful.
if [ $? -ne 0 ]; then
    echo "Building sdk failed"
    exit 1
fi

# Untar SDK to test directory.
mkdir build.sdk_tar
pushd build.sdk_tar
tar xvf ../build.sdk/SDK/object/sdk/*.tar.gz

# Run licence post-check.
pushd sdk
licence_tool --type=sdk --mode=post-check . > licence_check_output
if [ `sed -n "1 p" licence_check_output | cut -f1`  ]; then
    echo "Licence tool post-check detected errors in the following files:"
    cat licence_check_output
    rm licence_check_output
    exit 1
fi
rm licence_check_output
popd

EXAMPLES=$(ls --ignore=sample --ignore=linux sdk/okl4/*/examples/)

popd

# For every example we create a testing ground using the build system
# - this is so that we can do simulate_test. FIXME: this is a waste of
# time -- there should be a quicker way to run simulate_test.
for EXAMPLE in $EXAMPLES; do
    if [ $EXAMPLE = "ctest" ]; then
        tools/build.py build_dir=build.$EXAMPLE project=$EXAMPLE XIP=True "$@"
    elif [ $EXAMPLE = "singlecell" ]; then
        tools/build.py build_dir=build.$EXAMPLE project=examples XIP=True example=hello "$@"
    elif [ $EXAMPLE = "multicell" ]; then
        tools/build.py build_dir=build.$EXAMPLE project=examples XIP=True example=chatterbox,echo "$@"
    else
        tools/build.py build_dir=build.$EXAMPLE project=examples XIP=True example=$EXAMPLE "$@"
    fi

    if [ $? -ne 0 ]; then
        echo "Building test base code failed"
        exit 1
    fi
done

# For every example build every configuration and then copy over the
# image.sim and run in the appropriate testing ground from above.
OKL4_CONFIGS=$(ls --ignore=examples build.sdk_tar/sdk/okl4/*/)

for EXAMPLE in $EXAMPLES; do
    for OKL4_CONFIG in $OKL4_CONFIGS; do

        pushd build.sdk_tar/sdk/okl4/*/examples/$EXAMPLE

        KERNEL=$(echo $OKL4_CONFIG | cut -d '-' -f 1)
        BUILD=$(echo $OKL4_CONFIG | cut -d '-' -f 2)

        # We don't test decrypt for now
        if [ $EXAMPLE = "decrypt" ]; then
            popd
            continue
        fi

        if [ $EXAMPLE = "multicell" ]; then
            if [ $KERNEL = "nano" ]; then
                popd
                continue
            fi
        fi

        make KERNEL=$KERNEL BUILD=$BUILD
        if [ $? -ne 0 ]; then
            echo "$OKL4_CONFIG make failed"
            exit 1
        fi

        popd

        echo cp build.sdk_tar/sdk/okl4/*/examples/$EXAMPLE/build.$OKL4_CONFIG/images/* build.$EXAMPLE/images/
        cp build.sdk_tar/sdk/okl4/*/examples/$EXAMPLE/build.$OKL4_CONFIG/images/* build.$EXAMPLE/images/

        if [ $KERNEL = "nano" ]; then
            doxip=False
        else
            doxip=True
        fi

        echo "===== $EXAMPLE : $OKL4_CONFIG ====="
        if [ $EXAMPLE = "ctest" ]; then
            echo tools/build.py build_dir=build.$EXAMPLE project=$EXAMPLE kernel=$KERNEL XIP=$doxip "$@" TESTING.TIMEOUT=300 $target
            tools/build.py build_dir=build.$EXAMPLE project=$EXAMPLE kernel=$KERNEL XIP=$doxip "$@" TESTING.TIMEOUT=300 $target
        elif [ $EXAMPLE = "multicell" ]; then
            echo tools/build.py build_dir=build.$EXAMPLE project=examples kernel=$KERNEL XIP=$doxip example=chatterbox,echo "$@" TESTING.TIMEOUT=300 $target
            tools/build.py build_dir=build.$EXAMPLE project=examples kernel=$KERNEL XIP=$doxip example=chatterbox,echo "$@"  TESTING.TIMEOUT=300 $target
        elif [ $EXAMPLE = "singlecell" ]; then
            echo tools/build.py build_dir=build.$EXAMPLE project=examples kernel=$KERNEL XIP=$doxip example=hello "$@" TESTING.TIMEOUT=300 $target
            tools/build.py build_dir=build.$EXAMPLE project=examples kernel=$KERNEL XIP=$doxip example=hello "$@" TESTING.TIMEOUT=300 $target
        else
            echo tools/build.py build_dir=build.$EXAMPLE project=examples kernel=$KERNEL XIP=$doxip example=$EXAMPLE "$@" TESTING.TIMEOUT=300 $target
            tools/build.py build_dir=build.$EXAMPLE project=examples kernel=$KERNEL XIP=$doxip example=$EXAMPLE "$@"  TESTING.TIMEOUT=300 $target
        fi
        RET=$?

        # If we are on gta01 then don't check output, because it does not
        # appear to like XIP.
        if echo $@ | grep 'gta01' > /dev/null; then
            continue
        fi

        if [ $RET -ne 0 ]; then
            echo -n "$EXAMPLE:$OKL4_CONFIG test failed."
            exit 1
        fi

    done
done

# Now test with the soc source. Build and install soc example for all
# kernel configurations.

pushd build.sdk_tar/sdk/kernel/*/soc/*/

KERNEL_CONFIGS=$(ls --ignore=soc ../../)
for KERNEL_CONFIG in $KERNEL_CONFIGS; do
    KERNEL=$(echo $KERNEL_CONFIG | cut -d '-' -f 1)
    BUILD=$(echo $KERNEL_CONFIG | cut -d '-' -f 2)

    make install KERNEL=$KERNEL BUILD=$BUILD
    if [ $? -ne 0 ]; then
        echo "$KERNEL_CONFIG make failed"
        exit 1
    fi
done

popd

# Do as above again this time using the build soc.
for EXAMPLE in $EXAMPLES; do
    for OKL4_CONFIG in $OKL4_CONFIGS; do

        pushd build.sdk_tar/sdk/okl4/*/examples/$EXAMPLE

        KERNEL=$(echo $OKL4_CONFIG | cut -d '-' -f 1)
        BUILD=$(echo $OKL4_CONFIG | cut -d '-' -f 2)

        # We don't test decrypt for now
        if [ $EXAMPLE = "decrypt" ]; then
            popd
            continue
        fi

        if [ $EXAMPLE = "multicell" ]; then
            if [ $KERNEL = "nano" ]; then
                popd
                continue
            fi
        fi

        make KERNEL=$KERNEL BUILD=$BUILD
        if [ $? -ne 0 ]; then
            echo "$OKL4_CONFIG make failed"
            exit 1
        fi

        popd

        echo cp build.sdk_tar/sdk/okl4/*/examples/$EXAMPLE/build.$OKL4_CONFIG/images/* build.$EXAMPLE/images/
        cp build.sdk_tar/sdk/okl4/*/examples/$EXAMPLE/build.$OKL4_CONFIG/images/* build.$EXAMPLE/images/

        if [ $KERNEL = "nano" ]; then
            doxip=False
        else
            doxip=True
        fi

        echo "===== $EXAMPLE : $OKL4_CONFIG ====="
        if [ $EXAMPLE = "ctest" ]; then
            echo tools/build.py build_dir=build.$EXAMPLE project=$EXAMPLE kernel=$KERNEL XIP=$doxip "$@" TESTING.TIMEOUT=300 $target
            tools/build.py build_dir=build.$EXAMPLE project=$EXAMPLE kernel=$KERNEL XIP=$doxip "$@" TESTING.TIMEOUT=300 $target
        elif [ $EXAMPLE = "multicell" ]; then
            echo tools/build.py build_dir=build.$EXAMPLE project=examples kernel=$KERNEL XIP=$doxip example=chatterbox,echo "$@" TESTING.TIMEOUT=300 $target
            tools/build.py build_dir=build.$EXAMPLE project=examples kernel=$KERNEL XIP=$doxip example=chatterbox,echo "$@"  TESTING.TIMEOUT=300 $target
        elif [ $EXAMPLE = "singlecell" ]; then
            echo tools/build.py build_dir=build.$EXAMPLE project=examples kernel=$KERNEL XIP=$doxip example=hello "$@" TESTING.TIMEOUT=300 $target
            tools/build.py build_dir=build.$EXAMPLE project=examples kernel=$KERNEL XIP=$doxip example=hello "$@" TESTING.TIMEOUT=300 $target
        else
            echo tools/build.py build_dir=build.$EXAMPLE project=examples kernel=$KERNEL XIP=$doxip example=$EXAMPLE "$@" TESTING.TIMEOUT=300 $target
            tools/build.py build_dir=build.$EXAMPLE project=examples kernel=$KERNEL XIP=$doxip example=$EXAMPLE "$@"  TESTING.TIMEOUT=300 $target
        fi
        RET=$?

        if echo $@ | grep 'gta01' > /dev/null; then
            continue
        fi

        if [ $RET -ne 0 ]; then
            echo -n "$EXAMPLE:$OKL4_CONFIG test failed."
            exit 1
        fi

    done
done

exit 0

