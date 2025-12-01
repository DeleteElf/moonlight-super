#!/bin/bash

export LANG=zh_CN.UTF-8

DIR=$PWD
USERNAME=$(logname)
APPNAME=Moonlight
APP=${DIR}/${APPNAME}
echo 正在关闭已经运行的相关程序
systemctl disable ${APPNAME}
systemctl disable pulseaudio

systemctl stop ${APPNAME}

rm -fr /home/{USERNAME}/.config/sddm-hooks


echo 正在准备相关程序
if [ -n "$1" ]; then
    # 使用正则表达式判断是否以http开头
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
chmod +x pulseaudio.sh
chmod +x audio.sh
chmod +x start.sh
# 安装sway
apt install sway swaybg waybar fonts-font-awesome -y
# 安装音频服务
apt install pulseaudio pulseaudio-utils pavucontrol -y
#将root用户加入音频的用户组，授权root访问
usermod -aG pulse-access root
usermod -aG audio root
# 安装freerdp
apt install freerdp3-x11 freerdp3-wayland -y
# 安装依赖库
apt install libva2 alacritty libfuse2t64 libjack-jackd2-0 -y

echo 正在配置相关程序

# 配置sway环境
mkdir -p /root/.config/sway
cat > /root/.config/sway/config <<EOF
# 桌面环境变量
#exec_always {
#  export LANG=zh_CN.UTF-8
#  export LC_ALL=zh_CN.UTF-8
#  export LC_CTYPE=zh_CN.UTF-8
#}
#bar {
#  # mode invisible
#  # 可选: top, bottom, left, right
#  position top
#  # 建议24-48像素
#  height 30
#  # 默认显示状态
#  hidden_state show
#  #设置显示时间
#  status_command while date +'%Y-%m-%d %H:%M:%S'; do sleep 1; done
#}
#设备背景
output * background ${DIR}/bg.png fill
# 这个设备目前没有效果
# output "Moonlight Company Remote Desktop" pos 0 0
#声音相关
# 静音
bindsym --locked Control+F1 exec pactl set-sink-mute @DEFAULT_SINK@ toggle
# 减少音量
bindsym --locked Control+F2 exec pactl set-sink-volume @DEFAULT_SINK@ -5%
# 增加音量
bindsym --locked Control+F3 exec pactl set-sink-volume @DEFAULT_SINK@ +5%
# 麦克风静音
bindsym --locked Control+F4 exec pactl set-source-mute \@DEFAULT_SOURCE@ toggle
# 打开shell
bindsym Control+F8 exec alacritty
# 屏幕亮度控制 需要安装应用
#bindsym Control+F9 exec brightnessctl set 5%-
#bindsym Control+F10 exec brightnessctl set 5%+

#切换全屏
bindsym Control+F11 fullscreen toggle

#for_window [title="FreeRDP*"] fullscreen
for_window [app_id=".*"] border none
#这个目录下并没有内容,只是作为未来扩展配置
include /root/.config/sway/monitors
#执行后退出
#exec --no-startup-id pulseaudio --start --daemonize=no --log-target=journal &

#exec ${DIR}/audio.sh

exec_always dbus-run-session waybar

EOF


mkdir -p /root/.config/waybar

cp -f ${DIR}/waybar/power_menu.xml /root/.config/waybar/
cp -f ${DIR}/waybar/config.jsonc /root/.config/waybar/config
cp -f ${DIR}/waybar/style.css /root/.config/waybar/

# 创建sway 服务
cat > /etc/systemd/system/sway.service <<EOF
[Unit]
Description=Sway - Window Manager
StartLimitIntervalSec=0
StartLimitBurst=0

[Service]
LimitNOFILE=65535
ExecStartPre=/bin/mkdir -p /run/Moonlight
ExecStart=/usr/bin/sway
Environment="XDG_RUNTIME_DIR=/run/Moonlight"
Restart=always
RestartSec=0
RestartForceExitStatus=SIGINT
KillSignal=SIGINT
User=root
WorkingDirectory=${DIR}
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target

EOF

# 创建Moonlight服务
cat > /etc/systemd/system/Moonlight.service <<EOF
[Unit]
Description=Cg Teamwork Remote Desktop Service
After=pulseaudio.service
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
User=root
WorkingDirectory=${DIR}
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target

EOF

# /etc/pulse/daemon.conf
# resample-method = speex-float-1
sed -i 's/; resample-method = speex-float-1/resample-method = src-sinc-medium-quality/' /etc/pulse/daemon.conf
sed -i 's/resample-method = speex-float-1/resample-method = src-sinc-medium-quality/' /etc/pulse/daemon.conf
sed -i 's/; default-sample-format = s16le/default-sample-format = s24le/' /etc/pulse/daemon.conf
sed -i 's/; default-sample-rate = 44100/default-sample-rate = 48000/' /etc/pulse/daemon.conf

cp -f ${DIR}/system.pa /etc/pulse/system.pa

# 启动声音服务 目前这个声音服务有问题,启动会引起声音故障
cat > /etc/systemd/system/pulseaudio.service <<EOF
[Unit]
Description=PulseAudio System Sound Server
Requires=sway.service
StartLimitIntervalSec=0
StartLimitBurst=0

[Service]
LimitNOFILE=65535
ExecStart=${DIR}/pulseaudio.sh
Restart=always
RestartSec=1
RestartForceExitStatus=SIGINT
KillSignal=SIGINT
User=root
WorkingDirectory=${DIR}
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target

EOF

systemctl daemon-reload

# 禁用tty1
systemctl disable getty@tty1.service
# 启用服务
systemctl enable sway
systemctl enable pulseaudio
systemctl enable ${APPNAME}
#
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
systemctl set-default multi-user.target
# 恢复原桌面环境
#sudo systemctl set-default graphical.target

echo "正在配置完成，请重启电脑！"

else
  echo "文件不存在！您可以直接将程序包改名为 ${APPNAME} ,然后再次运行sudo install.sh,也可以通过 以下命令直接安装:"
  echo "sudo install.sh filename"
  echo "sudo install.sh http[s]://downloadurl"
fi