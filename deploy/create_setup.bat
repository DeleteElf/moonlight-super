@echo off
echo "create setup dir"
mkdir setup

cd ..
echo %cd%

echo "copy application ..."
copy build\app\release\Moonlight.exe deploy\setup\

echo "copy sub dll ..."
xcopy build\AntiHooking\release\*.dll deploy\setup\ /y

echo "copy libs ..."
xcopy libs\windows\lib\x64\*.dll deploy\setup\ /y

cd deploy\setup
echo "linking files ..."
echo %cd%
C:\Qt\6.9.1\msvc2022_64\bin\windeployqt6.exe  Moonlight.exe --qmldir %cd%\..\..\app\gui
pause