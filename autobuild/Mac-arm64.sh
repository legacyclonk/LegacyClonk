#!/bin/bash
# Install dependencies from Homebrew for ARM Mac builds,
# since no prebuilt deps package exists for this architecture yet.
brew install sdl2 sdl2_mixer freetype libpng jpeg-turbo glew fmt openssl miniupnpc catch2

echo "CMAKE_PREFIX_PATH=$(brew --prefix)" >> $GITHUB_ENV

# Skip prebuilt deps download
echo "SKIP_DEPS=1" >> $GITHUB_ENV
