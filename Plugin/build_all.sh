#!/bin/sh

set -e

# Set the plugin binary to match the CMake project
BINARY_NAME="Plugin"
DEVELOPER_SIGNATURE="Developer ID Application"

# If you don't have PACE SDK installed, you can ignore this data
PACE_ACCOUNT=YourPaceAccount
PACE_PASSWORD=YourPaceAccountPassword
PACE_WCGUID=141991C2-DB49-452B-96E0-87B405A0544E

MYDIR="$(dirname "$0")"
MYDIR="$( cd "$MYDIR" ; pwd -P )"

cd "$MYDIR"

if [ ! -d "$HOME/MacOSX-SDKs" ]; then
    git clone https://github.com/phracker/MacOSX-SDKs.git "$HOME/MacOSX-SDKs"
fi

if ! which ninja >/dev/null; then 
    echo Error: ninja not found
    echo install: brew install ninja
    exit 1
fi

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

splice() {
    cd "$MYDIR"

    ditto "build/macos10/${BINARY_NAME}_artefacts/Release/$1/$2" "build/output/$2"
    OUTPUTBIN=`echo "build/output/$2/Contents/MacOS/"*`
    MACOS11BIN=`echo "build/macos11/${BINARY_NAME}_artefacts/Release/$1/$2/Contents/MacOS/"*`

    strip -x -S "$OUTPUTBIN"
    strip -x -S "$MACOS11BIN"
    if lipo -info "$MACOS11BIN" | grep 'arm64' >/dev/null; then
        lipo "$MACOS11BIN" -thin arm64 -output build/output/arm64bin
        lipo -create "$OUTPUTBIN" build/output/arm64bin -output "$OUTPUTBIN"
        rm -rf build/output/arm64bin
    fi
    strip -x -S "$OUTPUTBIN"

    codesign -v -f -o runtime --timestamp --deep \
        --preserve-metadata=entitlements \
        --sign "$DEVELOPER_SIGNATURE" \
        --digest-algorithm=sha1,sha256 \
        "build/output/$2"
}

spliceAll() {
    splice AAX "$1.aaxplugin"
    splice AU "$1.component"
    splice AUv3 "$1.appex"
    splice Standalone "$1.app"
    splice VST "$1.vst"
    splice VST3 "$1.vst3"

    if [ -f /Applications/PACEAntiPiracy/Eden/Fusion/Current/bin/wraptool ]; then
        mv "build/output/$1.aaxplugin" "build/output/$1.unsigned.aaxplugin"
        /Applications/PACEAntiPiracy/Eden/Fusion/Current/bin/wraptool \
            sign --verbose --strip on \
            --account "$PACE_ACCOUNT" --password "$PACE_PASSWORD" \
            --signid "$DEVELOPER_SIGNATURE" \
            --wcguid "$WCGUID" \
            --in "build/output/$1.unsigned.aaxplugin" --out "build/output/$1.aaxplugin" || true
    fi
}

# Compile plugins and standalone
rm -rf build/output
mkdir -p build/output

rm -rf "build/macos10"
mkdir -p "build/macos10"
cd "build/macos10"
cmake -G Ninja -DUSE_BUILDNUMBER=1 -DBUILD_MACOS11=0 -DCMAKE_BUILD_TYPE=Release ../..
cmake --build .
cd "$MYDIR"

rm -rf "build/macos11"
mkdir -p "build/macos11"
cd "build/macos11"
cmake -G Ninja -DUSE_BUILDNUMBER=1 -DBUILD_MACOS11=1 -DCMAKE_BUILD_TYPE=Release ../..
cmake --build .
cd "$MYDIR"

spliceAll "$BINARY_NAME"
