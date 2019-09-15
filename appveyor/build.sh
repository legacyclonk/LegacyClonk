#!/bin/bash
sudo DEBIAN_FRONTEND=noninteractive apt-get install zlib1g-dev build-essential libgl1-mesa-dev libjpeg-dev libpng-dev libssl-dev libglu1-mesa-dev libglew-dev libsdl1.2-dev libsdl-mixer1.2-dev libfreetype6-dev freeglut3-dev libxpm-dev cmake make git -y
sudo update-alternatives --set gcc /usr/bin/gcc-8
git status || cd /legacyclonk
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX="/usr" -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
make -j$(nproc)
test -e clonk && tar -cvzf LegacyClonk-${PLATFORM}.tar.gz clonk c4group
