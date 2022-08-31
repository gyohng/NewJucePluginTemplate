#!/bin/sh

set -e

# Pick your Microsoft Visual Studio version
MSVS_VERSION="Visual Studio 16 2019"
#MSVS_VERSION="Visual Studio 17 2022"

MYDIR="$(dirname "$0")"
MYDIR="$( cd "$MYDIR" ; pwd -P )"

cd "$MYDIR"
mkdir -p build

# Building a 64-bit version
cd "$MYDIR/build"
rm -rf win64
mkdir -p win64
cd win64

# using MSBuild/CL
#cmake -G "${MSVS_VERSION}" -T host=x64 -A x64 ../..
#cmake --build . --config Release

# using Clang (requires installation)
cmake -G "Ninja" "-DCMAKE_TOOLCHAIN_FILE=${MYDIR}/../tools/toolchain-clang-win64.cmake"  -DCMAKE_BUILD_TYPE=Release ../..
cmake --build . --config Release

# ======= DELETE EVERYTHING BELOW THIS POINT IF YOU DON'T NEED x86 32-bit =======

# Building a 32-bit version
cd "$MYDIR/build"
rm -rf win32
mkdir -p win32
cd win32

# using MSBuild/CL
#cmake -G "${MSVS_VERSION}" -T host=x64 -A Win32 ../..
#cmake --build . --config Release

# using Clang (requires installation)
cmake -G "Ninja" "-DCMAKE_TOOLCHAIN_FILE=${MYDIR}/../tools/toolchain-clang-win32.cmake"  -DCMAKE_BUILD_TYPE=Release ../..
cmake --build . --config Release
