@echo off
if not exist build mkdir build
pushd build
gcc ..\source\main.c -o main.exe -lwinmm -std=c11 
popd