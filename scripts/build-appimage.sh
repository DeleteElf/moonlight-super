BUILD_CONFIG="release"

fail()
{
	echo "$1" 1>&2
	exit 1
}
#回到上级目录
cd ..
#设置编译环境
BUILD_ROOT=$PWD/build
SOURCE_ROOT=$PWD
BUILD_FOLDER=$BUILD_ROOT/build-$BUILD_CONFIG
DEPLOY_FOLDER=$BUILD_ROOT/deploy-$BUILD_CONFIG
INSTALLER_FOLDER=$BUILD_ROOT/installer-$BUILD_CONFIG
VERSION=`cat $SOURCE_ROOT/app/version.txt`

while [[ "$#" -gt 0 ]]; do
  case "$1" in
      -w|--wayland) #官方的意思是，不用支持wayland
        MODE="wayland"
        VERSION="${VERSION}-wayland"
        echo "set wayland mode success"
        shift 1
      ;;
      -c|--clear) #官方的意思是，不用支持wayland
        #清理历史记录
        echo "Cleaning output directories"
        rm -rf $BUILD_FOLDER
        rm -rf $DEPLOY_FOLDER
        rm -rf $INSTALLER_FOLDER
        shift 1
        ;;
      *)
        echo "unknown args"
        exit 1
      ;;  esac
done

#设置qt环境
export QT_ROOT=~/Qt/6.9.3/gcc_64
echo "QT_ROOT=$QT_ROOT"
export PATH=$QT_ROOT/bin:$PATH
export LD_LIBRARY_PATH=$QT_ROOT/lib:$LD_LIBRARY_PATH
export QT_PLUGIN_PATH=$QT_ROOT/plugins:$QT_PLUGIN_PATH

export QML2_IMPORT_PATH=$QT_ROOT/qml:$QML2_IMPORT_PATH

date
#拉取代码
git pull

command -v qmake >/dev/null 2>&1 || fail "Unable to find 'qmake' in your PATH!"
command -v linuxdeployqt >/dev/null 2>&1 || fail "Unable to find 'linuxdeployqt' in your PATH!"

mkdir $BUILD_ROOT
mkdir $BUILD_FOLDER
mkdir $DEPLOY_FOLDER
mkdir $INSTALLER_FOLDER
#配置项目
date
echo Configuring the project
pushd $BUILD_FOLDER
# Building with Wayland support will cause linuxdeployqt to include libwayland-client.so in the AppImage.
# Since we always use the host implementation of EGL, this can cause libEGL_mesa.so to fail to load due
# to missing symbols from the host's version of libwayland-client.so that aren't present in the older
# version of libwayland-client.so from our AppImage build environment. When this happens, EGL fails to
# work even in X11. To avoid this, we will disable Wayland support for the AppImage.
#
# We disable DRM support because linuxdeployqt doesn't bundle the appropriate libraries for Qt EGLFS.
if [ "$MODE" == "wayland" ]; then
echo "wayland mode qmake===========================================================>"
qmake $SOURCE_ROOT/moonlight-qt.pro CONFIG+=disable-libdrm CONFIG+=disable-cuda PREFIX=$DEPLOY_FOLDER/usr DEFINES+=APP_IMAGE || fail "Qmake failed!"
else
echo "default mode qmake===========================================================>"
qmake $SOURCE_ROOT/moonlight-qt.pro CONFIG+=disable-wayland CONFIG+=disable-libdrm CONFIG+=disable-cuda PREFIX=$DEPLOY_FOLDER/usr DEFINES+=APP_IMAGE || fail "Qmake failed!"
fi
popd

date
echo Compiling Moonlight in $BUILD_CONFIG configuration
pushd $BUILD_FOLDER
make -j$(nproc) $(echo "$BUILD_CONFIG" | tr '[:upper:]' '[:lower:]') || fail "Make failed!"
popd

date
echo Deploying to staging directory
pushd $BUILD_FOLDER
make install || fail "Make install failed!"
popd
#创建独立运行包
date
echo Creating AppImage
pushd $INSTALLER_FOLDER
echo "导入的qml目录：$SOURCE_ROOT/app/gui"
#加入界面文件 、排除数据库驱动、加入wayland的插件
if [ "$MODE" == "wayland" ]; then
  VERSION=$VERSION linuxdeployqt $DEPLOY_FOLDER/usr/share/applications/com.moonlight_stream.Moonlight.desktop -qmldir=$SOURCE_ROOT/app/gui -appimage \
  -exclude-libs=libqsqlmimer,libqsqlmysql,libqsqlite,libqsqlodbc,libqsqlpsql \
  -extra-plugins=platforms/libqwayland-egl.so,platforms/libqwayland-generic.so,wayland-decoration-client,wayland-graphics-integration-client,wayland-shell-integration \
  || fail "linuxdeployqt failed!"
else
  VERSION=$VERSION linuxdeployqt $DEPLOY_FOLDER/usr/share/applications/com.moonlight_stream.Moonlight.desktop \
  -qmldir=$SOURCE_ROOT/app/gui -appimage -exclude-libs=libqsqlmimer,libqsqlmysql,libqsqlite,libqsqlodbc,libqsqlpsql \
  || fail "linuxdeployqt failed!"
fi

popd

date
echo Build successful