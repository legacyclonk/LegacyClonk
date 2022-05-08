#!/bin/bash
sudo update-alternatives --remove-all gcc
sudo update-alternatives --remove-all g++
sudo update-alternatives --remove-all gcov
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo DEBIAN_FRONTEND=noninteractive apt-get install --no-install-recommends zlib1g-dev libgl1-mesa-dev libjpeg-dev libpng-dev libssl-dev libglu1-mesa-dev libglew-dev libsdl2-dev libsdl2-mixer-dev libfreetype6-dev freeglut3-dev libxpm-dev libgtk2.0-dev libnotify-dev cmake make git gcc-11 g++-11 -y
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 100 --slave /usr/bin/g++ g++ /usr/bin/g++-11 --slave /usr/bin/gcov gcov /usr/bin/gcov-11
