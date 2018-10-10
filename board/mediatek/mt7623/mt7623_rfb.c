// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018 MediaTek Inc.
 */

#include <common.h>

DECLARE_GLOBAL_DATA_PTR;

int board_init(void)
{
	/* address of boot parameters */
	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x100;

	return 0;
}

int mmc_get_env_dev(void)
{
	printf("%s:%d %s\n",__FILE__,__LINE__,__FUNCTION__);
	int g_mmc_devid = -1;
	char *uflag = (char *)0x81DFFFF0;
	if((uflag[0] == 'e') && (uflag[1] == 'M') && (uflag[2] == 'M') && (uflag[3] == 'C'))
	{
		g_mmc_devid = 0;
		printf("Boot From Emmc(id:%d)\n\n", g_mmc_devid);
	}
	else
	{
		g_mmc_devid = 1;
		printf("Boot From SD(id:%d)\n\n", g_mmc_devid);
	}
	return CONFIG_SYS_MMC_ENV_DEV; //have to replaced by g_mmc_devid if sdcard-offset is implemented
}
