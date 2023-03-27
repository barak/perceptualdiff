#!/bin/bash

set -eux

# Script to run pdiff against a set of image file pairs, and check that the
# PASS or FAIL status is as expected.

# Disable test of loading a corrupt png file, which tickled a bug in some
# libraries on some architectures. Re-enable when fixed. See 8e7f360f,
# https://bugs.debian.org/982864, and https://bugs.debian.org/992905.
test_bad_file=false
# Guard a test that mysteriously fails on s390x and sparc64
test_alpha_bug=false

trap "echo -e '\x1b[01;31mFailed\x1b[0m'" ERR

#------------------------------------------------------------------------------
# Image files and expected perceptualdiff PASS/FAIL status.  Line format is
# (PASS|FAIL) image1.(tif|png) image2.(tif|png)
#
# Edit the following lines to add additional tests.
all_tests () {
    echo FAIL Bug1102605{_ref,}.tif
    echo PASS Bug1471457{_ref,}.tif
    echo PASS cam_mb{_ref,}.tif
    echo FAIL fish{2,1}.png
    echo PASS square{,_scaled}.png
    echo FAIL Aqsis_vase{,_ref}.png
    if $(test_alpha_bug); then
	echo FAIL alpha{1,2}.png
    fi
}

echo "*** tmpdir: ${tmpdir:=/tmp}"

# Change to test directory
echo "*** script_directory: ${script_directory:=$(dirname "$0")}"
cd "$script_directory"

if [ -z "${pdiff:=}" ]; then
found=0
for d in ../build .. ../obj*; do
    pdiff="$d/perceptualdiff"
    if [ -f "$pdiff" ]; then
	found=1
	break
    fi
done

if [ $found = 0 ]; then
    echo 'perceptualdiff must be built and exist in the repository root or the "build" directory'
    exit 1
fi
fi
echo "*** testing executable binary $pdiff"

#------------------------------------------------------------------------------

total_tests=0
num_tests_failed=0

# Run all tests.
while read expected_result image1 image2 ; do
	if "$pdiff" --verbose --scale "$image1" "$image2" 2>&1 | grep -q "^$expected_result" ; then
		total_tests=$((total_tests+1))
	else
		num_tests_failed=$((num_tests_failed+1))
		echo "Regression failure: expected $expected_result for \"$pdiff $image1 $image2\"" >&2
	fi
done <<EOF
$(all_tests)
EOF
# (the above with the EOF's is a stupid bash trick to stop while from running
# in a subshell)

# Give some diagnostics:
if [[ $num_tests_failed == 0 ]] ; then
	echo "*** all $total_tests tests passed"
else
	echo "*** $num_tests_failed failed tests of $total_tests"
	exit $num_tests_failed
fi

# Run additional tests.
"$pdiff" 2>&1 | grep -i 'openmp'
"$pdiff" --version | grep -i 'perceptualdiff'
"$pdiff" --help | grep -i 'usage'

rm -f diff.png
"$pdiff" --output ${tmpdir}/diff.png --verbose fish{1,2}.png 2>&1 | grep -q 'FAIL'
ls ${tmpdir}/diff.png
rm -f ${tmpdir}/diff.png

if ${test_bad_file}; then
head fish1.png > ${tmpdir}/fake.png
"$pdiff" --verbose fish1.png ${tmpdir}/fake.png 2>&1 | grep -q 'Failed to load'
rm -f ${tmpdir}/fake.png
fi

mkdir -p ${tmpdir}/unwritable.png
"$pdiff" --output ${tmpdir}/unwritable.png --verbose fish{1,2}.png 2>&1 | grep -q 'Failed to save'
rmdir ${tmpdir}/unwritable.png

"$pdiff" fish{1,2}.png --output ${tmpdir}/foo 2>&1 | grep -q 'unknown filetype'
"$pdiff" --verbose fish1.png 2>&1 | grep -q 'Not enough'
"$pdiff" --down-sample -3 fish1.png Aqsis_vase.png 2>&1 | grep -q 'Invalid'
"$pdiff" --threshold -3 fish1.png Aqsis_vase.png 2>&1 | grep -q 'Invalid'
"$pdiff" cam_mb_ref.tif cam_mb.tif --fake-option
"$pdiff" --verbose --scale fish1.png Aqsis_vase.png 2>&1 | grep -q 'FAIL'
"$pdiff" --down-sample 2 fish1.png Aqsis_vase.png 2>&1 | grep -q 'FAIL'
"$pdiff"  /dev/null /dev/null 2>&1 | grep -q 'Unknown filetype'
"$pdiff" --verbose --sum-errors fish{1,2}.png 2>&1 | grep -q 'sum'
"$pdiff" --color-factor .5 -threshold 1000 --gamma 3 --luminance 90 cam_mb_ref.tif cam_mb.tif
"$pdiff" --verbose -down-sample 30 -scale --luminance-only --fov 80 cam_mb_ref.tif cam_mb.tif
"$pdiff" --fov wrong fish1.png fish1.png 2>&1 | grep -q 'Invalid argument'

echo -e '\x1b[01;32mOK\x1b[0m'
