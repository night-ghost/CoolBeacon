#!/bin/sh

version(){
    cat OpenBeacon/version.h | grep 'RELEASE_NUM' | awk '{print $3}'
}

RELEASE='Released/FW'
BUILD='OpenBeacon/build-atmega328'

[ -f $BUILD/OpenBeacon.hex ] && mv $BUILD/OpenBeacon.hex $RELEASE/OpenBeacon.`version`-release.hex

rm -rf $BUILD
rm -f latest.zip
zip -r latest.zip Released

git add . -A
git commit
git push
