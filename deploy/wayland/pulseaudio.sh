#!/bin/bash

DIR=$PWD

# 等待 Sway 启动并确认其正在运行
while ! pgrep -x sway > /dev/null; do
    sleep 1
done

# Wayland环境变量
export XDG_RUNTIME_DIR=/run/Moonlight
export WAYLAND_DISPLAY=$(ls ${XDG_RUNTIME_DIR}/wayland-* 2>/dev/null | head | grep -v \.lock | head -n 1)
# 获取有效的 SWAYSOCK 文件
SWAYSOCKS=($(ls ${XDG_RUNTIME_DIR}/sway-* 2>/dev/null | grep \.sock))
for sock in "${SWAYSOCKS[@]}"; do
    if ss -lx | grep -q "$sock"; then
        export SWAYSOCK="$sock"
        break
    fi
done

# 启动pulseaudio
if [ "$USER" != "root" ]; then #因为root用户不能运行
  exec /usr/bin/pulseaudio --start --daemonize=no --log-target=journal
else
  #运行在系统模式下
  echo "pulseaudio run in root user"
  exec /usr/bin/pulseaudio --system --disallow-exit --disallow-module-loading --daemonize=no --log-target=journal
fi