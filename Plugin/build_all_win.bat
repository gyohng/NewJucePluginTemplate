@echo off

REM Use bash-like script for windows builds as well

cd "%~dp0"
set PATH=%~dp0..\tools;%PATH%
..\tools\busybox64 bash ./windows-build_all.sh
