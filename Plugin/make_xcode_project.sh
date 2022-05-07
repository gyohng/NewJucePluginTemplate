#!/bin/sh

set -e

if ! which cmake >/dev/null; then 
    echo Error: cmake not found
    echo install: brew install cmake
    exit 1
fi

if ! which ccache >/dev/null; then 
    echo Error: ccache not found
    echo install: brew install cccache
    exit 1
fi

export CMAKE_C_COMPILER_LAUNCHER=ccache
export CMAKE_CXX_COMPILER_LAUNCHER=ccache
export CMAKE_OBJC_COMPILER_LAUNCHER=ccache

mkdir -p xcode_project
cd xcode_project

cmake -G Xcode -DUSE_XCODE=1 -DCOPY_PLUGINS=1 ..
cd ..
