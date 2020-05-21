/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Configuration for MediaTek MT7623 SoC
 *
 * Copyright (C) 2018 MediaTek Inc.
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef __MT7623_H
#define __MT7623_H

#include <linux/sizes.h>
#include <linux/stringify.h>

/* Miscellaneous configurable options */
#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_INITRD_TAG
#define CONFIG_CMDLINE_TAG

#define CONFIG_SYS_MAXARGS		8
#define CONFIG_SYS_BOOTM_LEN		SZ_64M
#define CONFIG_SYS_CBSIZE		SZ_1K
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE +	\
					sizeof(CONFIG_SYS_PROMPT) + 16)

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		SZ_4M
#define CONFIG_SYS_NONCACHED_MEMORY	SZ_1M

/* Environment */
/* Allow to overwrite serial and ethaddr */
#define CONFIG_ENV_OVERWRITE

/* Preloader -> Uboot */
#define CONFIG_SYS_INIT_SP_ADDR		(CONFIG_SYS_TEXT_BASE + SZ_2M - \
					 GENERATED_GBL_DATA_SIZE)

/* MMC */
#define MMC_SUPPORTS_TUNING

/* DRAM */
#define SDRAM_OFFSET(x) 0x8##x
#define CONFIG_SYS_SDRAM_BASE		0x80000000

/* UBoot -> Kernel */
#define CONFIG_LOADADDR			SDRAM_OFFSET(4000000)
#define CONFIG_SYS_LOAD_ADDR		CONFIG_LOADADDR

#define KERNEL_ADDR_R  __stringify(CONFIG_LOADADDR)
#define FDT_ADDR_R     __stringify(SDRAM_OFFSET(5000000))
#define SCRIPT_ADDR_R  __stringify(SDRAM_OFFSET(5100000))
#define PXEFILE_ADDR_R __stringify(SDRAM_OFFSET(5200000))
#define RAMDISK_ADDR_R __stringify(SDRAM_OFFSET(5300000))

/* This is needed for kernel booting */
#define FDT_HIGH			"fdt_high=0xac000000\0"

#define BOOT_TARGET_DEVICES(func) \
        func(MMC, mmc, 1) \
        func(SCSI, scsi, 0) \
        func(SCSI, scsi, 1) \
        func(MMC, mmc, 0)

#define BOOT_TARGET_DEVICES_SCSI_NEEDS_PCI

#include <config_distro_bootcmd.h>

#define MEM_LAYOUT_ENV_SETTINGS \
	"kernel_addr_r=" KERNEL_ADDR_R "\0" \
	"fdt_addr_r=" FDT_ADDR_R "\0" \
	"scriptaddr=" SCRIPT_ADDR_R "\0" \
	"pxefile_addr_r=" PXEFILE_ADDR_R "\0" \
	"ramdisk_addr_r=" RAMDISK_ADDR_R "\0"

/* Extra environment variables */
#define CONFIG_EXTRA_ENV_SETTINGS \
        MEM_LAYOUT_ENV_SETTINGS \
        FDT_HIGH \
        BOOTENV

/* Ethernet */
#define CONFIG_IPADDR			192.168.1.1
#define CONFIG_SERVERIP			192.168.1.2

#define CONFIG_SYS_MMC_ENV_DEV		0

#endif
