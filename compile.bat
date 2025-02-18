@echo off

echo build started for windows

cmake -B build -G "MinGW Makefiles"
cmake --build build
call "./build/yamenu.exe"
