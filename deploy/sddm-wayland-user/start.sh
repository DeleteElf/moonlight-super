#!/bin/bash

DIR=$PWD
export LD_LIBRARY_PATH=${DIR}

export QT_QPA_PLATFORM=wayland
export QT_QPA_EGLFS_ALWAYS_SET_MODE=0

exec ${DIR}/Moonlight
