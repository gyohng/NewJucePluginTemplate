@echo off

REM Use bash-like script for windows builds as well

cd "%~dp0"
set PATH=%~dp0..\tools;%PATH%

mkdir msvs_project 2>nul
cd msvs_project
cmake -G "Visual Studio 17 2022" -DUSE_MSVS=1 -DCOPY_PLUGINS=1 -T host=x64 -A x64 ..
