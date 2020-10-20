#!/bin/bash
echo "CMAKE_CONFIGURE_ARGS=-DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_FLAGS=-Wno-parentheses -DCMAKE_OSX_DEPLOYMENT_TARGET=10.9 -C $PWD/config.txt" >> $GITHUB_ENV
LIBS=("\"-framework "{CoreAudio,CoreVideo,AudioToolbox,AudioUnit,ForceFeedback,IOKit,Cocoa,Foundation,AVFoundation,Carbon}\")
echo "set(EXTRA_LINK_LIBS ${LIBS[@]} CACHE STRING \"Extra libs needed for static linking\")" > config.txt

cat autobuild/mac_any_cast.h >> src/C4InteractiveThread.h
