#!/bin/bash
CCVER=$(arm-linux-gnueabihf-gcc --version |grep arm| sed -e 's/^.* \([0-9]\.[0-9-]\).*$/\1/')
if [[ $CCVER =~ ^[789] ]]; then
	echo "arm-linux-gnueabihf-gcc version 7+ currently not supported";exit 1;
fi

LANG=C
CFLAGS=-j$(grep ^processor /proc/cpuinfo  | wc -l)
export CROSS_COMPILE='ccache arm-linux-gnueabihf-'

case "$1" in
	"config") make menuconfig;;
	"build"|"")
		exec 3> >(tee build.log)
		make ${CFLAGS} 2>&3
		ret=$?
		exec 3>&-
		;;
	"install")
		UBOOT=u-boot-mtk.bin
		if [[ ! -e $UBOOT ]];then
			echo "please build uboot first..."
			exit;
		fi
		dev=/dev/sdb
		read -e -i $dev -p "device to write: " input
		dev="${input:-$dev}"
		choice=y
		if ! [[ "$(lsblk ${dev}1 -o label -n)" == "BPI-BOOT" ]]; then
			read -e -p "this device seems not to be a BPI-R2 SD-Card, do you really want to use this device? [yn]" choice
		fi
		if [[ "$choice" == "y" ]];then
			echo "writing to $dev"
			sudo dd if=$UBOOT of=$dev bs=1k seek=768 #768k = 0xC0000
			sync
		fi
	;;
esac
