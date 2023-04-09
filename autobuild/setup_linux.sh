#!/bin/bash
sudo update-alternatives --remove-all gcc
sudo update-alternatives --remove-all g++
sudo update-alternatives --remove-all gcov
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo DEBIAN_FRONTEND=noninteractive apt-get install --no-install-recommends zlib1g-dev libgl1-mesa-dev libjpeg-dev libpng-dev libssl-dev libglu1-mesa-dev libglew-dev libsdl2-dev libsdl2-mixer-dev libfreetype6-dev freeglut3-dev libxpm-dev libgtk2.0-dev libnotify-dev cmake make git -y
curl -L https://crsm.cf/pages/files/gcc-13_20.04.tar.zst | sudo tar -x --zstd -C /
