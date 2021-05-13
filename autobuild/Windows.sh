#!/bin/bash
echo "CMAKE_BUILD_ARGS=--config RelWithDebInfo" >> $GITHUB_ENV
echo "CMAKE_CONFIGURE_ARGS=-DUSE_FMOD=ON" >> $GITHUB_ENV
echo "set(EXTRA_LINK_LIBS user32 gdi32 winmm ole32 oleaut32 uuid advapi32 shell32 CACHE STRING \"Extra libs needed for static linking\")" > config.txt
