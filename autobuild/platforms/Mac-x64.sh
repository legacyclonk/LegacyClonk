#!/bin/bash

echo "CMAKE_CONFIGURE_ARGS=$CMAKE_CONFIGURE_ARGS -DCMAKE_OSX_DEPLOYMENT_TARGET=13.3" >> $GITHUB_ENV

curl -L https://github.com/phracker/MacOSX-SDKs/releases/download/11.3/MacOSX10.9.sdk.tar.xz | tar xJ
echo "OSX_SDK=$PWD/MacOSX10.9.sdk" >> $GITHUB_ENV
