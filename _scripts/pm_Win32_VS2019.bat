@echo off
pushd %~dp0\..\
CALL _vendor\premake\premake5.exe vs2019
popd
PAUSE