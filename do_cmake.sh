#!/bin/bash

cd build
cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=On \
    -DHH_DATELIB=On \
    -DIB_CLIENT_SOURCE_DIR=/Users/jmjatlanta/IBJts/source \
    ..
cd ..

