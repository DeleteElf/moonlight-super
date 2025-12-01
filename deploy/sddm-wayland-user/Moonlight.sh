#!/bin/bash
# 桌面环境变量
export LANG=zh_CN.UTF-8

DIR=$PWD

# 等待 Sway 启动并确认其正在运行
while ! pgrep -x sway > /dev/null; do
    sleep 1
done

# 系统库所需要的环境变量
export LD_LIBRARY_PATH=${DIR}

# QT环境变量
export QT_QPA_PLATFORM=wayland
export QT_QPA_EGLFS_ALWAYS_SET_MODE=0
# Wayland环境变量
export XDG_RUNTIME_DIR=/run/user/$(id -u)
export WAYLAND_DISPLAY=$(ls ${XDG_RUNTIME_DIR}/wayland-* 2>/dev/null | head | grep -v \.lock | head -n 1)
# 获取有效的 SWAYSOCK 文件
SWAYSOCKS=($(ls ${XDG_RUNTIME_DIR}/sway-* 2>/dev/null | grep \.sock))
for sock in "${SWAYSOCKS[@]}"; do
    if ss -lx | grep -q "$sock"; then
        export SWAYSOCK="$sock"
        break
    fi
done

# 启动桌面
exec ${DIR}/Moonlight
