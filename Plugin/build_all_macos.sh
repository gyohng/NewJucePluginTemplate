#!/bin/sh

set -e

# Set the project name and product name to match the CMake project
CMAKE_PROJECT_NAME="MyAudioPlugin"
PRODUCT_NAME="My Audio Plugin"
DEVELOPER_SIGNATURE="Developer ID Application"

# If you don't have PACE SDK installed, you can ignore this data
# It's best not to put the data here but inside .paceaccount file
# in the same folder, which is not submitted to git
PACE_ACCOUNT=SpecifyInDotPaceaccountFileInstead
PACE_PASSWORD=SpecifyInDotPaceaccountFileInstead

# This one is safe to put here, please change in place
PACE_WCGUID=141991C2-DB49-452B-96E0-87B405A0544E

# Note: do not pay attention to the warnings that say that some object files
# were built for newer macOS version (10.11) than being linked (10.9)

MYDIR="$(dirname "$0")"
MYDIR="$( cd "$MYDIR" ; pwd -P )"

cd "$MYDIR"

# this file should contain PACE_ACCOUNT and PACE_PASSWORD setting
if [ -f .paceaccount ]; then
    . ./.paceaccount
fi

NOTFOUND_PACKAGES=""

for f in git ninja cmake ccache wget; do
    if ! which $f >/dev/null; then 
        NOTFOUND_PACKAGES="$NOTFOUND_PACKAGES $f"
    fi
done

if [ "$NOTFOUND_PACKAGES" != "" ]; then
    echo Error: the following packages were not found:$NOTFOUND_PACKAGES
    echo install: brew install$NOTFOUND_PACKAGES
    exit 1
fi

if [ ! -d "$HOME/MacOSX-SDKs" ]; then
    git clone https://github.com/phracker/MacOSX-SDKs.git "$HOME/MacOSX-SDKs"
fi

#if [ ! -d "$MYDIR/../JUCE/sdks/vst3sdk" ]; then
#    cd "$MYDIR/../JUCE/sdks"
#    ./download-vst3-sdk.sh
#    cd "$MYDIR"
#fi

export CMAKE_C_COMPILER_LAUNCHER=ccache
export CMAKE_CXX_COMPILER_LAUNCHER=ccache
export CMAKE_OBJC_COMPILER_LAUNCHER=ccache

splice() {
    cd "$MYDIR"

    ditto "build/macos10/${CMAKE_PROJECT_NAME}_artefacts/Release/$1/$2" "build/output/$2"
    OUTPUTBIN=`echo "build/output/$2/Contents/MacOS/"*`
    MACOS11BIN=`echo "build/macos11/${CMAKE_PROJECT_NAME}_artefacts/Release/$1/$2/Contents/MacOS/"*`

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

    splice AU "$1.component"
    splice AUv3 "$1.appex"
    splice Standalone "$1.app"
    splice VST "$1.vst"
    splice VST3 "$1.vst3"

    if [ -d "build/macos10/${CMAKE_PROJECT_NAME}_artefacts/Release/AAX" ]; then
        splice AAX "$1.aaxplugin"
        if [ -f /Applications/PACEAntiPiracy/Eden/Fusion/Current/bin/wraptool ]; then
            mv "build/output/$1.aaxplugin" "build/output/$1.unsigned.aaxplugin"
            /Applications/PACEAntiPiracy/Eden/Fusion/Current/bin/wraptool \
                sign --verbose --strip on \
                --account "$PACE_ACCOUNT" --password "$PACE_PASSWORD" \
                --signid "$DEVELOPER_SIGNATURE" \
                --wcguid "$PACE_WCGUID" \
                --removeunsupportedarchs \
                --dsig1-compat on \
                --extrasigningoptions "--digest-algorithm=sha1,sha256 --preserve-metadata=entitlements" \
                --in "build/output/$1.unsigned.aaxplugin" --out "build/output/$1.aaxplugin" \
                --autoinstall on
        fi
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

spliceAll "$PRODUCT_NAME"
