#!/bin/bash
echo "CMAKE_BUILD_ARGS=--config RelWithDebInfo" >> $GITHUB_ENV
echo "CMAKE_CONFIGURE_ARGS=-DUSE_FMOD=ON" >> $GITHUB_ENV
