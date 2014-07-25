/*
 *  Copyright (C) 2012 Altera Corporation <www.altera.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <asm/arch/reset_manager.h>
#include <asm/arch/system_manager.h>
#include <asm/io.h>

#include <netdev.h>
#include <mmc.h>
#include <linux/dw_mmc.h>
#include <asm/arch/interrupts.h>
#include <asm/arch/sdram.h>
#include <asm/arch/sdmmc.h>
#include <phy.h>
#include <micrel.h>
#include <../drivers/net/designware.h>

#include <altera.h>
#include <fpga.h>

#include <asm/arch/amp_config.h>


DECLARE_GLOBAL_DATA_PTR;

/*
 * Declaration for DW MMC structure
 */
#ifdef CONFIG_DW_MMC
struct dw_host socfpga_dw_mmc_host = {
	.need_init_cmd = 1,
	.clock_in = CONFIG_HPS_CLK_SDMMC_HZ / 4,
	.reg = (struct dw_registers *)CONFIG_SDMMC_BASE,
#ifdef CONFIG_SOCFPGA_VIRTUAL_TARGET
	.set_timing = NULL,
	.use_hold_reg = NULL,
#else
	.set_timing = sdmmc_set_clk_timing,
	.use_hold_reg = sdmmc_use_hold_reg,
#endif
};
#endif

/*
 * FPGA programming support for SoC FPGA Cyclone V
 */
/* currently only single FPGA device avaiable on dev kit */
Altera_desc altera_fpga[CONFIG_FPGA_COUNT] = {
	{Altera_SoCFPGA, /* family */
	fast_passive_parallel, /* interface type */
	-1,		/* no limitation as
			additional data will be ignored */
	NULL,		/* no device function table */
	NULL,		/* base interface address specified in driver */
	0}		/* no cookie implementation */
};

/* add device descriptor to FPGA device table */
void socfpga_fpga_add(void)
{
	int i;
	fpga_init();
	for (i = 0; i < CONFIG_FPGA_COUNT; i++)
		fpga_add(fpga_altera, &altera_fpga[i]);
}

/*
 * Print CPU information
 */
int print_cpuinfo(void)
{
	puts("CPU   : Altera SOCFPGA Platform\n");
	return 0;
}

/*
 * Print Board information
 */
int checkboard(void)
{
#ifdef CONFIG_SOCFPGA_VIRTUAL_TARGET
	puts("BOARD : Altera VTDEV5XS1 Virtual Board\n");
#else
	puts("BOARD : Altera SOCFPGA Cyclone 5 Board\n");
#endif
	return 0;
}

/*
 * Initialization function which happen at early stage of c code
 */
int board_early_init_f(void)
{
#ifdef CONFIG_HW_WATCHDOG
	/* disable the watchdog when entering U-Boot */
	watchdog_disable();
#endif
	return 0;
}

/*
 * Miscellaneous platform dependent initialisations
 */
int board_init(void)
{
	/* adress of boot parameters (ATAG or FDT blob) */
	gd->bd->bi_boot_params = 0x00000100;
	return 0;
}

int misc_init_r(void)
{
	/* add device descriptor to FPGA device table */
	socfpga_fpga_add();
	return 0;
}

#if defined(CONFIG_SYS_CONSOLE_IS_IN_ENV) && defined(CONFIG_SYS_CONSOLE_OVERWRITE_ROUTINE)
int overwrite_console(void)
{
	return 0;
}
#endif


#ifndef CONFIG_SOCFPGA_VIRTUAL_TARGET

#define MICREL_KSZ9021_EXTREG_CTRL 11
#define MICREL_KSZ9021_EXTREG_DATA_WRITE 12


/*
 * Write the extended registers in the PHY
 */
static int eth_emdio_write(struct eth_device *dev, u8 addr, u16 reg, u16 val,
		int (*mii_write)(struct eth_device *, u8, u8, u16))

{
	int ret = (*mii_write)(dev, addr,
		MICREL_KSZ9021_EXTREG_CTRL, 0x8000|reg);

	if (0 != ret) {
		printf("eth_emdio_read write0 failed %d\n", ret);
		return ret;
	}
	ret = (*mii_write)(dev, addr, MICREL_KSZ9021_EXTREG_DATA_WRITE, val);
	if (0 != ret) {
		printf("eth_emdio_read write1 failed %d\n", ret);
		return ret;
	}

	return 0;
}

/*
 * DesignWare Ethernet initialization
 * This function overrides the __weak  version in the driver proper.
 * Our Micrel Phy needs slightly non-conventional setup
 */
 static char * mii_reg1,*mii_reg2;
int designware_board_phy_init(struct eth_device *dev, int phy_addr,
		int (*mii_write)(struct eth_device *, u8, u8, u16),
		int (*dw_reset_phy)(struct eth_device *))
{
	unsigned long reg104= 0xa0e0,reg105=0x0;
	mii_reg1 =  getenv("mii_104_reg");
	mii_reg2 = getenv("mii_105_reg");

	if(mii_reg1 != NULL)
		reg104 = simple_strtoul(mii_reg1,NULL,16);
	if(mii_reg2 != NULL)
		reg105 = simple_strtoul(mii_reg2,NULL,16);

	printf("reg104=%x,reg105=%x\n", reg104, reg105);

	
	if ((*dw_reset_phy)(dev) < 0)
		return -1;
	
	/* add 2 ns of RXC PAD Skew and 2.6 ns of TXC PAD Skew */
	if (eth_emdio_write(dev, phy_addr,
		MII_KSZ9021_EXT_RGMII_CLOCK_SKEW, reg104, mii_write) < 0)
		return -1;

	/* set no PAD skew for data */
	if (eth_emdio_write(dev, phy_addr,
		MII_KSZ9021_EXT_RGMII_RX_DATA_SKEW, reg105, mii_write) < 0)
		return -1;
	
	return 0;
}
#endif

/* We know all the init functions have been run now */
int board_eth_init(bd_t *bis)
{
#ifndef CONFIG_SOCFPGA_VIRTUAL_TARGET
	/* setting emac1 to rgmii */
	clrbits_le32(CONFIG_SYSMGR_EMAC_CTRL,
		(SYSMGR_EMACGRP_CTRL_PHYSEL_MASK <<
		SYSMGR_EMACGRP_CTRL_PHYSEL_WIDTH));

	setbits_le32(CONFIG_SYSMGR_EMAC_CTRL,
		(SYSMGR_EMACGRP_CTRL_PHYSEL_ENUM_RGMII <<
		SYSMGR_EMACGRP_CTRL_PHYSEL_WIDTH));

	int rval = designware_initialize(0, CONFIG_EMAC1_BASE,
			CONFIG_EPHY1_PHY_ADDR, PHY_INTERFACE_MODE_RGMII);

	debug("board_eth_init %d\n", rval);
	return rval;
#else
	return 0;
#endif
}

/*
 * Initializes MMC controllers.
 * to override, implement board_mmc_init()
 */
int cpu_mmc_init(bd_t *bis)
{
#ifdef CONFIG_DW_MMC
	//return dw_mmc_init(&socfpga_dw_mmc_host);
	return altera_dwmmc_init(CONFIG_SDMMC_BASE,
		CONFIG_DWMMC_BUS_WIDTH, 0);
#else
	return 0;
#endif
}

unsigned long cpu1start_addr = 0xffd080c4;
unsigned long sys_manager_base_addr = 0xffd08000;
unsigned long rst_manager_base_addr = 0xffd05000;
extern char secondary_trampoline, secondary_trampoline_end;

#ifdef CONFIG_SPL_BUILD

struct amp_share_param *asp = (struct amp_share_param *)SH_MEM_START;

extern unsigned int save_timer_value;
/* clear the share parameters memory region for u-boot, bare metal, linux */
void amp_share_param_init(void)
{
	memset(asp, 0x0, sizeof(struct amp_share_param));
}

/* using OSC Timer 0 to measue the loading image time, get the start time stamp here */
void load_bm_start(void)
{
	reset_timer();
	asp->load_bm_start = get_timer(0);

}
/* using OSC Timer 0 to measue the loading image time, get the end time stamp here */
void load_bm_end(void)
{
	asp->load_bm_end  = get_timer(0);
}
/*
 * CPU1-->release reset by cpu0--> run from the 0x0 address-->
 * get the real executable address from the cpu1startaddr of the sysmgr module
 */
void boot_bm_on_cpu1(void)
{
	unsigned long boot_entry;
	int trampoline_size;
	unsigned int val;
	unsigned int cpu=1;

	/* save the bm start time stamp */
	memcpy(&(asp->boot_start_stamp), &save_timer_value, sizeof(unsigned int));

	writel(0, 0xffd0501c);
	/* code loading the real executable address, and it locates at arch/arm/cpu/armv7/start.S */
	trampoline_size = &secondary_trampoline_end - &secondary_trampoline;
	memcpy(0, &secondary_trampoline, trampoline_size);
	/* write bare metal start address into cpu1startaddress of the sysmgr module */
	writel(AMPPHY_START, (sys_manager_base_addr+(cpu1start_addr & 0x000000ff)));

	__asm__ __volatile__("dsb\n");

	/* release the CPU1 reset */
	writel(0x0, rst_manager_base_addr + 0x10);

}

/* 0: is ok 
  * 1: is timeout
 */
int cpu0_wait_cpu1_load_rbf(void)
{
	unsigned int timeout  = 5000; /*  unit ms*/
	unsigned int start;
	
	reset_timer();
	start = get_timer(0);
	//while(1)
	while(get_timer(start) < timeout)
		if(readl(&(asp->preloader_wait_bm_load_rbf)) == (('R' << 16)|('B' << 8)|('F' << 0)))
			return 0;
	
	return 1;

}

#endif	/* CONFIG_SPL_BUILD */


#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
	/* create event for tracking ECC count */
	setenv_ulong("ECC_SDRAM", 0);
#ifndef CONFIG_SOCFPGA_VIRTUAL_TARGET
	setenv_ulong("ECC_SDRAM_SBE", 0);
	setenv_ulong("ECC_SDRAM_DBE", 0);
#endif
	/* register SDRAM ECC handler */
	/*irq_register(IRQ_ECC_SDRAM,
		irq_handler_ecc_sdram,
		(void *)&irq_cnt_ecc_sdram, 0);*/

	return 0;
}
#endif

#ifndef CONFIG_SYS_DCACHE_OFF
void enable_caches(void)
{
	/* Enable D-cache. I-cache is already enabled in start.S */
	dcache_enable();
}
#endif
