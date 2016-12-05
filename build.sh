#!/bin/sh


NAME="OpenBeacon"

BUILD="$NAME/build-atmega328"
SRC=$NAME
RELEASE='Released/FW'


version(){
    cat $NAME/version.h | grep 'RELEASE_NUM' | awk '{print $3}'
}

VERS=`version`

make_one() {
    PROTO=$1

    make -C $SRC PROTO="-DUSE_${PROTO}=1"
    if [ -f $BUILD/$NAME.hex ] 
    then
         mv $BUILD/$NAME.hex $RELEASE/$NAME.${VERS}-${PROTO}.hex && rm -rf $BUILD
         echo "$RELEASE/$NAME.${VERS}-${PROTO}.hex is ok"
    else
        echo "no boult target for ${PROTO} - $BUILD/$NAME.hex " 
    fi
}


make_one 'UAVTALK' &&
make_one 'MAVLINK' &&
make_one 'MWII' &&
make_one 'LTM' 


#make_one 'MAVLINK' '-DWALKERA_TELEM=RssiPin' &&