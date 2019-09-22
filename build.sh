#!/bin/bash
CCVER=$(arm-linux-gnueabihf-gcc --version |grep arm| sed -e 's/^.* \([0-9]\.[0-9-]\).*$/\1/')
echo "gcc-version (CROSS_COMPILE):"$CCVER
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
		ls -lh u-boot-mtk.bin
		;;
	"upload")
		host=192.168.0.29
		dir=/home/frank/Schreibtisch/
		grep '^OFF_BOARD_SD_CARD_COMPONENT=y' .config;if [[ $? -eq 0 ]];then FLASH="SD";fi
		grep '^ON_BOARD_EMMC_COMPONENT=y' .config;if [[ $? -eq 0 ]];then FLASH="EMMC";fi
		grep '^CONFIG_RTL8367=y' .config;if [[ $? -eq 0 ]];then ETH="RTL8367";fi
		grep '^CONFIG_MT7531=y' .config;if [[ $? -eq 0 ]];then ETH="MT7531";fi
		filename=u-boot-mtk_r64_${FLASH,,}_${ETH,,}_gcc${CCVER}.bin
		echo "uploading $filename to $host:$dir"
		scp u-boot-mtk.bin $host:$dir/$filename
		;;
	"install")
		UBOOT=u-boot-mtk.bin
		read -e -i "$UBOOT" -p "Please enter source file: " UBOOT
		if [[ ! -e $UBOOT ]];then
			echo "please build uboot first or set correct file..."
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
	"soc")
		nano include/configs/mt7622_evb.h
		;;
	"umount")
		umount /media/$USER/BPI-BOOT
		umount /media/$USER/BPI-ROOT
	;;
esac
