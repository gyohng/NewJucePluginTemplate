#!/bin/sh

set -e

CMAKE_PROJECT_NAME="MyAudioPlugin"
PRODUCT_NAME="My Audio Plugin"

# Pick your Microsoft Visual Studio version
MSVS_VERSION="Visual Studio 17 2022"

MSVS_PATH="`"C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere" -latest -property installationPath`"
set +e # workaround for busybox64 bash
MSVS_PATH="$(echo "$MSVS_PATH" | sed 's,\\,/,g' )"

MYDIR="$(dirname "$0")"
MYDIR="$( cd "$MYDIR" ; pwd -P )"
set -e # workaround for busybox64 bash

cd "$MYDIR"
mkdir -p build

CMAKEPATH=
for f in \
    "$MSVS_PATH/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" \
    "`which cmake`" \
    "C:/Program Files/CMake/bin/cmake.exe" \
; do
    if [ -f "$f" ]; then
        CMAKEPATH="$f"
        break
    fi
done

if [ "$CMAKEPATH" == "" ]; then
    echo Please install CMake for Visual Studio
    echo Alternatively install CMake from https://cmake.org/download/
    exit -1
fi

cmake() {
    "$CMAKEPATH" "$@"
}

for platform in win64 win32; do
    cd "$MYDIR/build"
    rm -rf $platform
    mkdir -p $platform
    cd $platform

    cmake -G "Ninja" \
        "-DCMAKE_TOOLCHAIN_FILE=${MYDIR}/../tools/toolchain-clang-$platform.cmake" \
        -DCMAKE_BUILD_TYPE=Release \
        -DUSE_BUILDNUMBER=1 \
        ../..
    cmake --build . --config Release
done

# Creatin combined result
cd "$MYDIR/build"
rm -rf output
mkdir -p output
cd output

if [ -f "../win64/${CMAKE_PROJECT_NAME}_artefacts/Release/AAX/${PRODUCT_NAME}.aaxplugin/desktop.ini" ]; then
    mkdir -p "${PRODUCT_NAME}.aaxplugin/Contents/x64"
    mkdir -p "${PRODUCT_NAME}.aaxplugin/Contents/Win32"

    cp "../win64/${CMAKE_PROJECT_NAME}_artefacts/Release/AAX/${PRODUCT_NAME}.aaxplugin/"desktop.ini "${PRODUCT_NAME}.aaxplugin/"
    cp "../win64/${CMAKE_PROJECT_NAME}_artefacts/Release/AAX/${PRODUCT_NAME}.aaxplugin/"Plugin.ico "${PRODUCT_NAME}.aaxplugin/"
    cp "../win64/${CMAKE_PROJECT_NAME}_artefacts/Release/AAX/${PRODUCT_NAME}.aaxplugin/"Contents/x64/* "${PRODUCT_NAME}.aaxplugin/Contents/x64"
    cp "../win32/${CMAKE_PROJECT_NAME}_artefacts/Release/AAX/${PRODUCT_NAME}.aaxplugin/"Contents/Win32/* "${PRODUCT_NAME}.aaxplugin/Contents/Win32"
fi

mkdir -p "${PRODUCT_NAME}.vst3/Contents/Resources"
mkdir -p "${PRODUCT_NAME}.vst3/Contents/x86_64-win"
mkdir -p "${PRODUCT_NAME}.vst3/Contents/x86-win"

cp "../win64/${CMAKE_PROJECT_NAME}_artefacts/Release/VST3/${PRODUCT_NAME}.vst3/"desktop.ini "${PRODUCT_NAME}.vst3/"
cp "../win64/${CMAKE_PROJECT_NAME}_artefacts/Release/VST3/${PRODUCT_NAME}.vst3/"Plugin.ico "${PRODUCT_NAME}.vst3/"
cp "../win64/${CMAKE_PROJECT_NAME}_artefacts/Release/VST3/${PRODUCT_NAME}.vst3/"Contents/Resources/* "${PRODUCT_NAME}.vst3/Contents/Resources"
cp "../win64/${CMAKE_PROJECT_NAME}_artefacts/Release/VST3/${PRODUCT_NAME}.vst3/"Contents/x86_64-win/* "${PRODUCT_NAME}.vst3/Contents/x86_64-win"
cp "../win32/${CMAKE_PROJECT_NAME}_artefacts/Release/VST3/${PRODUCT_NAME}.vst3/"Contents/x86-win/* "${PRODUCT_NAME}.vst3/Contents/x86-win"

cp "../win64/${CMAKE_PROJECT_NAME}_artefacts/Release/VST/${PRODUCT_NAME}.dll" "${PRODUCT_NAME}64.dll"
cp "../win32/${CMAKE_PROJECT_NAME}_artefacts/Release/VST/${PRODUCT_NAME}.dll" "${PRODUCT_NAME}32.dll"

cp "../win64/${CMAKE_PROJECT_NAME}_artefacts/Release/Standalone/${PRODUCT_NAME}.exe" "${PRODUCT_NAME}64.exe"
cp "../win32/${CMAKE_PROJECT_NAME}_artefacts/Release/Standalone/${PRODUCT_NAME}.exe" "${PRODUCT_NAME}32.exe"
