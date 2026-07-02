### Moonlight-Super

#### 热键说明
##### 本地客户端
```text
CTRL+ALT+SHIFT+Q 关闭窗口，多窗口模式下，可以单独关闭某个窗口
CTRL+ALT+SHIFT+E 关闭所有窗口，并退出远程
CTRL+ALT+SHIFT+S 屏幕打印参数
CTRL+ALT+SHIFT+M 切换鼠标的模式，远程桌面模式(不捕获鼠标)、游戏模式(捕获鼠标)
CTRL+ALT+SHIFT+Z 取消捕获鼠标，释放鼠标，以移出窗口范围
CTRL+ALT+SHIFT+X 切换全屏模式，窗口模式与全屏模式之间切换
CTRL+ALT+SHIFT+D 最小化窗口
CTRL+ALT+SHIFT+C 远程桌面时，显示本地的鼠标位置，这个需要注意，可能会有滞后效果。
CTRL+ALT+SHIFT+L 将鼠标锁定在远程主机内，再次使用过热键，可以解除锁定
CTRL+ALT+SHIFT+V 复制本地剪切板内容到目标主机
```

##### 远程主机sunshine的功能
```text
CTRL+ALT+SHIFT+F1~F12 切换显示，主机如果存在多个显示器，可以进行切换，最多支持12个
```

#### 相关命令行指令
##### 更新多国语言到qm
```shell
C:\Qt\6.9.1\msvc2022_64\bin\lrelease app/languages/qml_zh_CN.ts -qm app/languages/qml_zh_CN.qm
```


#### 版本变更日志
详细参考 changelog.md


#### 安装说明
##### linux ubuntu/debian
```shell
#更新安装包版本
sudo apt update
#安装ifconfig
sudo apt install -y net-tools
#安装openssh
sudo apt install -y openssh-server
#安装git
sudo apt install -y git
#配置git存储账号密码
git config --global credential.helper store
#安装clion
sudo snap install -y clion --classic

#主要驱动相关，参考 https://github.com/moonlight-stream/moonlight-docs/wiki/Fixing-Hardware-Decoding-Problems
#安装xcb
sudo apt install -y libxcb-cursor0 libxcb-cursor-dev
#安装vdpau 组件 硬件加速需要 nvidia卡
sudo apt install -y libvdpau-dev libvdpau-va-gl1
#确认 i915 驱动和 VA-API 支持 i915集成显卡
sudo apt install -y libva-drm2 libva-x11-2 libva-dev i965-va-driver
#amd显卡
sudo apt install -y vulkan-tools

#安装 python
sudo apt install -y python3
#安装 python pip
sudo apt install -y python3-pip
#安装openssl dev库
sudo apt-get install -y libssl-dev
#安装sdl dev库
sudo apt-get install -y libsdl2-dev libsdl2-ttf-dev
#安装 opus dev库
sudo apt-get install -y libopus-dev
#安装 sound io dev库
sudo apt install -y libsoundio2 libsoundio-dev
安装ffmpeg 7.1.1
sudo add-apt-repository ppa:ubuntuhandbook1/ffmpeg7
sudo apt update
sudo apt install -y ffmpeg
sudo apt install -y libavcodec-dev libswscale-dev libavcodec-extra
sudo apt install -y build-essential yasm libx264-dev libx265-dev libvpx-dev libfdk-aac-dev libmp3lame-dev libopus-dev libtheora-dev libvorbis-dev libass-dev libwebp-dev libzimg-dev libopencore-amrwb-dev

#安装一些必要的基础库
sudo apt install -y libjpeg-dev libpng-dev
#安装 patchelf 使用脚本编译时，需要这个
sudo apt install -y patchelf
#安装qt 调试需要的gbd
sudo apt install -y gdb

#安装appimagetool
下载最新版 appimagetool（若网络不可达需切换代理或镜像源）
wget https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage 
添加执行权限并安装到系统路径 
chmod +x appimagetool-x86_64.AppImage 
sudo mv appimagetool-x86_64.AppImage /usr/local/bin/appimagetool 
#如果使用appimage，还需要这个组件
sudo apt install libfuse2t64

#安装qt creator https://mirror.maeen.sa/qtproject/official_releases/qtcreator/17.0/17.0.0/
#安装qt 6.9.3

#安装 linuxdeployqt
chmod +x linuxdeployqt
sudo mv linuxdeployqt /usr/local/bin/linuxdeployqt


#切换显示控制器
sudo dpkg-reconfigure sddm
sudo dpkg-reconfigure gdm3
```
