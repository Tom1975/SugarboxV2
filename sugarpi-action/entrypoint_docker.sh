#!/bin/sh -l

echo 'Generating makefile'
mkdir build
cd build
echo 'Version : '  ${{ inputs.version }}
cmake .. -DCMAKE_BUILD_TYPE=Release -DSUGARBOX_VERSION='${{ inputs.version }}'
make package
ls -l
