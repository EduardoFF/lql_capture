#!/bin/bash


USAGE="<interface> <ip with mask> <channel> <power (dBm)>  optional: if environment variable \n\
DO_FISH equals 1, then it creates a monitor interface over <interface> \n\
example: ./ADHOC_connection_script.sh wlan2 10.42.43.14/24 3 0" 

if [ "$#" -ne "1" ];
then
    echo -e $USAGE
    exit
fi



INTERFACE=$1
FISHI=fish1

function cleanup()
{
    ifconfig | grep -q ${FISHI}
    if [ $? -eq 0 ]; 
    then
	echo "bringing down ${FISHI}"
	ifconfig ${FISHI} down
	iw dev ${FISHI} del
    fi
}

cleanup
#trap cleanup INT


#

echo "creating monitor interface ${FISHI}"
iw dev ${INTERFACE} interface add ${FISHI} type monitor flags none
ifconfig ${FISHI} up
