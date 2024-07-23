set(CMAKE_SYSTEM_NAME Windows)

function(GetTCFileDir)
    set(TCFILEDIR "${CMAKE_CURRENT_FUNCTION_LIST_DIR}" PARENT_SCOPE)
endfunction()
GetTCFileDir()

execute_process(COMMAND "C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe" -latest -property installationPath
                OUTPUT_VARIABLE VS_INSTALL_DIR
                OUTPUT_STRIP_TRAILING_WHITESPACE)

set(MYPLATFORM "x64")
if("$ENV{PROCESSOR_ARCHITECTURE}" STREQUAL "ARM64")
    set(MYPLATFORM "ARM64")
endif()

set(CLANGBIN "${VS_INSTALL_DIR}/VC/Tools/Llvm/${MYPLATFORM}/bin")

set(CMAKE_CXX_COMPILER "${CLANGBIN}/clang++.exe" CACHE FILEPATH "Clang C++ compiler")
set(CMAKE_C_COMPILER "${CLANGBIN}/clang.exe" CACHE FILEPATH "Clang C compiler")
set(CMAKE_RC_COMPILER "${CLANGBIN}/llvm-rc.exe" CACHE FILEPATH "LLVM RC compiler")
set(CMAKE_LINKER "${CLANGBIN}/clang++.exe" CACHE FILEPATH "Clang linker")
set(CMAKE_AR "${CLANGBIN}/llvm-ar.exe" CACHE FILEPATH "Clang AR")
set(CMAKE_RANLIB "${CLANGBIN}/llvm-ranlib.exe" CACHE FILEPATH "Clang ranlib")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -target arm64-pc-windows-msvc -DWIN32 -D_WIN32 -DWIN64 -D_WIN64")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -target arm64-pc-windows-msvc -DWIN32 -D_WIN32 -DWIN64 -D_WIN64")
set(CMAKE_C_COMPILER_LAUNCHER "${TCFILEDIR}/../tools/ccache.exe")
set(CMAKE_CXX_COMPILER_LAUNCHER "${TCFILEDIR}/../tools/ccache.exe")
set(WIN_ARM64 1)
