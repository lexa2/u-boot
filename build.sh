#!/bin/bash
case "$1" in
	"config") make menuconfig;;
	"build"|"") make;;
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
