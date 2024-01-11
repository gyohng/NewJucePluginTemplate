#!/bin/sh

set -e

PROJECT_NAME="My Audio Plugin"

# Pick your Microsoft Visual Studio version
MSVS_VERSION="Visual Studio 17 2022"

MSVS_PATH="$("C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere" -latest -property installationPath)"
MSVS_PATH="$(echo $MSVS_PATH | sed 's,\\,/,g')"

MYDIR="$(dirname "$0")"
MYDIR="$( cd "$MYDIR" ; pwd -P )"

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

    cmake -G "Ninja" "-DCMAKE_TOOLCHAIN_FILE=${MYDIR}/toolchain-clang-$platform.cmake" -DCMAKE_BUILD_TYPE=Release ../..
    cmake --build . --config Release
done

# Creatin combined result
cd "$MYDIR/build"
rm -rf output
mkdir -p output
cd output

if [ -f "../win64/${PROJECT_NAME}_artefacts/Release/AAX/${PROJECT_NAME}.aaxplugin/desktop.ini" ]; then
    mkdir -p "${PROJECT_NAME}.aaxplugin/Contents/x64"
    mkdir -p "${PROJECT_NAME}.aaxplugin/Contents/Win32"

    cp "../win64/${PROJECT_NAME}_artefacts/Release/AAX/${PROJECT_NAME}.aaxplugin/"desktop.ini "${PROJECT_NAME}.aaxplugin/"
    cp "../win64/${PROJECT_NAME}_artefacts/Release/AAX/${PROJECT_NAME}.aaxplugin/"Plugin.ico "${PROJECT_NAME}.aaxplugin/"
    cp "../win64/${PROJECT_NAME}_artefacts/Release/AAX/${PROJECT_NAME}.aaxplugin/"Contents/x64/* "${PROJECT_NAME}.aaxplugin/Contents/x64"
    cp "../win32/${PROJECT_NAME}_artefacts/Release/AAX/${PROJECT_NAME}.aaxplugin/"Contents/Win32/* "${PROJECT_NAME}.aaxplugin/Contents/Win32"
fi

mkdir -p "${PROJECT_NAME}.vst3/Contents/x86_64-win"
mkdir -p "${PROJECT_NAME}.vst3/Contents/x86-win"

cp "../win64/${PROJECT_NAME}_artefacts/Release/VST3/${PROJECT_NAME}.vst3/"desktop.ini "${PROJECT_NAME}.vst3/"
cp "../win64/${PROJECT_NAME}_artefacts/Release/VST3/${PROJECT_NAME}.vst3/"Plugin.ico "${PROJECT_NAME}.vst3/"
cp "../win64/${PROJECT_NAME}_artefacts/Release/VST3/${PROJECT_NAME}.vst3/"Contents/x86_64-win/* "${PROJECT_NAME}.vst3/Contents/x86_64-win"
cp "../win32/${PROJECT_NAME}_artefacts/Release/VST3/${PROJECT_NAME}.vst3/"Contents/x86-win/* "${PROJECT_NAME}.vst3/Contents/x86-win"

cp "../win64/${PROJECT_NAME}_artefacts/Release/VST/${PROJECT_NAME}.dll" "${PROJECT_NAME}64.dll"
cp "../win32/${PROJECT_NAME}_artefacts/Release/VST/${PROJECT_NAME}.dll" "${PROJECT_NAME}32.dll"

cp "../win64/${PROJECT_NAME}_artefacts/Release/Standalone/${PROJECT_NAME}.exe" "${PROJECT_NAME}64.exe"
cp "../win32/${PROJECT_NAME}_artefacts/Release/Standalone/${PROJECT_NAME}.exe" "${PROJECT_NAME}32.exe"
