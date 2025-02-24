#!/bin/bash

cmake vendor/SDL/ -B vendor/SDL/build
cmake --build vendor/SDL/build
cmake vendor/cglm/ -B vendor/cglm/build
cmake --build vendor/cglm/build

cmake -B build 
cmake --build build
./build/yamenu

