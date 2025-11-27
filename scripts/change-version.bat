@echo off

echo change to master
cd ..

cd libs
git checkout master
git pull
git checkout aba5ddf

cd ..
cd app/SDL_GameControllerDB
git checkout master
git pull
git checkout 7979e7b

cd ..
cd ..
cd moonlight-common-c/moonlight-common-c
git checkout dev-master
git pull

cd enet
git checkout master
git pull
git checkout 115a10b

pause