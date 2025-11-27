#!/bin/bash

DIR=$PWD
export LD_LIBRARY_PATH=${DIR}
export QT_QPA_PLATFORM=wayland
exec ${DIR}/Moonlight
