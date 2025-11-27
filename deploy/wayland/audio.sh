#!/bin/bash
# 获取音量百分比，这里以默认的sink为例，如果你的默认设备不是0号，请替换相应的编号
volume=$(pactl get-sink-volume @DEFAULT_SINK@ | grep 'Volume:' | head -n1 | awk '{print $5}' | sed 's/%//')
# 检查音量是否为100%
while [ "$volume" -ne 100 ]; do
    echo "当前音量：$volume%"
    # 如果不是100%，则等待100毫秒
    sleep 0.05
    pactl set-sink-volume @DEFAULT_SINK@ 100%
    # 重新获取音量
    volume=$(pactl get-sink-volume @DEFAULT_SINK@ | grep 'Volume:' | head -n1 | awk '{print $5}' | sed 's/%//')
done
