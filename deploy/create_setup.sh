date

echo "create setup dir"
mkdir setup

cd ..
pwd

echo "copy application ..."
cp build/Desktop_Qt_6_9_3-Release/app/Moonlight deploy/setup/

cd deploy/setup
pwd
echo "linking files ..."
export QT_ROOT=~/Qt/6.9.3/gcc_64
echo "QT_ROOT=$QT_ROOT"

export PATH=$QT_ROOT/bin:$PATH
export LD_LIBRARY_PATH=$QT_ROOT/lib:$LD_LIBRARY_PATH
export QT_PLUGIN_PATH=$QT_ROOT/plugins:$QT_PLUGIN_PATH
export QML2_IMPORT_PATH=$QT_ROOT/qml:$QML2_IMPORT_PATH
echo "PATH=$PATH"

linuxdeployqt Moonlight -qmldir=$PWD/../../app/gui -exclude-libs=libqsqlmimer,libqsqlmysql,libqsqlite,libqsqlodbc,libqsqlpsql

date