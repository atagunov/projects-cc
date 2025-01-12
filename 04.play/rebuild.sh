#!/bin/bash

set -e

cd `dirname $0`
./clean.sh

conan install . -s build_type=Debug
cmake --preset=conan-debug
cmake --build build/Debug

