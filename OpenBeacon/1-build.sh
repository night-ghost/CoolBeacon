#!/bin/sh


NAME="OpenBeacon"

BUILD="$NAME/build-atmega328"
SRC=$NAME
RELEASE='Released/FW'


version(){
    cat version.h | grep 'RELEASE_NUM' | awk '{print $3}'
}

VERS=`version`

make_one() {
    PROTO=$1

    make PROTO="-DUSE_${PROTO}=1"
}


make_one 'MAVLINK' 
