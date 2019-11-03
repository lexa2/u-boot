#!/bin/bash
export ARCH=arm;
export CROSS_COMPILE=arm-linux-gnueabihf-

uver=$(make ubootversion)
ubranch=$(git branch --contains $(git log -n 1 --pretty='%h') | grep -v '(HEAD' | head -1 | sed 's/^..//'| sed 's/^[0-9.-]*//')

echo "ver:$uver,ubranch:$ubranch"

if [[ -e "build.conf" ]];
then
	. build.conf
fi

#values in kB
if [[ "$board" == "bpi-r64" ]];then
	UBOOT_START=768
	ENV_START=1280 #ENV_OFFSET = 0x140000
else
	UBOOT_START=320
	ENV_START=1024 #ENV_OFFSET = 0x100000
fi
MAXSIZE=$(( ($ENV_START - $UBOOT_START)*1024 -1 ))

function upload {
	imagename="u-boot_${uver}-${ubranch}.bin"
	read -e -i $imagename -p "u-boot-filename: " input
	imagename="${input:-$imagename}"

	echo "Name: $imagename"
	echo "uploading to ${uploadserver}:${uploaddir}..."

	srcfile=u-boot.bin

	if [[ "$board" == "bpi-r64" ]];then
		srcfile=u-boot-mtk.bin
	fi
	echo $srcfile
	scp ${srcfile} ${uploaduser}@${uploadserver}:${uploaddir}/${imagename}
}

case $1 in
	"build")
		LANG=C
		CFLAGS=-j$(grep ^processor /proc/cpuinfo  | wc -l)
		make LOCALVERSION="-$ubranch" ${CFLAGS} 2> >(tee "build.log")
		if [[ $? -eq 0 ]];then
			FILESIZE=$(stat -c%s "u-boot.bin");
			if [[ $FILESIZE -gt $MAXSIZE ]]; then
				echo "=============== WARNING ==============="
				echo "u-boot will overwrite env-storage area!"
				echo "if you use this u-boot.bin don't use saveenv!"
			fi;
		else
			echo "build failed!"
		fi
	;;
	"config")
		make menuconfig;
	;;
	"importconfig")
		if [[ "$board" == "bpi-r64" ]];then
			make mt7622_bpir64_efi_defconfig
		else
			make mt7623n_bpir2_defconfig;
		fi
	;;
	"defconfig")
		if [[ "$board" == "bpi-r64" ]];then
			nano configs/mt7622_bpir64_efi_defconfig;
		else
			nano configs/mt7623n_bpir2_defconfig;
		fi
	;;
	"install")
		dev=/dev/sdb
		choice=y
		read -e -i "$dev" -p "Please enter target device: " dev
		if ! [[ "$(lsblk ${dev}1 -o label -n)" == "BPI-BOOT" ]]; then
			read -e -p "this device seems not to be a BPI-R2 SD-Card, do you really want to use this device? [yn]" choice
		fi
		if [[ "$choice" == "y" ]];then
			echo "writing to $dev"
			sudo dd of=$dev if=u-boot.bin bs=1k seek=$UBOOT_START;
			sync
		fi
	;;
	"umount")
		umount /media/$USER/BPI-BOOT
		umount /media/$USER/BPI-ROOT
	;;
	"uenv")
		if [[ "$board" == "bpi-r64" ]];then
			nano /media/$USER/BPI-BOOT/bananapi/bpi-r64/linux/uEnv.txt
		else
			nano /media/$USER/BPI-BOOT/bananapi/bpi-r2/linux/uEnv.txt
		fi
	;;
	"board")
		if [[ "$board" == "bpi-r64" ]];then
			nano board/mediatek/mt7622/mt7622_evb.c
		else
			nano board/mediatek/mt7623/mt7623_rfb.c
		fi
	;;
	"dts")
		if [[ "$board" == "bpi-r64" ]];then
			nano arch/arm/dts/mt7622-bananapi-bpi-r64.dts
		else
			nano arch/arm/dts/mt7623n-bananapi-bpi-r2.dts
		fi
	;;
	"dtsi")
		if [[ "$board" == "bpi-r64" ]];then
			nano arch/arm/dts/mt7622.dtsi
		else
			nano arch/arm/dts/mt7623.dtsi
		fi
	;;
	"soc")
		if [[ "$board" == "bpi-r64" ]];then
			nano ./include/configs/mt7622.h
		else
			nano ./include/configs/mt7623.h
		fi
	;;
	"upload")
		upload
	;;
	*)
		$0 build;
	;;
esac
