#!/bin/bash
export MACHINE=i686
ARGS=("cd $PWD;" "$@")
eval $(echo sudo -E chroot chroot bash -c "'${ARGS[@]}'")
