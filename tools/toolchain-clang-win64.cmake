set(CMAKE_SYSTEM_NAME Windows)

function(GetTCFileDir)
    set(TCFILEDIR "${CMAKE_CURRENT_FUNCTION_LIST_DIR}" PARENT_SCOPE)
endfunction()
GetTCFileDir()

set(CLANGBIN 
    # TODO: detect current MSVS path instead and throw some diagnostics about
    # installing Clang
    "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/Llvm/bin"
)

set(CMAKE_CXX_COMPILER "${CLANGBIN}/clang++.exe" CACHE FILEPATH "Clang C++ compiler")
set(CMAKE_C_COMPILER  "${CLANGBIN}/clang.exe"  CACHE FILEPATH "Clang C compiler")
set(CMAKE_LINKER "${CLANGBIN}/clang++.exe" CACHE FILEPATH "Clang linker")
set(CMAKE_AR "${CLANGBIN}/llvm-ar.exe" CACHE FILEPATH "Clang AR")
set(CMAKE_RANLIB "${CLANGBIN}/llvm-ranlib.exe" CACHE FILEPATH "Clang ranlib")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -target x86_64-pc-windows-msvc -fms-extensions -fms-compatibility -fms-compatibility-version=19.00 -DWIN32 -D_WIN32 -DWIN64 -D_WIN64")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -target x86_64-pc-windows-msvc -fms-extensions -fms-compatibility -fms-compatibility-version=19.00 -DWIN32 -D_WIN32 -DWIN64 -D_WIN64")
set(CMAKE_C_COMPILER_LAUNCHER "${TCFILEDIR}/ccache.exe")
set(CMAKE_CXX_COMPILER_LAUNCHER "${TCFILEDIR}/ccache.exe")
