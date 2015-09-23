#!/bin/bash


USAGE="<interface>" 

if [ "$#" -ne "1" ];
then
    echo -e $USAGE
    exit
fi



INTERFACE=$1


function cleanup()
{
    ifconfig | grep -q $INTERFACE
    if [ $? -eq 0 ]; 
    then
	echo "bringing down ${INTERFACE}"
	ifconfig $INTERFACE down
    fi

}

cleanup
#trap cleanup INT
#
iwconfig ${INTERFACE} mode monitor
# 
ifconfig ${INTERFACE} up
