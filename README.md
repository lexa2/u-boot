# Uboot for BananaPi R64

* compileable / working with gcc 4.8-8.3
* both switches (rtl8367s, mt7531) supported
* emmc supported

# Usage
please make "make mrproper" after changing branch to bpi-r64,
else you may get compile-error on missing gpio.h

* ```./build.sh config```
  * if it shows only 2 entries, exit+save and run again
  * change only flash-type (SD/eMMC) or Switch
* ```./build.sh``` compiles uboot using linaro Cross-Compiler
* ```./build.sh install``` installs u-boot-mtk.bin to right offset (768k) to sd-card
* ```./build.sh rename``` u-boot-mtk.bin to have switch/flash/compiler in name



