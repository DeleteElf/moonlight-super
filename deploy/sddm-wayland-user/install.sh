#!/bin/bash

export LANG=zh_CN.UTF-8

DIR=$PWD
USERNAME=$(logname)
APPNAME=Moonlight
APP=${DIR}/${APPNAME}
echo 正在关闭已经运行的相关程序
systemctl disable ${APPNAME}
systemctl disable sway

systemctl stop ${APPNAME}

rm -fr /home/{USERNAME}/.config/sddm-hooks
rm -f /usr/share/xsessions/sway.desktop

echo 正在准备相关程序
if [ -n "$1" ]; then
    # 使用正则表达式判断是否以http开头swa
    if [[ $url =~ ^http ]]; then
       echo 正在下载目标应用程序
       curl -o $APP -L -O $1
       echo 下载已完成
    else
       cp $1 $APP -f
    fi
fi

if [ -e "$APP" ]; then

chmod +x ${APP}
chmod +x ${APP}.sh
chmod +x start.sh
# 安装sway
apt install sway swaybg waybar -y
# 安装音频服务 普通用户模式无需安装
apt install pulseaudio-utils -y
# 安装依赖库
apt install alacritty libfuse2t64 libjack-jackd2-0 -y

echo 正在配置相关程序

# 配置sway环境
mkdir -p /home/${USERNAME}/.config/sway
cat > /home/${USERNAME}/.config/sway/config <<EOF

output * background ${DIR}/bg.png fill
#声音相关
# 静音
bindsym Control+F5 exec pactl set-sink-mute @DEFAULT_SINK@ toggle
# 减少音量
bindsym Control+F6 exec pactl set-sink-volume @DEFAULT_SINK@ -5%
# 增加音量
bindsym Control+F7 exec pactl set-sink-volume @DEFAULT_SINK@ +5%

# 打开shell
bindsym Control+F8 exec alacritty

# 屏幕亮度控制 需要安装应用
#bindsym Control+F9 exec brightnessctl set 5%-
#bindsym Control+F10 exec brightnessctl set 5%+

#切换全屏
bindsym Control+F11 fullscreen toggle


#这里我们使用X11所以会导致原来的全屏没有效果，修改一下捕获的标题,使用swaymsg -t get_tree 获取层级树
#for_window [app_id="com.moonlight_stream.Moonlight"] fullscreen
#去掉边框线
for_window [app_id=".*"] border none
#这个目录下并没有内容,只是作为未来扩展配置
include /root/.config/sway/monitors

# 启动应用程序  注意这边因为使用普通用户登录waybar，所以需要 dbus-run-session来授权
exec_always dbus-run-session waybar
EOF

mkdir -p /home/${USERNAME}/.config/waybar

cat  > /home/${USERNAME}/.config/waybar/config <<EOF
{
    "layer": "top",
    "position": "top",
    "height": 30,
    "modules-left": ["sway/workspaces", "sway/mode"],
    "modules-center": ["sway/window"],
    "modules-right": [
        "pulseaudio",
        "network",
        "cpu",
        "memory",
        "temperature",
        "battery",
        "clock",
        "tray"
    ]
}
EOF

# 创建sddm托管的sway
cat > /usr/share/wayland-sessions/sway.desktop <<EOF
[Desktop Entry]
Encoding=UTF-8
Name=Sway
Comment=An i3-compatible Wayland compositor
Exec=env XDG_CURRENT_DESKTOP=sway sway
Icon=sway
Type=Application
DesktopNames=sway;wlroots

EOF

cat > /etc/sddm.conf <<EOF
[Autologin]
User=${USERNAME}
Session=sway

EOF
#替换配置
sed -i "s/^GRUB_CMDLINE_LINUX_DEFAULT='quiet splash''/GRUB_CMDLINE_LINUX_DEFAULT='quiet splash wayland'/" /etc/default/grub
sed -i "s/^GRUB_CMDLINE_LINUX_DEFAULT='quiet splash wayland systemd.fastboot=1'/GRUB_CMDLINE_LINUX_DEFAULT='quiet splash wayland'/" /etc/default/grub
update-grub

# 创建Moonlight服务
cat > Moonlight.service <<EOF
[Unit]
Description=Cg Teamwork Remote Desktop Service
Requires=sddm.service
#重试的时间
StartLimitIntervalSec=0
#重试的次数
StartLimitBurst=0

[Service]
LimitNOFILE=65535
ExecStart=${APP}.sh
ExecStop=/bin/kill -9 $MAINPID
Restart=always
RestartSec=1
RestartForceExitStatus=SIGINT
User=${USERNAME}
WorkingDirectory=${DIR}
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target

EOF

cp ./Moonlight.service /etc/systemd/system/Moonlight.service

systemctl daemon-reload

# 启用tty1
systemctl enable getty@tty1.service
# 启用服务
systemctl enable ${APPNAME}

## 安装usbip
#cat > /etc/modules-load.d/usbip.conf <<EOF
#usbip_host
#vhci_hcd
#EOF

#关闭软件自升级
sed -i 's/Update-Package-Lists "1"/Update-Package-Lists "0"/' /etc/apt/apt.conf.d/20auto-upgrades
sed -i 's/Unattended-Upgrade "1"/Unattended-Upgrade "0"/' /etc/apt/apt.conf.d/20auto-upgrades

#关闭无人值守更新功能
systemctl stop unattended-upgrades
systemctl disable unattended-upgrades

# 禁用原桌面环境
#systemctl set-default multi-user.target
# 恢复原桌面环境
sudo systemctl set-default graphical.target

echo "正在配置完成，请重启电脑！"

else
  echo "文件不存在！您可以直接将程序包改名为 ${APPNAME} ,然后再次运行sudo install.sh,也可以通过 以下命令直接安装:"
  echo "sudo install.sh filename"
  echo "sudo install.sh http[s]://downloadurl"
fi